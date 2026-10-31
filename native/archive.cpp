#include "i18n.h"
#include "archive.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QSaveFile>
#include <QCryptographicHash>
#include <stdexcept>

static void fail(const QString &message) { throw std::runtime_error(message.toStdString()); }
Archive::Archive(const QString &path) {
    int error=0;handle=zip_open(QFile::encodeName(path).constData(),ZIP_RDONLY,&error);
    if(!handle)fail(uiText("Could not open the archive: ")+QFileInfo(path).fileName());
}
Archive::~Archive(){if(handle)zip_close(handle);}
QStringList Archive::names() const {
    QStringList result;
    for(zip_int64_t i=0;i<zip_get_num_entries(handle,0);i++)result<<QString::fromUtf8(zip_get_name(handle,i,0));
    return result;
}
void Archive::extractFile(const QString &name,const QString &destination,qint64 size,const QByteArray &sha256){
    const auto parts=name.split('/');
    if(name.isEmpty()||name.startsWith('/')||name.contains('\\')||parts.contains("..")||parts.first().contains(':'))fail(uiText("Invalid extra path."));
    zip_stat_t st;zip_stat_init(&st);
    if(zip_stat(handle,name.toUtf8().constData(),0,&st)!=0||st.size!=quint64(size))fail(uiText("The extra does not match the library index."));
    zip_uint8_t os=0;zip_uint32_t attributes=0;zip_file_get_external_attributes(handle,st.index,0,&os,&attributes);
    if(os==ZIP_OPSYS_UNIX&&((attributes>>16)&0170000)==0120000)fail(uiText("The extra contains a disallowed link."));
    if(!QDir().mkpath(QFileInfo(destination).absolutePath()))fail(uiText("Could not create the extras folder."));
    if(QStorageInfo(QFileInfo(destination).absolutePath()).bytesAvailable()<size+64*1024*1024)fail(uiText("There is not enough space to open this extra."));
    QSaveFile out(destination);if(!out.open(QIODevice::WriteOnly))fail(uiText("Could not save the extra."));
    zip_file_t *input=zip_fopen_index(handle,st.index,0);if(!input)fail(uiText("Could not read the extra."));
    QByteArray buffer(1024*1024,Qt::Uninitialized);QCryptographicHash hash(QCryptographicHash::Sha256);qint64 total=0,n=0;
    while((n=zip_fread(input,buffer.data(),buffer.size()))>0){
        total+=n;
        if(total>size||out.write(buffer.constData(),n)!=n){zip_fclose(input);fail(uiText("Could not finish preparing the extra."));}
        hash.addData(QByteArrayView(buffer.constData(),n));
    }
    const int closed=zip_fclose(input);
    if(n<0||closed!=0||total!=size||hash.result().toHex()!=sha256)fail(uiText("The extra failed its integrity check."));
    if(!out.commit())fail(uiText("Could not save the complete extra."));
}
QByteArray Archive::read(const QString &name){
    zip_stat_t st;zip_stat_init(&st);
    if(zip_stat(handle,name.toUtf8().constData(),0,&st)!=0)fail(uiText("Resource not found: ")+name);
    if(st.size>256*1024*1024)fail(uiText("The resource is too large to load into memory."));
    zip_file_t *f=zip_fopen(handle,name.toUtf8().constData(),0);if(!f)fail(uiText("Could not read: ")+name);
    QByteArray result;result.resize(st.size);zip_uint64_t done=0;
    while(done<st.size){auto n=zip_fread(f,result.data()+done,st.size-done);if(n<=0){zip_fclose(f);fail(uiText("Incomplete read: ")+name);}done+=n;}
    if(zip_fclose(f)!=0)fail(uiText("Integrity error: ")+name);return result;
}
QString Archive::extract(const QString &destination,const QString &expectedRoot,const std::function<void(int)> &progress,bool selectedRootOnly){
    quint64 total=0,done=0;QString actualRoot;QList<zip_int64_t> selected;
    for(zip_int64_t i=0;i<zip_get_num_entries(handle,0);i++){
        zip_stat_t st;zip_stat_index(handle,i,0,&st);
        const QString name=QString::fromUtf8(st.name).replace('\\','/');const QStringList parts=name.split('/');
        const bool extraSigil=expectedRoot=="Sigil" && parts.first()=="Sigil (2019)";
        if(name.startsWith('/')||parts.contains("..")||parts.first().contains(':'))fail(uiText("Unexpected path inside the ZIP: ")+name);
        if(parts.first().compare(expectedRoot,Qt::CaseInsensitive)!=0&&!extraSigil){
            if(selectedRootOnly)continue;
            fail(uiText("Unexpected path inside the ZIP: ")+name);
        }
        selected<<i;total+=st.size;
        if(!extraSigil){
            if(actualRoot.isEmpty())actualRoot=parts.first();
            if(actualRoot!=parts.first())fail(uiText("The ZIP contains incompatible root folders."));
        }
        zip_uint8_t os=0;zip_uint32_t attributes=0;zip_file_get_external_attributes(handle,i,0,&os,&attributes);
        if(os==ZIP_OPSYS_UNIX && ((attributes>>16)&0170000)==0120000)fail(uiText("Disallowed link inside the ZIP."));
    }
    if(selected.isEmpty())fail(uiText("The game was not found in the ZIP: ")+expectedRoot);
    QDir().mkpath(destination);
    if(QStorageInfo(destination).bytesAvailable()<qint64(total+512*1024*1024))fail(uiText("There is not enough space to install this game."));
    QByteArray buffer(1024*1024,Qt::Uninitialized);int last=-1;
    for(zip_int64_t i:selected){
        zip_stat_t st;zip_stat_index(handle,i,0,&st);QString name=QString::fromUtf8(st.name).replace('\\','/');
        const QString path=destination+'/'+name;
        if(name.endsWith('/')){QDir().mkpath(path);continue;}
        QDir().mkpath(QFileInfo(path).absolutePath());QFile out(path);
        if(!out.open(QIODevice::WriteOnly))fail(uiText("Cannot write the game files."));
        zip_file_t *file=zip_fopen_index(handle,i,0);if(!file)fail(uiText("Cannot read a game file."));
        zip_int64_t bytes;
        while((bytes=zip_fread(file,buffer.data(),buffer.size()))>0){
            if(out.write(buffer.constData(),bytes)!=bytes){zip_fclose(file);fail(uiText("Could not finish writing the files."));}
            done+=bytes;int percent=int(done*100/qMax<quint64>(1,total));if(percent!=last){progress(percent);last=percent;}
        }
        int closed=zip_fclose(file);if(bytes<0||closed!=0)fail(uiText("The ZIP failed its integrity check."));
    }
    return actualRoot;
}
QString resolveCase(const QString &path){
    if(QFileInfo::exists(path))return QDir::cleanPath(path);
    QString current="/";
    for(const QString &part:QDir::cleanPath(QFileInfo(path).absoluteFilePath()).split('/',Qt::SkipEmptyParts)){
        QString found=part;
        for(const QString &candidate:QDir(current).entryList(QDir::AllEntries|QDir::NoDotAndDotDot))
            if(candidate.compare(part,Qt::CaseInsensitive)==0){found=candidate;break;}
        current=QDir(current).filePath(found);
    }
    return current;
}
