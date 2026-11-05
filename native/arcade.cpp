#include "arcade.h"
#include "i18n.h"
#include "librarylayout.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

static QJsonObject readObject(const QString &path){QFile f(path);return f.open(QIODevice::ReadOnly)?QJsonDocument::fromJson(f.readAll()).object():QJsonObject{};}
ArcadeLibrary::ArcadeLibrary(QString r,QObject *parent):QObject(parent),root(r){
    prefs=readObject(root+"/config/arcade.json");
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){running=false;notify(uiText("Could not start MAME: ")+process.errorString());});
    connect(&process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        running=false;notify(code==0&&status==QProcess::NormalExit?uiText("MAME closed · Your saves have been kept"):uiText("MAME ended with an error. Check logs/mame.log."));
    });
    connect(&scanner,&QProcess::readyReadStandardOutput,this,[this]{
        const auto lines=QString::fromUtf8(scanner.readAllStandardOutput()).split('\n');
        for(const auto &line:lines)if(line.count('/')==1 && line.section('/',0,0).toInt()>0)notify(uiText("Auditing MAME ROMs and reading artwork…")+" "+line.trimmed());
    });
    connect(&scanner,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){scanning=false;notify(uiText("Could not update the Arcade catalog: ")+scanner.errorString());});
    connect(&scanner,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        scanning=false;
        if(code!=0||status!=QProcess::NormalExit){notify(uiText("Could not update the Arcade catalog: ")+QString::fromUtf8(scanner.readAllStandardError()).right(1500));return;}
        reload();notify(uiText("Arcade catalog updated"));
    });
}
void ArcadeLibrary::notify(const QString &text){message=text;emit changed();}
QString ArcadeLibrary::executable() const{
    const QString bundled=root+"/runtime/mame-0.241/mame";
    return QFileInfo(bundled).isExecutable()?bundled:QString();
}
QString ArcadeLibrary::safeFile(const QString &base,const QString &relative){
    if(relative.isEmpty()||QDir::isAbsolutePath(relative))return {};
    const auto folder=QFileInfo(base).canonicalFilePath();
    const QFileInfo file(QDir(base).filePath(relative));const auto canonical=file.canonicalFilePath();
    return !folder.isEmpty()&&file.isFile()&&canonical.startsWith(folder+"/")?canonical:QString();
}
QString ArcadeLibrary::cachePath() const{return root+"/data/arcade/"+QString::fromLatin1(QCryptographicHash::hash((base+"\n"+metadataBase).toUtf8(),QCryptographicHash::Sha256).toHex())+".json";}
void ArcadeLibrary::setLibrary(const QString &path,const QString &metadata){
    metadataOverride=metadata;
    prefs=readObject(root+"/config/arcade.json");
    library=path;const QString selected=prefs["folder"].toString();
    const auto next=selected.isEmpty()?(library.isEmpty()?QString():LibraryLayout::games(library,"mame")):selected;
    base=next;metadataBase=QFileInfo(base+"/metadata").isDir()?base+"/metadata":metadata.isEmpty()?LibraryLayout::metadata(library,"mame",base):metadata;reload();emit changed();
}
void ArcadeLibrary::reload(){
    viewCache.clear();imageCache.clear();viewDirty=true;
    catalog={};if(base.isEmpty()||!QFileInfo(base).isDir())return;
    catalog=readObject(cachePath());if((catalog["format"].toInt()!=1 || catalog["engine"].toString()!="0.241") || QFileInfo(metadataBase+"/catalog.json").lastModified()>QFileInfo(cachePath()).lastModified())catalog=readObject(metadataBase+"/catalog.json");
    if((catalog["format"].toInt()!=1 || catalog["engine"].toString()!="0.241"))catalog={};
}
QVariantMap ArcadeLibrary::settings() const{
    return {{"folder",prefs["folder"].toString()},{"folderUrl",prefs["folder"].toString().isEmpty()?QString():QUrl::fromLocalFile(prefs["folder"].toString()).toString()},
        {"resolvedFolder",base},{"fullscreen",prefs["fullscreen"].toBool(true)},{"artwork",prefs["artwork"].toBool(true)},
        {"variants",catalog["variants"].toInt()},{"verified",catalog["verified"].toInt()},{"version","0.241"}};
}
QVariantList ArcadeLibrary::games() const{
    if(!viewDirty)return viewCache;
    QVariantList result;
    for(const auto &entry:catalog["games"].toArray()){
        auto game=entry.toObject().toVariantMap();const auto id=game["id"].toString();QVariantList variants;
        for(const auto &v:game["variants"].toList()){
            auto variant=v.toMap();
            for(const auto &key:{"image","cover","cabinet","marquee","titleImage","panel"}){
                const auto relative=variant[key].toString();
                if(!imageCache.contains(relative)){const auto path=safeFile(metadataBase,relative);imageCache[relative]=path.isEmpty()?QString():QUrl::fromLocalFile(path).toString();}
                variant[key]=imageCache[relative];
            }
            variants<<variant;
        }
        if(variants.isEmpty())continue;
        QString chosen=prefs["selected"].toObject()[id].toString();QVariantMap selected=variants.first().toMap();
        for(const auto &v:variants)if(v.toMap()["id"].toString()==chosen)selected=v.toMap();
        game["variants"]=variants;game["selected"]=selected;game["image"]=selected["image"];game["cover"]=selected["cover"];
        game["favorite"]=prefs["favorites"].toObject()[id].toBool();game["hidden"]=prefs["hidden"].toObject()[id].toBool();result<<game;
    }
    viewCache=result;viewDirty=false;return result;
}
bool ArcadeLibrary::save(const QJsonObject &next){
    QDir().mkpath(root+"/config");const QString path=root+"/config/arcade.json";
    if(QFileInfo::exists(path)){
        QDir().mkpath(root+"/backups");
        if(!QFile::copy(path,root+"/backups/arcade-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".json")){notify(uiText("Could not save Arcade settings."));return false;}
    }
    QSaveFile f(path);const auto bytes=QJsonDocument(next).toJson();
    if(!f.open(QIODevice::WriteOnly)||f.write(bytes)!=bytes.size()||!f.commit()){notify(uiText("Could not save Arcade settings."));return false;}
    prefs=next;viewDirty=true;emit changed();return true;
}
QString ArcadeLibrary::configure(const QUrl &folder,bool fullscreen,bool artwork){
    if(busy()||(blocked&&blocked()))return uiText("Wait for the current task to finish.");
    QString path=folder.toLocalFile();
    if(!folder.isEmpty()&&(!folder.isLocalFile()||!QFileInfo(path).isDir()))return uiText("Select an existing local folder.");
    if(!path.isEmpty()){path=QDir(path).absolutePath();if(!QFileInfo(path+"/roms").isDir())return uiText("Select a MAME library containing a roms folder.");}
    auto next=prefs;next["folder"]=path;next["fullscreen"]=fullscreen;next["artwork"]=artwork;
    if(!save(next))return message;
    setLibrary(library,metadataOverride);return {};
}
void ArcadeLibrary::choose(const QString &id,const QString &variant){
    for(const auto &g:games())if(g.toMap()["id"].toString()==id)for(const auto &v:g.toMap()["variants"].toList())if(v.toMap()["id"].toString()==variant){
        auto next=prefs;auto selected=next["selected"].toObject();selected[id]=variant;next["selected"]=selected;save(next);return;
    }
}
void ArcadeLibrary::setHidden(const QString &id,bool value){auto next=prefs;auto map=next["hidden"].toObject();map[id]=value;next["hidden"]=map;save(next);}
void ArcadeLibrary::setFavorite(const QString &id,bool value){auto next=prefs;auto map=next["favorites"].toObject();map[id]=value;next["favorites"]=map;save(next);}
bool ArcadeLibrary::ready(){
    if(busy()||(blocked&&blocked())){notify(uiText("Wait for the current task to finish."));return false;}
    if(base.isEmpty()||!QFileInfo(base).isDir()){notify(uiText("Choose your Arcade library folder in Arcade settings."));return false;}
    return true;
}
void ArcadeLibrary::enter(){
    if(busy())return;
    reload();emit changed();
    if(catalog.isEmpty()&&QFileInfo(base).isDir())scan();
    else if(!available())notify(uiText("MAME 0.241 is required for this library."));
}
void ArcadeLibrary::scan(){
    if(!ready())return;
    const auto python=QStandardPaths::findExecutable("python3");
    if(python.isEmpty()){notify(uiText("Python 3 is required to update this catalog."));return;}
    QDir().mkpath(QFileInfo(cachePath()).absolutePath());scanning=true;notify(uiText("Auditing MAME ROMs and reading artwork…"));
    if(!available()){scanning=false;notify(uiText("MAME 0.241 is required for this library."));return;}
    QStringList args{root+"/helpers/mame_index.py","--library",base,"--output",cachePath(),"--mame",executable()};if(metadataBase!=base)args<<"--metadata"<<metadataBase;scanner.start(python,args);
}
QStringList ArcadeLibrary::launchArguments(const QString &game,const QVariantMap &screen) const{
    QStringList args{root+"/helpers/mame_launch.py","--mame",executable(),"--library",base,"--personal",root+"/data/mame/0.241"};
    if(metadataBase!=base)args<<"--metadata"<<metadataBase;
    if(!game.isEmpty())args<<"--game"<<game;
    if(!prefs["fullscreen"].toBool(true))args<<"--window";
    if(!prefs["artwork"].toBool(true))args<<"--crop";
    if(!screen.isEmpty())args<<"--screen"<<QString("screen%1").arg(screen["index"].toInt());
    return args;
}
void ArcadeLibrary::start(const QString &game){
    if(!ready())return;
    if(!available()){notify(uiText("MAME 0.241 is required for this library."));return;}
    const auto python=QStandardPaths::findExecutable("python3");
    if(python.isEmpty()){notify(uiText("Python 3 is required to update this catalog."));return;}
    QDir().mkpath(root+"/data/mame/0.241");QDir().mkpath(root+"/logs");
    process.setWorkingDirectory(root+"/data/mame/0.241");process.setStandardOutputFile(root+"/logs/mame.log",QIODevice::Append);process.setProcessChannelMode(QProcess::MergedChannels);
    const auto screen=monitor?monitor():QVariantMap{};
    auto env=QProcessEnvironment::systemEnvironment();if(!screen.isEmpty())env.insert("SDL_VIDEODRIVER",screen.value("driver","x11").toString());process.setProcessEnvironment(env);
    running=true;notify(uiText("MAME 0.241 · Checking and starting game…"));process.start(python,launchArguments(game,screen));
}
void ArcadeLibrary::play(const QString &id,const QString &variant){
    for(const auto &g:games())if(g.toMap()["id"].toString()==id)for(const auto &v:g.toMap()["variants"].toList())if(v.toMap()["id"].toString()==variant){if(v.toMap()["playable"].toBool())start(v.toMap()["id"].toString());else notify(uiText("This edition has missing or invalid ROMs. Update the catalog for details."));return;}
    notify(uiText("This ROM is unavailable. Update the catalog."));
}
void ArcadeLibrary::manage(){start({});}
void ArcadeLibrary::openFolder(bool personal){const auto path=personal?root+"/data/mame/0.241":base;if(QFileInfo(path).isDir())QDesktopServices::openUrl(QUrl::fromLocalFile(path));}
