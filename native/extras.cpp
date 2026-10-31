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
void Extras::load(const QString &path){
    library=QFileInfo(path).absoluteFilePath();games={};
    QFile file(library+"/Complementos/extras.json");
    if(!file.exists())return;
    if(file.size()>16*1024*1024||!file.open(QIODevice::ReadOnly))fail("No se pudo leer el índice de extras.");
    const auto doc=QJsonDocument::fromJson(file.readAll());
    if(!doc.isObject()||doc["format"].toInt()!=1||!doc["games"].isObject())fail("El índice de extras no es válido.");
    games=doc["games"].toObject();
}
QVariantList Extras::list(const QString &game) const{
    QVariantList result;const auto entries=games[game].toArray();
    for(int i=0;i<entries.size();++i){
        const auto entry=entries[i].toObject();const auto name=entry["name"].toString();
        if(!supported(name))continue;
        result<<QVariantMap{{"index",i},{"name",name},{"kind",entry["kind"].toString()},{"bytes",entry["bytes"].toVariant()}};
    }
    return result;
}
QString Extras::prepare(const QString &game,int index,const QString &cache) const{
    const auto entries=games[game].toArray();
    if(index<0||index>=entries.size())fail("Ese extra ya no está disponible.");
    const auto entry=entries[index].toObject();const QString name=entry["name"].toString(),relative=entry["archive"].toString(),member=entry["entry"].toString(),hash=entry["sha256"].toString();
    const qint64 size=entry["bytes"].toVariant().toLongLong();
    if(!supported(name)||name.contains('/')||name.contains('\\')||name.contains(QChar(0))||!QRegularExpression("^[a-f0-9]{64}$").match(hash).hasMatch()||size<0||size>qint64(2)*1024*1024*1024)fail("Los datos del extra no son válidos.");
    if(relative.isEmpty()||QDir::isAbsolutePath(relative)||relative.contains('\\')||relative.contains(':')||relative.split('/').contains(".."))fail("La ubicación del extra no es válida.");
    if(member.isEmpty()||member.startsWith('/')||member.contains('\\')||member.contains(QChar(0))||member.split('/').contains("..")||member.section('/',0,0).contains(':'))fail("La ruta del extra no es válida.");
    const QString archive=QFileInfo(library+'/'+relative).canonicalFilePath(),base=QFileInfo(library).canonicalFilePath();
    if(base.isEmpty()||archive.isEmpty()||!archive.startsWith(base+'/'))fail("No se encuentra el archivo de extras en esta biblioteca.");
    const QString path=QDir(cache).absoluteFilePath(hash+'/'+name);
    if(QFileInfo(QDir(cache).absoluteFilePath(hash)).isSymLink())fail("La carpeta de caché contiene un enlace no permitido.");
    QFile cached(path);
    if(!QFileInfo(path).isSymLink()&&cached.size()==size&&cached.open(QIODevice::ReadOnly)){
        QCryptographicHash checksum(QCryptographicHash::Sha256);
        if(checksum.addData(&cached)&&checksum.result().toHex()==hash.toLatin1())return path;
        cached.close();
    }
    Archive zip(archive);zip.extractFile(member,path,size,hash.toLatin1());
    return path;
}
