#include "backend.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QLockFile>
#include <QDir>
#include <QIcon>
#include <QTimer>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QTextStream>
#include <QJsonDocument>

int main(int argc,char **argv){
    if(qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")&&!qEnvironmentVariableIsEmpty("DISPLAY"))qputenv("QT_QPA_PLATFORM","xcb");
    QGuiApplication app(argc,argv);app.setApplicationName("VETUST");app.setApplicationDisplayName("VETUSTware");app.setOrganizationName("VETUST");app.setDesktopFileName("vetust");
    QLocale::setDefault(QLocale("es_UY"));QTranslator translator;
    if(translator.load("qt_es",QLibraryInfo::path(QLibraryInfo::TranslationsPath)))app.installTranslator(&translator);
    QString root=QCoreApplication::applicationDirPath();
    if(!QFileInfo::exists(root+"/vetustware.install")&&QFileInfo::exists(root+"/../../native/CMakeLists.txt"))root=QDir(root+"/../..").absolutePath();
    QDir().mkpath(root+"/data");
    QLockFile lock(root+"/data/native.lock");if(!lock.tryLock(100)){QTextStream(stderr)<<"VETUSTware ya está abierto.\n";return 0;}
    app.setWindowIcon(QIcon(QFileInfo::exists(root+"/icon.svg")?root+"/icon.svg":root+"/native/icon.svg"));QQuickStyle::setStyle("Basic");
    try{
        Backend backend(root);
        if(app.arguments().contains("--diagnostics")){QTextStream(stdout)<<"Instalación: "<<root<<'\n'<<backend.diagnostics()<<QJsonDocument::fromVariant(backend.storageSettings()).toJson();return 0;}
        if(app.arguments().contains("--self-test")){
            const int total=backend.count();
            backend.filter("","","all",false);if(backend.count()!=total)return 2;
            backend.filter("zzzzsinresultado","","all",false);if(backend.count()!=0)return 4;
            backend.filter("","","all",true);for(const auto &game:backend.pageGames())if(!game.toMap()["tested"].toBool())return 5;
            QTextStream(stdout)<<backend.diagnostics()<<"Filtros nativos: correctos\n";return 0;
        }
        QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("vetust",&backend);auto art=new ArtProvider(root,backend.assets());engine.addImageProvider("art",art);
        QObject::connect(&backend,&Backend::storageChanged,&app,[&]{art->setAssets(backend.assets());});
        QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
        engine.loadFromModule("Vetust","Main");
        return app.exec();
    }catch(const std::exception &e){QTextStream(stderr)<<"VETUSTware: "<<e.what()<<'\n';return 1;}
}
