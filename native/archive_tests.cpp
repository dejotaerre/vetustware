#include "archive.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <stdexcept>

static void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
static void fixture(const QString &path,const QStringList &names,bool symlink=false){
    int err;zip_t *z=zip_open(QFile::encodeName(path).constData(),ZIP_CREATE|ZIP_TRUNCATE,&err);require(z,"No se pudo crear fixture");
    static const char data[]="partida";
    for(const auto &name:names){auto source=zip_source_buffer(z,data,7,0);auto i=zip_file_add(z,name.toUtf8().constData(),source,ZIP_FL_ENC_UTF_8);require(i>=0,"Error fixture");if(symlink)zip_file_set_external_attributes(z,i,0,ZIP_OPSYS_UNIX,0120777u<<16);}
    require(zip_close(z)==0,"Error cerrando fixture");
}
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QTemporaryDir tmp;
    try{
        QString zip=tmp.path()+"/game.zip",out=tmp.path()+"/out";
        fixture(zip,{"GAME/SAVE.DAT"});{Archive a(zip);require(a.extract(out,"GAME",[](int){})=="GAME","Raíz incorrecta");}
        QFile save(out+"/GAME/SAVE.DAT");require(save.open(QIODevice::ReadOnly),"No se extrajo partida");require(save.readAll()=="partida","Contenido incorrecto");save.close();
        fixture(zip,{"GAME/../../escape"});bool rejected=false;try{Archive a(zip);a.extract(tmp.path()+"/bad","GAME",[](int){});}catch(...){rejected=true;}require(rejected,"Traversal aceptado");require(!QFile::exists(tmp.path()+"/escape"),"Salida del directorio");
        fixture(zip,{"GAME/link"},true);rejected=false;try{Archive a(zip);a.extract(tmp.path()+"/link","GAME",[](int){});}catch(...){rejected=true;}require(rejected,"Enlace aceptado");
        fixture(zip,{"OTHER/SAVE.DAT"});rejected=false;try{Archive a(zip);a.extract(tmp.path()+"/wrong","GAME",[](int){});}catch(...){rejected=true;}require(rejected,"Raíz ajena aceptada");
        fixture(zip,{"Sigil/SIGIL.WAD","Sigil (2019)/readme.txt"});{Archive a(zip);require(a.extract(tmp.path()+"/sigil","Sigil",[](int){})=="Sigil","Excepción Sigil incorrecta");}
        require(resolveCase(out+"/game/save.dat")==out+"/GAME/SAVE.DAT","Resolución de mayúsculas incorrecta");
        fixture(zip,{"FIRST/SAVE.DAT","SECOND/SAVE.DAT"});
        {Archive a(zip);require(a.extract(tmp.path()+"/selected","SECOND",[](int){},true)=="SECOND","No seleccionó la edición solicitada");}
        require(!QFile::exists(tmp.path()+"/selected/FIRST/SAVE.DAT"),"Extrajo otro juego de la biblioteca");
        require(QFile::exists(tmp.path()+"/selected/SECOND/SAVE.DAT"),"Falta la edición solicitada");
        rejected=false;try{Archive a(zip);a.extract(tmp.path()+"/missing","MISSING",[](int){},true);}catch(...){rejected=true;}require(rejected,"Edición ausente aceptada");
        fixture(zip,{"SECOND/../../escape"});rejected=false;try{Archive a(zip);a.extract(tmp.path()+"/bad-selected","SECOND",[](int){},true);}catch(...){rejected=true;}require(rejected,"Traversal en selección aceptado");
        QTextStream(stdout)<<"ZIP: extracción, selección de edición, ausencia, integridad de rutas y enlaces correctos\n";return 0;
    }catch(const std::exception &e){QTextStream(stderr)<<e.what()<<'\n';return 1;}
}
