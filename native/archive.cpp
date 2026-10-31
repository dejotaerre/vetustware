#include "archive.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <stdexcept>

static void fail(const QString &message) { throw std::runtime_error(message.toStdString()); }
Archive::Archive(const QString &path) {
    int error=0;handle=zip_open(QFile::encodeName(path).constData(),ZIP_RDONLY,&error);
    if(!handle)fail("No se pudo abrir el archivo: "+QFileInfo(path).fileName());
}
Archive::~Archive(){if(handle)zip_close(handle);}
QStringList Archive::names() const {
    QStringList result;
    for(zip_int64_t i=0;i<zip_get_num_entries(handle,0);i++)result<<QString::fromUtf8(zip_get_name(handle,i,0));
    return result;
}
QByteArray Archive::read(const QString &name){
    zip_stat_t st;zip_stat_init(&st);
    if(zip_stat(handle,name.toUtf8().constData(),0,&st)!=0)fail("Recurso no encontrado: "+name);
    if(st.size>256*1024*1024)fail("El recurso es demasiado grande para cargarlo en memoria.");
    zip_file_t *f=zip_fopen(handle,name.toUtf8().constData(),0);if(!f)fail("No se pudo leer "+name);
    QByteArray result;result.resize(st.size);zip_uint64_t done=0;
    while(done<st.size){auto n=zip_fread(f,result.data()+done,st.size-done);if(n<=0){zip_fclose(f);fail("Lectura incompleta: "+name);}done+=n;}
    if(zip_fclose(f)!=0)fail("Error de integridad: "+name);return result;
}
QString Archive::extract(const QString &destination,const QString &expectedRoot,const std::function<void(int)> &progress,bool selectedRootOnly){
    quint64 total=0,done=0;QString actualRoot;QList<zip_int64_t> selected;
    for(zip_int64_t i=0;i<zip_get_num_entries(handle,0);i++){
        zip_stat_t st;zip_stat_index(handle,i,0,&st);
        const QString name=QString::fromUtf8(st.name).replace('\\','/');const QStringList parts=name.split('/');
        const bool extraSigil=expectedRoot=="Sigil" && parts.first()=="Sigil (2019)";
        if(name.startsWith('/')||parts.contains("..")||parts.first().contains(':'))fail("Ruta inesperada dentro del ZIP: "+name);
        if(parts.first().compare(expectedRoot,Qt::CaseInsensitive)!=0&&!extraSigil){
            if(selectedRootOnly)continue;
            fail("Ruta inesperada dentro del ZIP: "+name);
        }
        selected<<i;total+=st.size;
        if(!extraSigil){
            if(actualRoot.isEmpty())actualRoot=parts.first();
            if(actualRoot!=parts.first())fail("El ZIP contiene raíces incompatibles.");
        }
        zip_uint8_t os=0;zip_uint32_t attributes=0;zip_file_get_external_attributes(handle,i,0,&os,&attributes);
        if(os==ZIP_OPSYS_UNIX && ((attributes>>16)&0170000)==0120000)fail("Enlace no permitido dentro del ZIP.");
    }
    if(selected.isEmpty())fail("No se encontró el juego dentro del ZIP: "+expectedRoot);
    QDir().mkpath(destination);
    if(QStorageInfo(destination).bytesAvailable()<qint64(total+512*1024*1024))fail("No hay espacio suficiente para instalar este juego.");
    QByteArray buffer(1024*1024,Qt::Uninitialized);int last=-1;
    for(zip_int64_t i:selected){
        zip_stat_t st;zip_stat_index(handle,i,0,&st);QString name=QString::fromUtf8(st.name).replace('\\','/');
        const QString path=destination+'/'+name;
        if(name.endsWith('/')){QDir().mkpath(path);continue;}
        QDir().mkpath(QFileInfo(path).absolutePath());QFile out(path);
        if(!out.open(QIODevice::WriteOnly))fail("No se puede escribir el juego.");
        zip_file_t *file=zip_fopen_index(handle,i,0);if(!file)fail("No se puede leer un archivo del juego.");
        zip_int64_t bytes;
        while((bytes=zip_fread(file,buffer.data(),buffer.size()))>0){
            if(out.write(buffer.constData(),bytes)!=bytes){zip_fclose(file);fail("No se pudo completar la escritura.");}
            done+=bytes;int percent=int(done*100/qMax<quint64>(1,total));if(percent!=last){progress(percent);last=percent;}
        }
        int closed=zip_fclose(file);if(bytes<0||closed!=0)fail("El ZIP no superó la comprobación de integridad.");
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
