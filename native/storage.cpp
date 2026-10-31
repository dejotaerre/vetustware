#include "i18n.h"
#include "storage.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
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
QString Storage::validate(const QString &library,const QString &games){
    for(const auto &path:{library,games}){
        if(!QDir::isAbsolutePath(path)||path.contains('"')||path.contains('\n')||path.contains('\r'))return uiText("Select folders using absolute paths, without quotes or line breaks.");
        if(!QFileInfo(path).isDir())return uiText("The folder is unavailable: ")+path;
    }
    if(!QFileInfo(library+"/Content/XODOSMetadata.zip").isFile()||!QFileInfo(library+"/Content/!DOSmetadata_linux.zip").isFile()||!QFileInfo(library+"/eXo/eXoDOS").isDir())return uiText("Select the root of an eXoDOS collection containing Content and eXo/eXoDOS.");
    const QString source=QFileInfo(library).canonicalFilePath(),target=QFileInfo(games).canonicalFilePath();
    if(target==source||target.startsWith(source+'/'))return uiText("Installed games must be outside the original collection.");
    QTemporaryFile probe(games+"/.vetust-write-XXXXXX");
    if(!probe.open())return uiText("Cannot write to the installed games folder.");
    return {};
}
QString Storage::save(const QString &lib,const QString &installed) const{
    auto next=data;auto previous=next["previousGameRoots"].toArray();
    if(!games.isEmpty()&&games!=installed&&!previous.contains(games))previous.append(games);
    next["library"]=QDir::cleanPath(lib);next["games"]=QDir::cleanPath(installed);next["previousGameRoots"]=previous;
    if(!QDir().mkpath(root+"/config"))return uiText("Could not create the configuration folder.");
    const QString path=root+"/config/storage.json";
    if(QFileInfo::exists(path)){
        const QString backup=root+"/backups/"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")+"-storage.json";
        if(!QDir().mkpath(root+"/backups")||!QFile::copy(path,backup))return uiText("Could not back up the folder configuration.");
    }
    QSaveFile file(path);auto bytes=QJsonDocument(next).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())return uiText("Could not save the folders.");
    return {};
}
