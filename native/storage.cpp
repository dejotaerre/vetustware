#include "i18n.h"
#include "librarylayout.h"
#include "storage.h"
#include <QUuid>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QMap>
#include <QJsonDocument>
#include <QSaveFile>
#include <QDateTime>
#include <QTemporaryFile>
#include <stdexcept>

Storage::Storage(const QString &r):root(r){
    QFile f(root+"/config/storage.json");
    if(f.exists()){
        if(!f.open(QIODevice::ReadOnly))throw std::runtime_error(uiText("Cannot read the configured folders.").toStdString());
        auto doc=QJsonDocument::fromJson(f.readAll());
        if(!doc.isObject())throw std::runtime_error(uiText("The folder configuration is invalid.").toStdString());
        data=doc.object();
    }
    library=data["library"].toString();
    games=data["games"].toString();
    if(games.isEmpty())games=root+"/data/games";
}
QString Storage::expand(QString text) const{
    text.replace("${GAMES}",games);text.replace("${LIBRARY}",library);text.replace("${APP}",root);
    for(const auto &value:data["previousGameRoots"].toArray()){
        const QString old=value.toString();
        if(!old.isEmpty()){
            if(text==old)text=games;
            else text.replace(old+'/',games+'/');
        }
    }
    return text;
}
QString Storage::metadataRoot() const{
    return data.contains("metadata")?data["metadata"].toString():LibraryLayout::organized(library)?LibraryLayout::path(library,"metadata",""):QString();
}
QString Storage::metadataDirectory(const QString &engine) const{
    QString base=data[engine].toString();
    if(engine=="atari2600"||engine=="mame"){
        QFile prefs(root+"/config/"+(engine=="mame"?"arcade":"atari")+".json");
        if(prefs.open(QIODevice::ReadOnly))base=QJsonDocument::fromJson(prefs.readAll()).object()["folder"].toString();
    }
    if(base.isEmpty()&&!library.isEmpty())base=LibraryLayout::games(library,engine);
    if(!base.isEmpty()&&(data["embeddedMetadata"].toBool()||QFileInfo(base+"/metadata").isDir()))return base+"/metadata";
    const auto shared=metadataRoot();return shared.isEmpty()?QString():shared+'/'+engine;
}
bool Storage::organized() const{return data.contains("metadata")||data["embeddedMetadata"].toBool()||LibraryLayout::organized(library);}
QString Storage::sharedPath(const QString &current,const QString &legacy) const{
    const auto kind=current.section('/',0,0);
    if(kind=="metadata"){
        const auto base=metadataDirectory(current.section('/',1,1));
        if(!base.isEmpty())return base+'/'+current.section('/',2);
    }
    if(kind=="resources"&&(current.section('/',1,1)=="audio"||current.section('/',1,1)=="dos")){
        QString base=data["dosbox"].toString();
        if(base.isEmpty()&&LibraryLayout::organized(library))base=LibraryLayout::games(library,"dosbox");
        if(!base.isEmpty()&&(data["embeddedResources"].toBool()||QFileInfo(base+"/resources").isDir()))
            return base+"/resources/"+current.section('/',1);
    }
    if(data.contains(kind)){
        const auto base=data[kind].toString();
        return base.isEmpty()?QString():base+'/'+current.section('/',1);
    }
    if(library.isEmpty())return {};
    return LibraryLayout::path(library,current,legacy);
}
QString Storage::dosboxPath(const QString &relative) const{
    const auto custom=data["dosbox"].toString();
    if(!custom.isEmpty())return custom+'/'+relative;
    if(library.isEmpty())return {};
    return LibraryLayout::path(library,"games/dosbox/"+relative,relative=="archives"?"eXo/eXoDOS":"Personalizaciones/juegos.zip");
}
QString Storage::validate(const QString &library,const QString &games,const QString &dosbox){
    for(const auto &path:{library,games}){
        if(!QDir::isAbsolutePath(path)||path.contains('"')||path.contains('\n')||path.contains('\r'))return uiText("Select folders using absolute paths, without quotes or line breaks.");
        if(!QFileInfo(path).isDir())return uiText("The folder is unavailable: ")+path;
    }
    if(!QFileInfo(LibraryLayout::path(library,"metadata/dosbox/recursos.zip","Content/XODOSMetadata.zip")).isFile()||!QFileInfo(LibraryLayout::path(library,"metadata/dosbox/perfiles-originales.zip","Content/!DOSmetadata_linux.zip")).isFile()||!QFileInfo(dosbox.isEmpty()?LibraryLayout::path(library,"games/dosbox/archives","eXo/eXoDOS"):dosbox+"/archives").isDir())return uiText("Select the VETUSTware library containing games and metadata.");
    const QString source=QFileInfo(library).canonicalFilePath(),target=QFileInfo(games).canonicalFilePath();
    if(target==source||target.startsWith(source+'/'))return uiText("Installed games must be outside the original collection.");
    QTemporaryFile probe(games+"/.vetust-write-XXXXXX");
    if(!probe.open())return uiText("Cannot write to the installed games folder.");
    return {};
}
QString Storage::save(const QString &lib,const QString &installed,const QString &scummvm,const QJsonObject &locations) const{
    auto next=data;auto previous=next["previousGameRoots"].toArray();
    if(!games.isEmpty()&&games!=installed&&!previous.contains(games))previous.append(games);
    if(scummvm.isEmpty())next.remove("scummvm");else next["scummvm"]=QDir::cleanPath(scummvm);
    for(const QString key:{"dosbox","magazines","metadata","resources"})if(locations.contains(key)){
        const auto value=locations[key].toString();
        if(value.isEmpty()&&!key.startsWith("metadata")&&key!="resources")next.remove(key);else next[key]=value.isEmpty()?QString():QDir::cleanPath(value);
    }
    if(locations["embeddedResources"].toBool()){next["embeddedResources"]=true;next.remove("resources");}
    if(locations["embeddedMetadata"].toBool()){
        next["embeddedMetadata"]=true;next.remove("metadata");
        for(const auto &engine:{"dosbox","scummvm","atari2600","mame"})next.remove("metadata_"+QString(engine));
    }
    if(lib.isEmpty())next.remove("library");else next["library"]=QDir::cleanPath(lib);next["games"]=QDir::cleanPath(installed);next["previousGameRoots"]=previous;
    if(!QDir().mkpath(root+"/config"))return uiText("Could not create the configuration folder.");
    // Se preparan todos los documentos antes de reemplazarlos y se recuperan si falla un guardado.
    QMap<QString,QByteArray> documents,originals;
    documents[root+"/config/storage.json"]=QJsonDocument(next).toJson();
    for(const auto &pair:{qMakePair(QString("atari2600"),QString("atari")),qMakePair(QString("mame"),QString("arcade"))}){
        if(!locations.contains(pair.first))continue;
        const auto path=root+"/config/"+pair.second+".json";
        QFile file(path);QJsonObject prefs;
        if(file.exists()){
            if(!file.open(QIODevice::ReadOnly))return uiText("Cannot read the configured folders.");
            const auto doc=QJsonDocument::fromJson(file.readAll());
            if(!doc.isObject())return uiText("The folder configuration is invalid.");
            prefs=doc.object();
        }
        prefs["folder"]=locations[pair.first].toString();
        documents[path]=QJsonDocument(prefs).toJson();
    }
    const auto backup=root+"/backups/"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    for(auto it=documents.begin();it!=documents.end();++it){
        QFile file(it.key());if(!file.exists())continue;
        if(!file.open(QIODevice::ReadOnly))return uiText("Cannot read the configured folders.");
        originals[it.key()]=file.readAll();
        if(!QDir().mkpath(backup)||!QFile::copy(it.key(),backup+'/'+QFileInfo(it.key()).fileName()))return uiText("Could not back up the folder configuration.");
    }
    QStringList committed;
    for(auto it=documents.begin();it!=documents.end();++it){
        QSaveFile file(it.key());
        if(!file.open(QIODevice::WriteOnly)||file.write(it.value())!=it.value().size()||!file.commit()){
            bool restored=true;
            for(const auto &path:committed){
                if(!originals.contains(path)){restored=QFile::remove(path)&&restored;continue;}
                QSaveFile restore(path);const auto bytes=originals[path];
                restored=(restore.open(QIODevice::WriteOnly)&&restore.write(bytes)==bytes.size()&&restore.commit())&&restored;
            }
            return restored?uiText("Could not save the folders."):uiText("Could not restore all folder settings. Recovery copy: ")+backup;
        }
        committed<<it.key();
    }
    return {};
}
