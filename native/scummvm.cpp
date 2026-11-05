#include "scummvm.h"
#include "i18n.h"
#include "librarylayout.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCollator>
#include <QUuid>
#include <algorithm>

ScummConfig ScummLibrary::readConfig(const QString &path){
    ScummConfig result;QFile file(path);
    if(!file.open(QIODevice::ReadOnly))return result;
    QString section;
    for(auto line:QString::fromUtf8(file.readAll()).split('\n')){
        line=line.trimmed();
        if(line.startsWith('#')||line.startsWith(';')||line.isEmpty())continue;
        if(line.startsWith('[')&&line.endsWith(']')){section=line.mid(1,line.size()-2);continue;}
        const int equals=line.indexOf('=');
        if(!section.isEmpty()&&equals>0)result[section][line.left(equals).trimmed()]=line.mid(equals+1).trimmed();
    }
    return result;
}
QByteArray ScummLibrary::serialize(const ScummConfig &config){
    QString text;
    for(auto s=config.cbegin();s!=config.cend();++s){
        text+='['+s.key()+"]\n";
        for(auto v=s->cbegin();v!=s->cend();++v)text+=v.key()+'='+v.value()+'\n';
        text+='\n';
    }
    return text.toUtf8();
}
QString ScummLibrary::relativePath(const QString &base,const QString &path){
    const QString canonicalBase=QFileInfo(base).canonicalFilePath(),canonical=QFileInfo(path).canonicalFilePath();
    if(canonicalBase.isEmpty()||canonical.isEmpty()||!canonical.startsWith(canonicalBase+'/'))return {};
    return QDir(base).relativeFilePath(path);
}
QString ScummLibrary::identity(const ScummSection &s){
    // La identidad no depende de la letra del disco ni de la carpeta de instalación.
    QStringList parts;
    for(const auto &key:{"vetust_relative_path","engineid","gameid","platform","language","extra"})parts<<s.value(key);
    return QString::fromLatin1(QCryptographicHash::hash(parts.join(QChar(0)).toUtf8(),QCryptographicHash::Sha256).toHex().left(24));
}
ScummLibrary::ScummLibrary(QString r,QObject *parent):QObject(parent),root(r){
    connect(&detector,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)fail(uiText("Could not start ScummVM: ")+detector.errorString());});
    connect(&detector,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        if(!scanning)return;
        if(status!=QProcess::NormalExit||code!=0){fail(uiText("ScummVM could not scan the folder. Your previous catalog has been kept."));return;}
        const auto found=readConfig(temporary->path()+"/scan.ini");
        for(auto section:found){
            if(section.value("gameid").isEmpty())continue;
            const QString relative=relativePath(base,section.value("path"));
            if(relative.isEmpty())continue;
            section["vetust_relative_path"]=relative;
            detected["vetust-"+identity(section)]=section;
        }
        nextFolder();
    });
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart)fail(uiText("Could not start ScummVM: ")+process.errorString());});
    connect(&process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        running=false;reload();
        message=code==0&&status==QProcess::NormalExit?uiText("ScummVM closed. Your settings and saves have been kept."):uiText("ScummVM ended with an error. Check logs/scummvm.log.");emit changed();
    });
}
QString ScummLibrary::executable() const{
#ifdef Q_OS_WIN
    const QString bundled=root+"/runtime/scummvm/scummvm.exe";
#else
    const QString bundled=root+"/runtime/scummvm/scummvm";
#endif
    return QFileInfo(bundled).isExecutable()?bundled:QStandardPaths::findExecutable("scummvm");
}
bool ScummLibrary::available() const{return !executable().isEmpty();}
void ScummLibrary::setLibrary(const QString &library,const QString &folder,const QString &metadata){
    if(running)return;
    base=folder.isEmpty()?(library.isEmpty()?QString():QDir::cleanPath(LibraryLayout::games(library,"scummvm"))):QDir::cleanPath(folder);metadataBase=QFileInfo(base+"/metadata").isDir()?base+"/metadata":metadata.isEmpty()?LibraryLayout::metadata(library,"scummvm",base):metadata;reload();emit changed();
}
void ScummLibrary::reload(){
    config=readConfig(configPath());
    for(auto it=config.begin();it!=config.end();++it){
        const auto current=relativePath(base,it->value("path"));
        if(!current.isEmpty())(*it)["vetust_relative_path"]=current;
        const auto relative=it->value("vetust_relative_path");
        if(!relative.isEmpty()&&!base.isEmpty()&&!QDir::isAbsolutePath(relative)&&!relative.split('/').contains(".."))(*it)["path"]=base+'/'+relative;
    }
}
bool ScummLibrary::save(const ScummConfig &next){
    QDir().mkpath(root+"/config");
    if(QFileInfo::exists(configPath())){
        const QString backup=root+"/backups/scummvm/"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")+"-"+QUuid::createUuid().toString(QUuid::Id128)+".ini";
        if(!QDir().mkpath(QFileInfo(backup).absolutePath())||!QFile::copy(configPath(),backup)){fail(uiText("Could not back up the ScummVM settings."));return false;}
    }
    QSaveFile file(configPath());const QByteArray bytes=serialize(next);
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){fail(uiText("Could not save the ScummVM settings."));return false;}
    config=next;return true;
}
void ScummLibrary::fail(const QString &error){running=false;scanning=false;message=error;temporary.reset();emit changed();}
bool ScummLibrary::ready(){
    if(running||(blocked&&blocked()))return false;
    if(!available()){message=uiText("Install ScummVM to use this library.");emit changed();return false;}
    if(base.isEmpty()||!QFileInfo(base).isDir()){message=uiText("The scummvm folder is unavailable. Connect the library drive.");emit changed();return false;}
    return true;
}
void ScummLibrary::enter(){
    if(running)return;
    if(!available()||base.isEmpty()||!QFileInfo(base).isDir()){ready();return;}
    reload();
    if(config.isEmpty())scan();else emit changed();
}
void ScummLibrary::scan(){
    if(!ready())return;
    QDir().mkpath(root+"/cache");QDir().mkpath(root+"/logs");
    temporary=std::make_unique<QTemporaryDir>(root+"/cache/scummvm-scan-XXXXXX");
    if(!temporary->isValid()){fail(uiText("Cannot prepare the ScummVM scan."));return;}
    pending=QDir(base).entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);pending.removeAll("metadata");total=pending.size();detected.clear();running=true;scanning=true;
    nextFolder();
}
void ScummLibrary::nextFolder(){
    if(pending.isEmpty()){finishScan();return;}
    const QString folder=base+'/'+pending.takeFirst();
    if(relativePath(base,folder).isEmpty()){QTimer::singleShot(0,this,&ScummLibrary::nextFolder);return;}
    // Cada carpeta se detecta por separado: --add omite ediciones iguales en carpetas distintas.
    QFile::remove(temporary->path()+"/scan.ini");
    message=uiText("Scanning ScummVM folders: %1 / %2").arg(total-pending.size()).arg(total);emit changed();
    detector.setStandardOutputFile(root+"/logs/scummvm-scan.log",QIODevice::Append);
    detector.setStandardErrorFile(root+"/logs/scummvm-scan.log",QIODevice::Append);
    detector.start(executable(),{"--config="+temporary->path()+"/scan.ini","--path="+folder,"--recursive","--add"});
}
void ScummLibrary::finishScan(){
    auto next=config;
    // Las opciones personales prevalecen al volver a detectar una edición conocida.
    for(auto it=detected.cbegin();it!=detected.cend();++it){
        QString target=it.key();ScummSection section=it.value();
        for(auto old=config.cbegin();old!=config.cend();++old){
            auto candidate=old.value();
            if(candidate.value("gameid").isEmpty())continue;
            candidate["vetust_relative_path"]=relativePath(base,candidate.value("path"));
            if(identity(candidate)==identity(section)){target=old.key();for(auto v=old->cbegin();v!=old->cend();++v)section[v.key()]=v.value();break;}
        }
        section["path"]=it->value("path");section["vetust_relative_path"]=it->value("vetust_relative_path");
        if(!section.contains("savepath"))section["savepath"]=root+"/data/scummvm/saves/"+target;
        next[target]=section;
    }
    if(!next["scummvm"].contains("fullscreen"))next["scummvm"]["fullscreen"]="true";
    if(!next["scummvm"].contains("aspect_ratio"))next["scummvm"]["aspect_ratio"]="true";
    if(!next["scummvm"].contains("savepath"))next["scummvm"]["savepath"]=root+"/data/scummvm/saves";
    const int count=detected.size();
    if(!save(next))return;
    running=false;scanning=false;temporary.reset();message=uiText("ScummVM catalog updated: %1 detected editions.").arg(count);emit changed();
}
static QPair<QString,int> seriesOrder(const QVariantMap &game){
    // Los identificadores mantienen juntas las sagas aunque cambien el idioma o el subtítulo.
    static const QHash<QString,QPair<QString,int>> series={
        {"monkey",{"Monkey Island",1}},{"monkey2",{"Monkey Island",2}},{"comi",{"Monkey Island",3}},
        {"indy3",{"Indiana Jones",1}},{"atlantis",{"Indiana Jones",2}},
        {"lsl1",{"Leisure Suit Larry",1}},{"lsl1sci",{"Leisure Suit Larry",1}},
        {"lsl2",{"Leisure Suit Larry",2}},{"lsl3",{"Leisure Suit Larry",3}},
        {"lsl5",{"Leisure Suit Larry",5}},{"lsl6",{"Leisure Suit Larry",6}},
        {"lsl6hires",{"Leisure Suit Larry",6}},{"lsl7",{"Leisure Suit Larry",7}},
        {"maniac",{"Maniac Mansion",1}},{"tentacle",{"Maniac Mansion",2}}
    };
    return series.value(game["gameId"].toString(),{game["title"].toString(),0});
}
QVariantList ScummLibrary::games() const{
    QVariantList result;
    const auto hidden=QJsonDocument::fromJson(config.value("scummvm").value("vetust_hidden_folders").toUtf8()).array();
    if(base.isEmpty()||!QFileInfo(base).isDir())return result;
    for(const auto &folder:QDir(base).entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name)){
        if(folder=="metadata"||relativePath(base,base+'/'+folder).isEmpty())continue;
        QVariantList variants;QString selected;
        for(auto it=config.cbegin();it!=config.cend();++it){
            if(it->value("gameid").isEmpty())continue;
            const QString rel=relativePath(base,it->value("path"));
            if(rel.isEmpty()||rel.section('/',0,0)!=folder)continue;
            variants<<QVariantMap{{"target",it.key()},{"title",it->value("description",it->value("gameid"))},{"gameId",it->value("gameid")},{"language",it->value("language")}};
            if(it->value("vetust_selected")=="true")selected=it.key();
        }
        std::sort(variants.begin(),variants.end(),[](const QVariant &a,const QVariant &b){return a.toMap()["title"].toString().localeAwareCompare(b.toMap()["title"].toString())<0;});
        if(variants.size()==1)selected=variants.first().toMap()["target"].toString();
        auto first=variants.isEmpty()?QVariantMap{}:variants.first().toMap();
        bool differentGames=false;
        for(const auto &variant:variants)if(variant.toMap()["gameId"]!=first["gameId"])differentGames=true;
        if(!selected.isEmpty()){
            for(const auto &variant:variants)if(variant.toMap()["target"]==selected){first=variant.toMap();break;}
        }else if(differentGames)first={};
        result<<QVariantMap{{"folder",folder},{"hidden",hidden.contains(folder)},{"title",first.value("title",folder).toString().section(" (",0,0)},{"gameId",first.value("gameId")},{"variants",variants},{"target",selected},{"cover",localMedia(folder,{"portada.jpg","cover.jpg","portada.png","cover.png"})},{"manual",localMedia(folder,{"manual.pdf"})}};
    }
    // La configuración regional C no admite comparación numérica en QCollator.
    QCollator collator(QLocale().language()==QLocale::C?QLocale(QLocale::English):QLocale());
    collator.setCaseSensitivity(Qt::CaseInsensitive);collator.setNumericMode(true);
    std::sort(result.begin(),result.end(),[&collator](const QVariant &a,const QVariant &b){
        const auto left=a.toMap(),right=b.toMap();const auto x=seriesOrder(left),y=seriesOrder(right);
        const int series=collator.compare(x.first,y.first);
        if(series!=0)return series<0;
        if(x.second!=y.second)return x.second<y.second;
        const int title=collator.compare(left["title"].toString(),right["title"].toString());
        if(title!=0)return title<0;
        return left["folder"].toString()<right["folder"].toString();
    });return result;
}
void ScummLibrary::choose(const QString &target){
    if(!ready()||!config.contains(target))return;
    const QString folder=relativePath(base,config[target].value("path")).section('/',0,0);
    if(folder.isEmpty())return;
    auto next=config;
    for(auto it=next.begin();it!=next.end();++it)if(relativePath(base,it->value("path")).section('/',0,0)==folder)(*it)["vetust_selected"]=it.key()==target?"true":"false";
    if(save(next))emit changed();
}
void ScummLibrary::setHidden(const QString &folder,bool hidden){
    if(running||(blocked&&blocked()))return;
    bool exists=false;
    for(const auto &game:games())if(game.toMap()["folder"]==folder){exists=true;break;}
    if(!exists)return;
    auto next=config;
    auto folders=QJsonDocument::fromJson(next.value("scummvm").value("vetust_hidden_folders").toUtf8()).array();
    if(folders.contains(folder)==hidden)return;
    if(hidden)folders.append(folder);
    else for(int i=folders.size()-1;i>=0;--i)if(folders[i].toString()==folder)folders.removeAt(i);
    // Guardar por carpeta conserva la elección al redetectar o trasladar la biblioteca.
    next["scummvm"]["vetust_hidden_folders"]=QString::fromUtf8(QJsonDocument(folders).toJson(QJsonDocument::Compact));
    if(save(next))emit changed();
}
void ScummLibrary::start(const QString &target){
    if(!ready())return;
    if(!target.isEmpty()&&(!config.contains(target)||relativePath(base,config[target].value("path")).isEmpty())){message=uiText("This ScummVM edition is unavailable. Update the catalog.");emit changed();return;}
    // Las partidas permanecen fuera de la biblioteca compartida, también al usar el gestor.
    if(!config["scummvm"].contains("savepath"))config["scummvm"]["savepath"]=root+"/data/scummvm/saves";
    for(const auto &section:config)if(!section.value("savepath").isEmpty()&&!QDir().mkpath(section.value("savepath"))){fail(uiText("Cannot create the ScummVM saves folder."));return;}
    if(!save(config))return;
    QDir().mkpath(root+"/logs");
    QStringList args={"--config="+configPath(),"--logfile="+root+"/logs/scummvm.log"};
    if(!target.isEmpty())args<<target;
    process.setWorkingDirectory(root);process.setStandardOutputFile(root+"/logs/scummvm-console.log",QIODevice::Append);process.setStandardErrorFile(root+"/logs/scummvm-console.log",QIODevice::Append);
    running=true;message=target.isEmpty()?uiText("ScummVM manager is open. Close it to return here."):uiText("Playing in ScummVM. Close ScummVM to return here.");emit changed();
    process.start(executable(),args);
}
void ScummLibrary::play(const QString &target){if(!target.isEmpty())start(target);}
void ScummLibrary::manage(){start({});}
void ScummLibrary::openFolder(const QString &folder){
    const QString path=folder.isEmpty()?base:base+'/'+folder;
    if(!base.isEmpty()&&QFileInfo(path).isDir()&&(folder.isEmpty()||!relativePath(base,path).isEmpty()))QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

QString ScummLibrary::localMedia(const QString &folder,const QStringList &names) const{
    if(base.isEmpty()||folder.isEmpty()||folder.contains('/')||folder.contains('\\')||folder=="..")return {};
    const QString directory=metadataBase+'/'+folder;
    if(relativePath(metadataBase,directory).isEmpty())return {};
    const auto files=QDir(directory).entryList(QDir::Files,QDir::Name);
    for(const auto &name:names)for(const auto &file:files){
        const QString path=directory+'/'+file;
        if(file.compare(name,Qt::CaseInsensitive)==0&&!relativePath(directory,path).isEmpty())return QUrl::fromLocalFile(path).toString();
    }
    return {};
}
void ScummLibrary::openManual(const QString &folder){
    if(running||(blocked&&blocked()))return;
    const auto url=localMedia(folder,{"manual.pdf"});
    if(url.isEmpty())return;
    for(const auto &entry:games())if(entry.toMap()["folder"]==folder){emit manualReady(QUrl(url),uiText("Manual · ")+entry.toMap()["title"].toString());return;}
}
