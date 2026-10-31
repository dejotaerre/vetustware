#include "magazines.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QUrl>
#include <stdexcept>

static void fail(const QString &text){throw std::runtime_error(text.toStdString());}
void Magazines::load(const QString &library){
    base=library+"/Revistas";catalog={};loadError.clear();
    QFile f(base+"/revistas.json");if(!f.exists())return;
    if(f.size()>4*1024*1024||!f.open(QIODevice::ReadOnly)){loadError="No se puede leer el catálogo de revistas.";return;}
    const auto doc=QJsonDocument::fromJson(f.readAll());
    if(!doc.isObject()||doc["format"].toInt()!=1||!doc["series"].isArray()||!doc["entries"].isArray()){
        loadError="El catálogo de revistas no es válido.";return;
    }
    catalog=doc.object();
}
QString Magazines::resolve(const QString &relative) const{
    if(relative.isEmpty()||QDir::isAbsolutePath(relative)||relative.contains('\\')||relative.contains(':')||relative.contains(QChar(0))||relative.split('/').contains(".."))return {};
    const QString root=QFileInfo(base).canonicalFilePath(),path=QFileInfo(base+'/'+relative).canonicalFilePath();
    return !root.isEmpty()&&path.startsWith(root+'/')?path:QString();
}
QVariantList Magazines::series() const{
    QVariantList result;
    for(auto v:catalog["series"].toArray()){
        auto m=v.toObject().toVariantMap();const QString cover=resolve(m["cover"].toString());
        m["cover"]=cover.isEmpty()?QUrl():QUrl::fromLocalFile(cover);result<<m;
    }
    return result;
}
QVariantList Magazines::issues(const QString &series) const{
    QVariantList result;
    for(auto v:catalog["entries"].toArray()){
        const auto e=v.toObject();if(e["series"]!=series||e["isIndex"].toBool())continue;
        auto m=e.toVariantMap();const QString cover=resolve(e["cover"].toString());
        m["cover"]=cover.isEmpty()?QUrl():QUrl::fromLocalFile(cover);result<<m;
    }
    return result;
}
QJsonObject Magazines::entry(const QString &id) const{
    if(!QRegularExpression("^[A-Za-z0-9_-]{1,80}$").match(id).hasMatch())fail("Identificador de revista inválido.");
    for(auto v:catalog["entries"].toArray())if(v.toObject()["id"]==id)return v.toObject();
    fail("Esta revista ya no está disponible.");return {};
}
QString Magazines::pdf(const QString &id) const{
    const auto e=entry(id);const QString path=resolve(e["path"].toString());
    if(e["kind"]!="pdf"||!QFileInfo(path).isFile()||QFileInfo(path).suffix().compare("pdf",Qt::CaseInsensitive)!=0)fail("No se encuentra la revista. Comprueba que el disco de la biblioteca esté conectado.");
    return path;
}
QString Magazines::prepareDos(const QString &id,const QString &workingRoot) const{
    const auto e=entry(id);const QString relative=e["path"].toString(),revision=e["revision"].toString();
    if(e["kind"]!="dos"||(relative!="BBD"&&relative!="GameBytes")||!QRegularExpression("^[a-f0-9]{64}$").match(revision).hasMatch())fail("La revista DOS no tiene un perfil válido.");
    const QString source=resolve(relative);
    if(!QFileInfo(source+"/run.bat").isFile()||source.isEmpty())fail("Faltan los archivos de la revista DOS. Comprueba el disco de la biblioteca.");
    if(QFileInfo(workingRoot).isSymLink()||!QDir().mkpath(workingRoot))fail("No se puede preparar la carpeta local de revistas.");
    const QString target=workingRoot+'/'+relative+'-'+revision.left(16);
    if(QFileInfo(target).isSymLink())fail("La copia local de revistas contiene un enlace no permitido.");
    if(QFileInfo(target+"/.ready").isFile()&&QFileInfo(target+"/run.bat").isFile())return target;
    if(QFileInfo::exists(target))fail("La copia local de la revista está incompleta. Consérvala aparte antes de volver a prepararla.");
    QTemporaryDir staging(workingRoot+"/.preparando-XXXXXX");if(!staging.isValid())fail("No se pudo crear la copia de trabajo de la revista.");
    // DOS escribe únicamente en la copia local; la biblioteca compartida queda intacta.
    QDirIterator it(source,QDir::AllEntries|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while(it.hasNext()){
        it.next();const auto info=it.fileInfo();const QString destination=staging.path()+'/'+QDir(source).relativeFilePath(info.filePath());
        if(info.isSymLink())fail("La revista contiene un enlace no permitido.");
        if(info.isDir()){if(!QDir().mkpath(destination))fail("No se pudo crear una carpeta de la revista.");}
        else if(!info.isFile()||!QDir().mkpath(QFileInfo(destination).absolutePath())||!QFile::copy(info.filePath(),destination))fail("No se pudo copiar la revista. Comprueba el espacio disponible.");
    }
    QFile marker(staging.path()+"/.ready");if(!marker.open(QIODevice::WriteOnly)||marker.write(revision.toUtf8())!=revision.size())fail("No se pudo completar la copia de la revista.");marker.close();
    if(!QDir().rename(staging.path(),target))fail("No se pudo guardar la copia de la revista.");
    return target;
}
