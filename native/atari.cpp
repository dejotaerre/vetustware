#include "atari.h"
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
AtariLibrary::AtariLibrary(QString r,QObject *parent):QObject(parent),root(r){
    prefs=readObject(root+"/config/atari.json");
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){running=false;notify(uiText("Could not start Stella: ")+process.errorString());});
    connect(&process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        running=false;notify(code==0&&status==QProcess::NormalExit?uiText("Stella closed · Your saves have been kept"):uiText("Stella ended with an error. Check logs/stella.log."));
    });
    connect(&scanner,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){scanning=false;notify(uiText("Could not update the Atari catalog: ")+scanner.errorString());});
    connect(&scanner,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){
        scanning=false;
        if(code!=0||status!=QProcess::NormalExit){notify(uiText("Could not update the Atari catalog: ")+QString::fromUtf8(scanner.readAllStandardError()).right(1500));return;}
        reload();notify(uiText("Atari catalog updated"));
    });
}
void AtariLibrary::notify(const QString &text){message=text;emit changed();}
QString AtariLibrary::executable() const{
    const QString bundled=root+"/runtime/stella/stella";
    return QFileInfo(bundled).isExecutable()?bundled:QStandardPaths::findExecutable("stella");
}
QString AtariLibrary::safeFile(const QString &base,const QString &relative){
    if(relative.isEmpty()||QDir::isAbsolutePath(relative))return {};
    const auto folder=QFileInfo(base).canonicalFilePath();
    const QFileInfo file(QDir(base).filePath(relative));const auto canonical=file.canonicalFilePath();
    return !folder.isEmpty()&&file.isFile()&&canonical.startsWith(folder+"/")?canonical:QString();
}
QString AtariLibrary::cachePath() const{return root+"/data/atari/"+QString::fromLatin1(QCryptographicHash::hash((base+"\n"+metadataBase).toUtf8(),QCryptographicHash::Sha256).toHex())+".json";}
void AtariLibrary::setLibrary(const QString &path,const QString &metadata){
    metadataOverride=metadata;
    prefs=readObject(root+"/config/atari.json");
    library=path;const QString selected=prefs["folder"].toString();
    const auto next=selected.isEmpty()?(library.isEmpty()?QString():LibraryLayout::games(library,"atari2600")):selected;
    base=next;metadataBase=QFileInfo(base+"/metadata").isDir()?base+"/metadata":metadata.isEmpty()?LibraryLayout::metadata(library,"atari2600",base):metadata;reload();emit changed();
}
void AtariLibrary::reload(){
    catalog={};if(base.isEmpty()||!QFileInfo(base).isDir())return;
    catalog=readObject(cachePath());if(catalog["format"].toInt()!=1 || QFileInfo(metadataBase+"/catalog.json").lastModified()>QFileInfo(cachePath()).lastModified())catalog=readObject(metadataBase+"/catalog.json");
    if(catalog["format"].toInt()!=1)catalog={};
}
QVariantMap AtariLibrary::settings() const{
    return {{"folder",prefs["folder"].toString()},{"folderUrl",prefs["folder"].toString().isEmpty()?QString():QUrl::fromLocalFile(prefs["folder"].toString()).toString()},{"resolvedFolder",base},{"fullscreen",prefs["fullscreen"].toBool(true)},{"crt",prefs["crt"].toBool(false)},
        {"romFiles",catalog["romFiles"].toInt()},{"uniqueRoms",catalog["uniqueRoms"].toInt()},{"legacyStates",QFileInfo(base+"/legacy/stella-3.2.2/states").isDir()}};
}
QVariantList AtariLibrary::games() const{
    QVariantList result;
    for(const auto &entry:catalog["games"].toArray()){
        auto game=entry.toObject().toVariantMap();const auto id=game["id"].toString();QVariantList variants;
        for(const auto &v:game["variants"].toList()){
            auto variant=v.toMap();QString path;
            for(const auto &file:variant["files"].toStringList()){path=safeFile(base,file);if(!path.isEmpty())break;}
            if(path.isEmpty())continue;
            variant["path"]=path;const auto image=safeFile(metadataBase,variant["image"].toString());
            variant["image"]=image.isEmpty()?QString():QUrl::fromLocalFile(image).toString();
            const auto cover=safeFile(metadataBase,variant["cover"].toString());variant["cover"]=cover.isEmpty()?QString():QUrl::fromLocalFile(cover).toString();variants<<variant;
        }
        if(variants.isEmpty())continue;
        QString chosen=prefs["selected"].toObject()[id].toString();QVariantMap selected=variants.first().toMap();
        for(const auto &v:variants)if(v.toMap()["id"].toString()==chosen)selected=v.toMap();
        game["variants"]=variants;game["selected"]=selected;game["image"]=selected["image"];game["cover"]=selected["cover"];
        game["favorite"]=prefs["favorites"].toObject()[id].toBool();game["hidden"]=prefs["hidden"].toObject()[id].toBool();result<<game;
    }
    return result;
}
bool AtariLibrary::save(const QJsonObject &next){
    QDir().mkpath(root+"/config");const QString path=root+"/config/atari.json";
    if(QFileInfo::exists(path)){
        QDir().mkpath(root+"/backups");
        if(!QFile::copy(path,root+"/backups/atari-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".json")){notify(uiText("Could not save Atari settings."));return false;}
    }
    QSaveFile f(path);const auto bytes=QJsonDocument(next).toJson();
    if(!f.open(QIODevice::WriteOnly)||f.write(bytes)!=bytes.size()||!f.commit()){notify(uiText("Could not save Atari settings."));return false;}
    prefs=next;emit changed();return true;
}
QString AtariLibrary::configure(const QUrl &folder,bool fullscreen,bool crt){
    if(busy()||(blocked&&blocked()))return uiText("Wait for the current task to finish.");
    QString path=folder.toLocalFile();
    if(!folder.isEmpty()&&(!folder.isLocalFile()||!QFileInfo(path).isDir()))return uiText("Select an existing local folder.");
    if(!path.isEmpty())path=QDir(path).absolutePath();
    auto next=prefs;next["folder"]=path;next["fullscreen"]=fullscreen;next["crt"]=crt;
    if(!save(next))return message;
    setLibrary(library,metadataOverride);return {};
}
void AtariLibrary::choose(const QString &id,const QString &variant){
    for(const auto &g:games())if(g.toMap()["id"].toString()==id)for(const auto &v:g.toMap()["variants"].toList())if(v.toMap()["id"].toString()==variant){
        auto next=prefs;auto selected=next["selected"].toObject();selected[id]=variant;next["selected"]=selected;save(next);return;
    }
}
void AtariLibrary::setHidden(const QString &id,bool value){auto next=prefs;auto map=next["hidden"].toObject();map[id]=value;next["hidden"]=map;save(next);}
void AtariLibrary::setFavorite(const QString &id,bool value){auto next=prefs;auto map=next["favorites"].toObject();map[id]=value;next["favorites"]=map;save(next);}
bool AtariLibrary::ready(){
    if(busy()||(blocked&&blocked())){notify(uiText("Wait for the current task to finish."));return false;}
    if(base.isEmpty()||!QFileInfo(base).isDir()){notify(uiText("Choose your Atari library folder in Atari settings."));return false;}
    return true;
}
void AtariLibrary::enter(){
    if(busy())return;
    reload();emit changed();
    if(catalog.isEmpty()&&QFileInfo(base).isDir())scan();
    else if(!available())notify(uiText("Install Stella to play Atari 2600 games."));
}
void AtariLibrary::scan(){
    if(!ready())return;
    const auto python=QStandardPaths::findExecutable("python3");
    if(python.isEmpty()){notify(uiText("Python 3 is required to update this catalog."));return;}
    QDir().mkpath(QFileInfo(cachePath()).absolutePath());scanning=true;notify(uiText("Reading Atari ROMs and images…"));
    QStringList args{root+"/helpers/atari_index.py","--library",base,"--output",cachePath()};
    if(available())args<<"--stella"<<executable();if(metadataBase!=base)args<<"--metadata"<<metadataBase;scanner.start(python,args);
}
QStringList AtariLibrary::launchArguments(const QString &rom,const QVariantMap &screen) const{
    const auto personal=root+"/data/stella";
    QStringList args{"-basedir",personal,"-userdir",personal,"-saveuserdir","0","-fullscreen",prefs["fullscreen"].toBool(true)?"1":"0",
        "-tia.correct_aspect","1","-tia.fs_stretch","0","-tia.inter","0","-tv.filter",prefs["crt"].toBool()?"3":"0","-tv.scanlines",prefs["crt"].toBool()?"20":"0",
        "-snaploaddir",metadataBase+"/snapshots","-snapsavedir",personal+"/snapshots","-romdir",QFileInfo(base+"/roms").isDir()?base+"/roms":base,"-exitlauncher",rom.isEmpty()?"1":"0"};
    if(!screen.isEmpty())args<<"-display"<<QString::number(screen["index"].toInt())<<"-launcherdisplay"<<QString::number(screen["index"].toInt());
    if(!rom.isEmpty())args<<rom;return args;
}
void AtariLibrary::start(const QString &rom){
    if(!ready())return;
    if(!available()){notify(uiText("Install Stella to play Atari 2600 games."));return;}
    QDir().mkpath(root+"/data/stella/snapshots");QDir().mkpath(root+"/logs");
    process.setWorkingDirectory(root+"/data/stella");process.setStandardOutputFile(root+"/logs/stella.log",QIODevice::Append);process.setProcessChannelMode(QProcess::MergedChannels);
    const auto screen=monitor?monitor():QVariantMap{};
    auto env=QProcessEnvironment::systemEnvironment();if(!screen.isEmpty())env.insert("SDL_VIDEODRIVER",screen.value("driver","x11").toString());process.setProcessEnvironment(env);
    running=true;notify(uiText("Stella is running"));process.start(executable(),launchArguments(rom,screen));
}
void AtariLibrary::play(const QString &id,const QString &variant){
    for(const auto &g:games())if(g.toMap()["id"].toString()==id)for(const auto &v:g.toMap()["variants"].toList())if(v.toMap()["id"].toString()==variant){start(v.toMap()["path"].toString());return;}
    notify(uiText("This ROM is unavailable. Update the catalog."));
}
void AtariLibrary::manage(){start({});}
void AtariLibrary::openFolder(bool legacy){const auto path=legacy?base+"/legacy/stella-3.2.2/states":base;if(QFileInfo(path).isDir())QDesktopServices::openUrl(QUrl::fromLocalFile(path));}
