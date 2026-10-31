#include "i18n.h"
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
    if(f.size()>4*1024*1024||!f.open(QIODevice::ReadOnly)){loadError="Cannot read the magazine catalog.";return;}
    const auto doc=QJsonDocument::fromJson(f.readAll());
    if(!doc.isObject()||doc["format"].toInt()!=1||!doc["series"].isArray()||!doc["entries"].isArray()){
        loadError="The magazine catalog is invalid.";return;
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
    if(!QRegularExpression("^[A-Za-z0-9_-]{1,80}$").match(id).hasMatch())fail(uiText("Invalid magazine identifier."));
    for(auto v:catalog["entries"].toArray())if(v.toObject()["id"]==id)return v.toObject();
    fail(uiText("This magazine is no longer available."));return {};
}
QString Magazines::pdf(const QString &id) const{
    const auto e=entry(id);const QString path=resolve(e["path"].toString());
    if(e["kind"]!="pdf"||!QFileInfo(path).isFile()||QFileInfo(path).suffix().compare("pdf",Qt::CaseInsensitive)!=0)fail(uiText("The magazine was not found. Check that the library drive is connected."));
    return path;
}
QString Magazines::prepareDos(const QString &id,const QString &workingRoot) const{
    const auto e=entry(id);const QString relative=e["path"].toString(),revision=e["revision"].toString();
    if(e["kind"]!="dos"||(relative!="BBD"&&relative!="GameBytes")||!QRegularExpression("^[a-f0-9]{64}$").match(revision).hasMatch())fail(uiText("The DOS magazine does not have a valid profile."));
    const QString source=resolve(relative);
    if(!QFileInfo(source+"/run.bat").isFile()||source.isEmpty())fail(uiText("The DOS magazine files are missing. Check the library drive."));
    if(QFileInfo(workingRoot).isSymLink()||!QDir().mkpath(workingRoot))fail(uiText("Cannot prepare the local magazines folder."));
    const QString target=workingRoot+'/'+relative+'-'+revision.left(16);
    if(QFileInfo(target).isSymLink())fail(uiText("The local magazine copy contains a disallowed link."));
    if(QFileInfo(target+"/.ready").isFile()&&QFileInfo(target+"/run.bat").isFile())return target;
    if(QFileInfo::exists(target))fail(uiText("The local magazine copy is incomplete. Keep it separately before preparing it again."));
    QTemporaryDir staging(workingRoot+"/.preparando-XXXXXX");if(!staging.isValid())fail(uiText("Could not create the magazine's working copy."));
    // DOS escribe únicamente en la copia local; la biblioteca compartida queda intacta.
    QDirIterator it(source,QDir::AllEntries|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
    while(it.hasNext()){
        it.next();const auto info=it.fileInfo();const QString destination=staging.path()+'/'+QDir(source).relativeFilePath(info.filePath());
        if(info.isSymLink())fail(uiText("The magazine contains a disallowed link."));
        if(info.isDir()){if(!QDir().mkpath(destination))fail(uiText("Could not create a magazine folder."));}
        else if(!info.isFile()||!QDir().mkpath(QFileInfo(destination).absolutePath())||!QFile::copy(info.filePath(),destination))fail(uiText("Could not copy the magazine. Check the available space."));
    }
    QFile marker(staging.path()+"/.ready");if(!marker.open(QIODevice::WriteOnly)||marker.write(revision.toUtf8())!=revision.size())fail(uiText("Could not complete the magazine copy."));marker.close();
    if(!QDir().rename(staging.path(),target))fail(uiText("Could not save the magazine copy."));
    return target;
}
