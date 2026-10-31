#include "magazines.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextStream>
#include <stdexcept>

static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);
    try{
        QTemporaryDir temp;const QString library=temp.path()+"/Biblioteca portátil",base=library+"/Revistas",work=temp.path()+"/local";
        auto write=[](const QString &path,const QByteArray &data){QDir().mkpath(QFileInfo(path).absolutePath());QFile f(path);check(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"escribir fixture");};
        Magazines m;m.load(library);check(m.series().isEmpty()&&m.error().isEmpty(),"biblioteca sin revistas");
        QJsonObject catalog{{"format",1},{"series",QJsonArray{QJsonObject{{"id","CGW"},{"title","CGW"},{"cover","Portadas/cover.jpg"}}}},{"entries",QJsonArray{
            QJsonObject{{"id","pdf"},{"series","CGW"},{"title","Número 1"},{"kind","pdf"},{"path","CGW/uno.pdf"}},
            QJsonObject{{"id","indice"},{"series","CGW"},{"isIndex",true},{"kind","pdf"},{"path","Indice.pdf"}},
            QJsonObject{{"id","BBD"},{"kind","dos"},{"path","BBD"},{"revision",QString(64,'a')}},
            QJsonObject{{"id","escape"},{"kind","pdf"},{"path","../fuera.pdf"}},
            QJsonObject{{"id","link"},{"kind","pdf"},{"path","enlace.pdf"}}
        }}};
        write(base+"/revistas.json",QJsonDocument(catalog).toJson());write(base+"/CGW/uno.pdf","documento");write(base+"/Portadas/cover.jpg","portada");write(base+"/BBD/run.bat","echo Revista\n");write(base+"/BBD/001/DATA.DAT","original");write(temp.path()+"/fuera.pdf","externo");
        check(QFile::link(temp.path()+"/fuera.pdf",base+"/enlace.pdf"),"crear enlace de prueba");
        m.load(library);check(m.series().size()==1&&m.issues("CGW").size()==1,"colecciones sin duplicar índices");
        check(m.pdf("pdf")==base+"/CGW/uno.pdf","resolver rutas relativas");
        for(const QString &id:QStringList{"escape","link","ausente","../BBD"}){bool rejected=false;try{m.pdf(id);}catch(const std::exception&){rejected=true;}check(rejected,"rechazar rutas externas e identificadores inválidos");}
        const auto local=m.prepareDos("BBD",work);check(QFileInfo(local+"/.ready").isFile(),"preparación completa");
        write(local+"/001/DATA.DAT","cambio local");check(m.prepareDos("BBD",work)==local,"reusar copia sin sobreescribir");
        QFile original(base+"/BBD/001/DATA.DAT");check(original.open(QIODevice::ReadOnly)&&original.readAll()=="original","original intacto");
        check(QFile::remove(local+"/.ready"),"retirar marcador de prueba");bool incomplete=false;try{m.prepareDos("BBD",work);}catch(const std::exception&){incomplete=true;}check(incomplete&&QFileInfo(local+"/001/DATA.DAT").exists(),"no sobrescribir copia incompleta");
        const QString relocated=temp.path()+"/Otro disco";check(QDir().rename(library,relocated),"trasladar biblioteca");m.load(relocated);check(m.pdf("pdf")==relocated+"/Revistas/CGW/uno.pdf","biblioteca portable");
        check(QFile::remove(relocated+"/Revistas/CGW/uno.pdf"),"retirar documento de prueba");bool missing=false;try{m.pdf("pdf");}catch(const std::exception&){missing=true;}check(missing,"archivo ausente produce error");
        write(relocated+"/Revistas/revistas.json","roto");m.load(relocated);check(m.series().isEmpty()&&!m.error().isEmpty(),"catálogo dañado no impide abrir la aplicación");
        QTextStream(stdout)<<"Revistas: rutas portables, aislamiento, preservación y errores correctos\n";return 0;
    }catch(const std::exception &e){QTextStream(stderr)<<e.what()<<'\n';return 1;}
}
