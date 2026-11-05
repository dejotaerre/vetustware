#include "i18n.h"
#include "librarylayout.h"
#include "extras.h"
#include "archive.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QSet>
#include <stdexcept>

static void fail(const QString &message){throw std::runtime_error(message.toStdString());}
static bool supported(const QString &name){
    static const QSet<QString> extensions={"pdf","txt","rtf","doc","xls","html","htm","png","jpg","jpeg","jfif","gif","bmp","mp3","ogg","wav","flac","mp2","mp4","avi"};
    return extensions.contains(QFileInfo(name).suffix().toLower());
}
void Extras::load(const QString &path,const QString &metadata,const QString &dosboxMetadata,const QString &dosboxGames){
    dosboxRoot=dosboxMetadata;
    if(dosboxRoot.isEmpty()&&metadata.isEmpty()&&!path.isEmpty()&&QFileInfo(path+"/games/dosbox/metadata").isDir())dosboxRoot=path+"/games/dosbox/metadata";
    gamesRoot=dosboxGames;
    if(gamesRoot.isEmpty()&&!dosboxRoot.isEmpty()&&QFileInfo(dosboxRoot).fileName()=="metadata")gamesRoot=QFileInfo(dosboxRoot).absolutePath();
    metadataRoot=metadata;
    library=QFileInfo(path).absoluteFilePath();games={};
    QFile file(!dosboxRoot.isEmpty()?dosboxRoot+"/extras/extras.json":metadata.isEmpty()?LibraryLayout::path(library,"metadata/dosbox/extras/extras.json","Complementos/extras.json"):metadata+"/dosbox/extras/extras.json");
    if(!file.exists())return;
    if(file.size()>16*1024*1024||!file.open(QIODevice::ReadOnly))fail(uiText("Could not read the extras index."));
    const auto doc=QJsonDocument::fromJson(file.readAll());
    if(!doc.isObject()||doc["format"].toInt()!=1||!doc["games"].isObject())fail(uiText("The extras index is invalid."));
    games=doc["games"].toObject();
}
QVariantList Extras::list(const QString &game) const{
    const QHash<QString,QString> kinds{{"Audio",uiText("Audio")},{"Documento",uiText("Document")},{"Documento de diseño · Inglés",uiText("Design document · English")},{"Imagen",uiText("Image")},{"Música",uiText("Music")},{"Texto",uiText("Text")},{"Vídeo",uiText("Video")}};
    QVariantList result;const auto entries=games[game].toArray();
    for(int i=0;i<entries.size();++i){
        const auto entry=entries[i].toObject();const auto name=entry["name"].toString();
        if(!supported(name))continue;
        result<<QVariantMap{{"index",i},{"name",name},{"kind",kinds.value(entry["kind"].toString(),entry["kind"].toString())},{"bytes",entry["bytes"].toVariant()}};
    }
    return result;
}
QString Extras::prepare(const QString &game,int index,const QString &cache) const{
    const auto entries=games[game].toArray();
    if(index<0||index>=entries.size())fail(uiText("This extra is no longer available."));
    const auto entry=entries[index].toObject();const QString name=entry["name"].toString(),relative=entry["archive"].toString(),member=entry["entry"].toString(),hash=entry["sha256"].toString();
    const qint64 size=entry["bytes"].toVariant().toLongLong();
    if(!supported(name)||name.contains('/')||name.contains('\\')||name.contains(QChar(0))||!QRegularExpression("^[a-f0-9]{64}$").match(hash).hasMatch()||size<0||size>qint64(2)*1024*1024*1024)fail(uiText("The extra's data is invalid."));
    if(relative.isEmpty()||QDir::isAbsolutePath(relative)||relative.contains('\\')||relative.contains(':')||relative.split('/').contains(".."))fail(uiText("The extra's location is invalid."));
    if(member.isEmpty()||member.startsWith('/')||member.contains('\\')||member.contains(QChar(0))||member.split('/').contains("..")||member.section('/',0,0).contains(':'))fail(uiText("The extra's path is invalid."));
    QString relativeArchive=relative,archiveRoot=library;
    if(relative.startsWith("games/dosbox/")&&!gamesRoot.isEmpty()){
        relativeArchive=relative.section('/',2);archiveRoot=gamesRoot;
    }else if(!dosboxRoot.isEmpty()){
        if(!relative.startsWith("metadata/dosbox/")&&!relative.startsWith("metadatos/dosbox/"))fail(uiText("The extra's location is invalid."));
        relativeArchive=relative.section('/',2);archiveRoot=dosboxRoot;
    }else if(!metadataRoot.isEmpty()){
        if(!relative.startsWith("metadata/")&&!relative.startsWith("metadatos/"))fail(uiText("The extra's location is invalid."));
        relativeArchive=relative.section('/',1);archiveRoot=metadataRoot;
    }
    const QString archive=QFileInfo(archiveRoot+'/'+relativeArchive).canonicalFilePath(),base=QFileInfo(archiveRoot).canonicalFilePath();
    if(base.isEmpty()||archive.isEmpty()||!archive.startsWith(base+'/'))fail(uiText("The extras archive was not found in this library."));
    const QString path=QDir(cache).absoluteFilePath(hash+'/'+name);
    if(QFileInfo(QDir(cache).absoluteFilePath(hash)).isSymLink())fail(uiText("The cache folder contains a disallowed link."));
    QFile cached(path);
    if(!QFileInfo(path).isSymLink()&&cached.size()==size&&cached.open(QIODevice::ReadOnly)){
        QCryptographicHash checksum(QCryptographicHash::Sha256);
        if(checksum.addData(&cached)&&checksum.result().toHex()==hash.toLatin1())return path;
        cached.close();
    }
    Archive zip(archive);zip.extractFile(member,path,size,hash.toLatin1());
    return path;
}
