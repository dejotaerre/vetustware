#include "extras.h"
#include "archive.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QTextStream>
#include <stdexcept>

static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void put(const QString &path,const QByteArray &data){QFile f(path);require(f.open(QIODevice::WriteOnly),"No se pudo escribir fixture");require(f.write(data)==data.size(),"Fixture incompleto");}
static void makeZip(const QString &path,const QByteArray &data,const QString &name="extras/mapa.pdf",bool link=false){
    int error=0;auto zip=zip_open(QFile::encodeName(path).constData(),ZIP_CREATE|ZIP_TRUNCATE,&error);require(zip,"No se pudo crear ZIP");
    auto src=zip_source_buffer(zip,data.constData(),data.size(),0);require(src,"No se pudo crear entrada");
    auto index=zip_file_add(zip,name.toUtf8().constData(),src,ZIP_FL_ENC_UTF_8);require(index>=0,"No se pudo añadir entrada");
    if(link)zip_file_set_external_attributes(zip,index,0,ZIP_OPSYS_UNIX,0120777u<<16);
    require(zip_close(zip)==0,"No se pudo cerrar ZIP");
}
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QTemporaryDir temp;
    try{
        const QString lib=temp.path()+"/biblioteca",cache=temp.path()+"/cache";
        QDir().mkpath(lib+"/Complementos");Extras extras;extras.load(lib);require(extras.list("juego").isEmpty(),"Biblioteca sin extras no admitida");
        const QByteArray data(3*1024*1024,'M');const auto sha=QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex();
        makeZip(lib+"/Complementos/contenido.zip",data);
        QJsonObject item{{"name","mapa.pdf"},{"kind","Documento"},{"archive","Complementos/contenido.zip"},{"entry","extras/mapa.pdf"},{"bytes",data.size()},{"sha256",QString::fromLatin1(sha)}};
        auto publish=[&](const QJsonObject &entry){put(lib+"/Complementos/extras.json",QJsonDocument(QJsonObject{{"format",1},{"games",QJsonObject{{"juego",QJsonArray{entry}}}}}).toJson());extras.load(lib);};
        auto rejected=[&]{try{extras.prepare("juego",0,cache);return false;}catch(const std::exception &){return true;}};
        publish(item);require(extras.list("juego").size()==1&&extras.list("otro").isEmpty(),"Asociación al juego incorrecta");
        QString path=extras.prepare("juego",0,cache);QFile file(path);require(file.open(QIODevice::ReadOnly)&&file.readAll()==data,"Extracción incorrecta");file.close();
        put(path,"caché dañada");require(extras.prepare("juego",0,cache)==path,"No recuperó la caché dañada");
        require(file.open(QIODevice::ReadOnly),"No se pudo leer la caché reparada");require(file.readAll()==data,"Caché sin reparar");file.close();
        auto bad=item;bad["name"]="lanzar.sh";publish(bad);require(extras.list("juego").isEmpty()&&rejected(),"Se permitió ejecutar un script");
        bad=item;bad["archive"]="../../fuera.zip";publish(bad);require(rejected(),"Se aceptó una ruta fuera de la biblioteca");
        bad=item;bad["name"]="../mapa.pdf";publish(bad);require(rejected(),"Se aceptó un nombre con traversal");
        bad=item;bad["sha256"]=QString(64,'0');publish(bad);require(rejected(),"Se aceptó un hash incorrecto");
        bad=item;bad["bytes"]=1;publish(bad);require(rejected(),"Se aceptó un tamaño incorrecto");
        bad=item;bad["entry"]="extras/../mapa.pdf";publish(bad);require(rejected(),"Se aceptó una ruta ZIP peligrosa");
        makeZip(lib+"/Complementos/enlace.zip",data,"extras/mapa.pdf",true);bad=item;bad["archive"]="Complementos/enlace.zip";bad["sha256"]=QString(64,'1');publish(bad);require(rejected(),"Se aceptó un enlace del ZIP");
        // Una extracción fallida nunca sustituye un archivo completo existente.
        const QString atomic=temp.path()+"/anterior.pdf";put(atomic,"anterior");bool refused=false;
        try{Archive archive(lib+"/Complementos/contenido.zip");archive.extractFile("extras/mapa.pdf",atomic,data.size(),QByteArray(64,'0'));}catch(...){refused=true;}
        QFile previous(atomic);require(previous.open(QIODevice::ReadOnly),"No se pudo leer el archivo anterior");require(refused&&previous.readAll()=="anterior","Escritura parcial visible");
        extras.load(temp.path()+"/otra-biblioteca");require(extras.list("juego").isEmpty(),"Se conservaron extras de otra biblioteca");
        QTextStream(stdout)<<"Extras: asociación, caché, integridad, formatos, rutas, enlaces y escritura atómica correctos\n";return 0;
    }catch(const std::exception &e){QTextStream(stderr)<<e.what()<<'\n';return 1;}
}
