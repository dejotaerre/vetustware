#include "backend.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QSignalSpy>
#include <QWheelEvent>

static const QString Seed="67bdc1a5-86ff-4cc8-a8e1-293a97de6e95";
struct Fixture {
    QTemporaryDir temp;
    std::unique_ptr<Backend> backend;
    explicit Fixture(bool history=false,bool console=false){
        QDir().mkpath(temp.path()+"/data/games");QDir().mkpath(temp.path()+"/spanish");
        QFile marker(temp.path()+"/spanish/.ready");if(!marker.open(QIODevice::WriteOnly))qFatal("No se pudo crear el marcador de prueba");marker.close();
        QJsonArray games;QJsonObject profiles;
        for(int i=0;i<40;++i){
            QString id=i==0?Seed:QString::number(i);
            QString genre=i<3?"Adventure":i==3?"Adventure;Interactive Fiction":i<20?"Racing / Driving":"Puzzle";
            games.append(QJsonObject{{"id",id},{"title",i==0?"Monkey Island":QString("Juego %1").arg(i,2,10,QChar('0'))},{"genre",genre},{"year","1994"},{"developer",i<2?"LucasArts":"Otro estudio"},{"description","Descripción de prueba"},{"cover",""},{"shots",QJsonArray{}}});
            profiles[id]=QJsonObject{{"config","[cpu]\ncycles=5000\n"},{"archive","test.zip"},{"tested",false}};
        }
        auto spanish=profiles["5"].toObject();spanish["language"]="es";spanish["editionDirectory"]=temp.path()+"/spanish";profiles["5"]=spanish;
        auto write=[&](const QString &name,const QJsonObject &object){QFile f(temp.path()+"/data/"+name);if(!f.open(QIODevice::WriteOnly))qFatal("No se pudo escribir el catálogo de prueba");f.write(QJsonDocument(object).toJson());};
        write("catalog.json",{{"games",games},{"assets",QJsonObject{}}});write("original-profiles.json",profiles);
        write("spanish-profiles.json",{{"2",QJsonObject{{"editionDirectory",temp.path()+"/spanish"}}},{"3",QJsonObject{{"editionDirectory",temp.path()+"/missing"}}}});
        if(history){
            {
                auto db=QSqlDatabase::addDatabase("QSQLITE","fixture");db.setDatabaseName(temp.path()+"/data/personal.sqlite");db.open();
                QSqlQuery q(db);q.exec("CREATE TABLE games(id TEXT PRIMARY KEY,favorite INTEGER DEFAULT 0,last_played REAL)");
                q.exec("INSERT INTO games VALUES('1',0,1000)");q.exec("INSERT INTO games VALUES('4',0,2000)");
            }
            QSqlDatabase::removeDatabase("fixture");
        }
        if(console){
            const QString commands="[sdl]\nfullscreen=false\n[cpu]\ncycles=5000\n[autoexec]\nmount c \""+temp.path()+"/spanish\"\nc:\nGAME.EXE\nexit\n";
            auto profile=QJsonObject{{"config",commands},{"editionDirectory",temp.path()+"/spanish"}};
            profiles[Seed]=profile;write("original-profiles.json",profiles);write("spanish-profiles.json",{{Seed,profile}});
            write("emulation.json",{{"global",QJsonObject{{"engine","classic"},{"options",QJsonObject{{"classic",QJsonObject{{"cpu.cycles","9000"}}}}}}},{"paths",QJsonObject{{"classic","/usr/bin/true"}}}});
        }
        QDir().mkpath(temp.path()+"/config");
        QFile location(temp.path()+"/config/storage.json");if(!location.open(QIODevice::WriteOnly))qFatal("No se pudo escribir la ubicación de prueba");location.write(QJsonDocument(QJsonObject{{"library",temp.path()+"/source"},{"games",temp.path()+"/data/games"}}).toJson());location.close();
        backend=std::make_unique<Backend>(temp.path());
    }
    ~Fixture(){backend.reset();QSqlDatabase::removeDatabase("vetust-native");}
    QVariantMap row(const QString &key){for(auto v:backend->homeRows())if(v.toMap()["key"]==key)return v.toMap();return {};}
};

static QQuickItem *findVisual(QQuickItem *item,const QString &name){
    if(item->objectName()==name)return item;
    for(auto child:item->childItems())if(auto found=findVisual(child,name))return found;
    return nullptr;
}
class HomeTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){QQuickStyle::setStyle("Basic");}
    void collectionsAndPagination(){
        Fixture f;auto &b=*f.backend;
        QVERIFY(!f.row("recommendations").isEmpty());QVERIFY(f.row("recent").isEmpty());QVERIFY(f.row("favorites").isEmpty());
        QCOMPARE(f.row("spanish")["count"].toInt(),2);
        b.filter("","","all",false,"adventure");QCOMPARE(b.count(),3);
        for(auto item:b.pageGames())QVERIFY(!item.toMap()["genre"].toString().contains("Interactive Fiction"));
        b.filter("Juego 05","","all",false,"racing");QCOMPARE(b.count(),1);
        b.filter("Monkey","","all",false,"racing");QCOMPARE(b.count(),0);
        b.filter("","","all",false);QCOMPARE(b.count(),40);QCOMPARE(b.pages(),2);QCOMPARE(b.pageGames().size(),28);
        QSet<QString> ids;QString previous;
        for(int page=0;page<2;++page){
            for(auto v:b.pageGames()){
                auto g=v.toMap();QVERIFY(!ids.contains(g["id"].toString()));ids.insert(g["id"].toString());
                QVERIFY(previous.isEmpty()||QString::localeAwareCompare(previous,g["title"].toString())<=0);previous=g["title"].toString();
            }
            b.pageBy(1);
        }
        QCOMPARE(ids.size(),40);b.pageBy(100);QCOMPARE(b.page(),1);
    }
    void recommendationsAndHistory(){
        Fixture f(true);auto &b=*f.backend;
        auto items=f.row("recent")["games"].toList();QCOMPARE(items.size(),2);QCOMPARE(items[0].toMap()["id"].toString(),QString("4"));
        b.selectGame(Seed);b.toggleFavorite();
        auto row=f.row("recommendations");QVERIFY(row["title"].toString().contains("Monkey Island"));
        auto recommended=row["games"].toList();QVERIFY(!recommended.isEmpty());
        for(auto v:recommended){auto g=v.toMap();QVERIFY(g["id"]!=Seed);QVERIFY(g["id"]!="1");QVERIFY(g["id"]!="4");QVERIFY(g["genre"].toString().contains("Adventure")||g["developer"]=="LucasArts");}
        QCOMPARE(f.row("favorites")["count"].toInt(),1);
        const auto rows=b.homeRows();b.filter("zzzz","","all",false);QCOMPARE(b.homeRows(),rows);b.refreshHome();QCOMPARE(b.homeRows(),rows);
        b.filter("","","all",false,"favorites");QCOMPARE(b.count(),1);b.toggleFavorite();QCOMPARE(b.count(),0);QVERIFY(f.row("favorites").isEmpty());
    }
    void consoleLaunch(){
        Fixture f(false,true);auto &b=*f.backend;b.selectGame(Seed);
        b.openConsole();QVERIFY(b.busy());QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);
        QVERIFY(b.status().startsWith("Consola cerrada"));QVERIFY(f.row("recent").isEmpty());
        QSqlQuery query(QSqlDatabase::database("vetust-native"));QVERIFY(query.exec("SELECT COUNT(*) FROM games WHERE last_played > 0"));QVERIFY(query.next());QCOMPARE(query.value(0).toInt(),0);
        QDir spanish(f.temp.path()+"/config/sessions/"+Seed+"-es");
        const auto configs=spanish.entryList({"vetust-console-*.conf"},QDir::Files);QCOMPARE(configs.size(),1);
        QFile config(spanish.filePath(configs.first()));QVERIFY(config.open(QIODevice::ReadOnly));const QString text=QString::fromUtf8(config.readAll());
        QVERIFY(text.contains("cycles=9000"));QVERIFY(!text.contains("GAME.EXE"));QVERIFY(text.contains("c:\ncd \\\n"));
        QVERIFY(QFileInfo::exists(f.temp.path()+"/logs/"+Seed+"-es-console-native.log"));
        b.setSpanish(false);b.openConsole();QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);
        QDir original(f.temp.path()+"/config/sessions/"+Seed);QCOMPARE(original.entryList({"vetust-console-*.conf"},QDir::Files).size(),1);
        QVERIFY(!b.xTreeAvailable());b.openConsole("xtree");QVERIFY(!b.busy());QVERIFY(b.status().contains("no está instalado"));
        const QString tools=f.temp.path()+"/runtime/dos-tools/xtgold";QDir().mkpath(tools);
        for(const QString &name:QStringList{"xtgold.com","xtg.exe","XTREE.BAT"}){QFile file(tools+'/'+name);QVERIFY(file.open(QIODevice::WriteOnly));file.write("prueba");}
        QVERIFY(b.xTreeAvailable());b.openConsole("xtree");QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);QVERIFY(f.row("recent").isEmpty());
        bool launchesXTree=false;
        for(const QString &name:original.entryList({"vetust-console-*.conf"},QDir::Files)){QFile file(original.filePath(name));QVERIFY(file.open(QIODevice::ReadOnly));if(file.readAll().contains("T:\\XTGOLD.COM C: /R"))launchesXTree=true;}
        QVERIFY(launchesXTree);
        QVERIFY(!b.nortonAvailable());b.openConsole("norton");QVERIFY(!b.busy());QVERIFY(b.status().contains("no está instalado"));
        const QString nc=f.temp.path()+"/runtime/dos-tools/norton";QDir().mkpath(nc);
        for(const QString &name:QStringList{"NC.EXE","NCMAIN.EXE"}){QFile file(nc+'/'+name);QVERIFY(file.open(QIODevice::WriteOnly));file.write("prueba");}
        QVERIFY(b.nortonAvailable());b.openConsole("norton");QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);QVERIFY(f.row("recent").isEmpty());
        bool launchesNorton=false;
        for(const QString &name:original.entryList({"vetust-console-*.conf"},QDir::Files)){QFile file(original.filePath(name));QVERIFY(file.open(QIODevice::ReadOnly));if(file.readAll().contains("U:\\NC.EXE\n"))launchesNorton=true;}
        QVERIFY(launchesNorton);
        b.play();QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);QCOMPARE(f.row("recent")["count"].toInt(),1);
        const auto normal=original.entryList({"vetust-classic-*.conf"},QDir::Files);QCOMPARE(normal.size(),1);
        QFile normalConfig(original.filePath(normal.first()));QVERIFY(normalConfig.open(QIODevice::ReadOnly));QVERIFY(normalConfig.readAll().contains("GAME.EXE\nexit"));
    }
    void externalLibraryAndMissingDisk(){
        Fixture f;auto &backend=*f.backend;
        QTemporaryDir external(QDir::tempPath()+"/vetust juegos-XXXXXX");QVERIFY(external.isValid());
        const QString lib=f.temp.path()+"/source";
        QDir().mkpath(lib+"/Content");QDir().mkpath(lib+"/eXo/eXoDOS");
        for(const auto &name:QStringList{"XODOSMetadata.zip","!DOSmetadata_linux.zip"}){QFile file(lib+"/Content/"+name);QVERIFY(file.open(QIODevice::WriteOnly));}
        QCOMPARE(backend.saveStorage(QUrl::fromLocalFile(lib),QUrl::fromLocalFile(external.path())),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!backend.busy(),5000);QCOMPARE(backend.storageSettings()["games"].toString(),external.path());
        const QString target=external.path()+"/relocated";QDir().mkpath(target+"/Game");
        for(const auto &name:QStringList{".ready","Game/SAVE.DAT"}){QFile file(target+'/'+name);QVERIFY(file.open(QIODevice::WriteOnly));file.write("partida conservada");}
        QJsonObject game{{"id","relocated"},{"folder","Game"}};
        QJsonObject profile{{"archive","original.zip"},{"config","[sdl]\nfullscreen=false\n[autoexec]\nmount c ./eXoDOS/Game\nc:\nGAME.EXE\nexit\n"}};
        auto plan=backend.prepare(game,profile,"classic",{},[](int){},true);
        QVERIFY2(plan.error.isEmpty(),qPrintable(plan.error));QVERIFY(plan.config.startsWith(f.temp.path()+"/config/sessions/"));
        QFile config(plan.config);QVERIFY(config.open(QIODevice::ReadOnly));QVERIFY(config.readAll().contains("mount c"));
        QCOMPARE(QFileInfo(plan.directory+"/eXoDOS/Game").symLinkTarget(),target+"/Game");
        QFile save(target+"/Game/SAVE.DAT");QVERIFY(save.open(QIODevice::ReadOnly));QCOMPARE(save.readAll(),QByteArray("partida conservada"));save.close();
        profile["editionDirectory"]="${GAMES}/relocated";profile["config"]="[autoexec]\nmount c \"${GAMES}/relocated/Game\"\nGAME.EXE\n";
        plan=backend.prepare(game,profile,"staging",{},[](int){},true);QVERIFY2(plan.error.isEmpty(),qPrintable(plan.error));
        QFile relocated(plan.config);QVERIFY(relocated.open(QIODevice::ReadOnly));const auto text=relocated.readAll();QVERIFY(text.contains(external.path().toUtf8()));QVERIFY(!text.contains("${GAMES}"));
        const QString absent=external.path()+"-offline";QVERIFY(QDir().rename(external.path(),absent));
        plan=backend.prepare(game,profile,"staging",{},[](int){});QVERIFY(plan.error.contains("no está disponible"));QVERIFY(!QFileInfo::exists(external.path()));
        QVERIFY(QDir().rename(absent,external.path()));
    }
    void restorePersonalizedEdition(){
        Fixture f;auto &backend=*f.backend;
        const QString custom=f.temp.path()+"/source/Personalizaciones";QDir().mkpath(custom);
        const QString file=custom+"/juegos.zip";int error=0;
        zip_t *archive=zip_open(QFile::encodeName(file).constData(),ZIP_CREATE|ZIP_TRUNCATE,&error);QVERIFY(archive);
        static const char payload[]="contenido original";
        for(const QString &name:QStringList{"custom/game/.ready","custom/game/SAVE.DAT","another/game/SAVE.DAT"}){
            auto source=zip_source_buffer(archive,payload,sizeof(payload)-1,0);QVERIFY(source);QVERIFY(zip_file_add(archive,name.toUtf8().constData(),source,ZIP_FL_ENC_UTF_8)>=0);
        }
        QCOMPARE(zip_close(archive),0);
        QJsonObject game{{"id","custom"},{"folder","game"}};
        QJsonObject profile{{"preparedRoot","custom"},{"editionDirectory","${GAMES}/custom/game"},{"config","[autoexec]\nmount c \"${GAMES}/custom/game\"\nc:\nGAME.EXE\n"}};
        auto plan=backend.prepare(game,profile,"staging",{},[](int){});QVERIFY2(plan.error.isEmpty(),qPrintable(plan.error));
        const QString save=f.temp.path()+"/data/games/custom/game/SAVE.DAT";
        QFile saved(save);QVERIFY(saved.open(QIODevice::WriteOnly));saved.write("partida nueva");saved.close();
        plan=backend.prepare(game,profile,"staging",{},[](int){});QVERIFY(plan.error.isEmpty());
        QVERIFY(saved.open(QIODevice::ReadOnly));QCOMPARE(saved.readAll(),QByteArray("partida nueva"));saved.close();
        QVERIFY(!QFileInfo::exists(f.temp.path()+"/data/games/another"));
        QVERIFY(QDir(f.temp.path()+"/data/games/custom").removeRecursively());
        plan=backend.prepare(game,profile,"staging",{},[](int){});QVERIFY(plan.error.isEmpty());
        QVERIFY(saved.open(QIODevice::ReadOnly));QCOMPARE(saved.readAll(),QByteArray(payload));saved.close();
        QVERIFY(QFile::remove(f.temp.path()+"/data/games/custom/game/.ready"));
        plan=backend.prepare(game,profile,"staging",{},[](int){});QVERIFY(!plan.error.isEmpty());QVERIFY(saved.exists());
        profile["preparedRoot"]="../escape";plan=backend.prepare(game,profile,"staging",{},[](int){});QVERIFY(!plan.error.isEmpty());
    }
    void monitorSettingsUi(){
        QQmlApplicationEngine engine;
        engine.loadData(R"QML(
import QtQuick
import QtQuick.Controls
ApplicationWindow {
    width:1050; height:820; visible:true
    QtObject {
        id:vetust
        property var monitors: []
        property var selected: ({title:"Prueba"})
        function refreshMonitors() {}
        function emulatorSettings(global) {return {scope:{engine:"staging",options:{},monitor:"DP-2"},paths:{},engines:[{id:"staging",name:"DOSBox Staging",available:true,path:"/dosbox"}],globalEngine:"staging",gameId:"prueba"}}
        function inheritedSettings(global,engine) {return {}}
        function emulatorFields(engine) {return []}
    }
    property alias screens:vetust.monitors
    EmulatorDialog {id:settings}
    function begin(global) {settings.begin(global)}
}
)QML",QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/MonitorTest.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        const QVariantMap a{{"id","DP-3"},{"label","DP-3 · 2560 × 1440"}},b{{"id","DP-2"},{"label","DP-2 · 1920 × 1080"}};
        auto begin=[&](bool global){return QMetaObject::invokeMethod(window,"begin",Q_ARG(QVariant,QVariant(global)));};
        QVERIFY(begin(true));QTest::qWait(100);
        auto section=window->findChild<QQuickItem*>("monitorSection");auto picker=window->findChild<QQuickItem*>("monitorPicker");QVERIFY(section);QVERIFY(picker);
        QVERIFY(!section->isVisible());
        window->setProperty("screens",QVariantList{a});QVERIFY(!section->isVisible());
        window->setProperty("screens",QVariantList{a,b});QTRY_VERIFY(section->isVisible());QCOMPARE(picker->property("currentIndex").toInt(),2);
        window->setProperty("screens",QVariantList{b,a});QCOMPARE(picker->property("currentIndex").toInt(),1);
        window->setProperty("screens",QVariantList{a});QVERIFY(!section->isVisible());QVERIFY(picker->property("currentText").toString().contains("Desconectado"));
        window->setProperty("screens",QVariantList{a,b});QCOMPARE(picker->property("currentIndex").toInt(),2);
        QVERIFY(begin(false));QVERIFY(!section->isVisible());
    }
    void imageStyleUi(){
        Fixture f;QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.loadData(R"QML(
import QtQuick
import QtQuick.Controls
ApplicationWindow {
    width:1050; height:820; visible:true
    EmulatorDialog {id:settings}
    function begin(global) {settings.begin(global)}
}
)QML",QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/ImageStyleTest.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        auto begin=[&](bool global){return QMetaObject::invokeMethod(window,"begin",Q_ARG(QVariant,QVariant(global)));};
        QVERIFY(begin(true));QTest::qWait(100);
        auto picker=window->findChild<QQuickItem*>("imageStylePicker");auto dialog=window->findChild<QObject*>("emulatorDialog");QVERIFY(picker&&dialog);
        QVERIFY(picker->isVisible());QCOMPARE(picker->property("currentText").toString(),QString("Nítido"));
        auto choose=[&](int index){picker->setProperty("currentIndex",index);return QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,index));};
        auto save=[&](bool global){return f.backend->saveEmulatorSettings(global,f.backend->selected()["id"].toString(),dialog->property("draft").toMap(),dialog->property("paths").toMap());};
        QVERIFY(choose(1));QCOMPARE(save(true),QString());
        QVERIFY(begin(true));QCOMPARE(picker->property("currentText").toString(),QString("Nítido"));
        QVERIFY(begin(false));QCOMPARE(picker->property("currentText").toString(),QString("Usar ajuste global · Nítido"));
        QVERIFY(choose(1));QCOMPARE(save(false),QString());
        QVERIFY(begin(false));QCOMPARE(picker->property("currentText").toString(),QString("Monitor clásico (CRT)"));
        QVERIFY(choose(0));QCOMPARE(save(false),QString());
        QVERIFY(begin(false));QCOMPARE(picker->property("currentText").toString(),QString("Usar ajuste global · Nítido"));
        dialog->setProperty("editEngine","classic");QVERIFY(!picker->isVisible());
    }
    void magazinesUiAndLaunch(){
        Fixture f;f.backend.reset();QSqlDatabase::removeDatabase("vetust-native");
        const QString base=f.temp.path()+"/source/Revistas";QDir().mkpath(base+"/BBD");QDir().mkpath(base+"/CGW");
        auto write=[](const QString &path,const QByteArray &data){QFile file(path);if(!file.open(QIODevice::WriteOnly))qFatal("No se pudo escribir la revista de prueba");file.write(data);};
        write(base+"/BBD/run.bat","echo Revista\n");write(base+"/CGW/uno.pdf","documento de prueba");
        const QJsonObject collection{{"id","CGW"},{"title","Computer Gaming World"},{"description","Revistas de prueba"},{"kind","pdf"},{"count",1}};
        const QJsonObject dos{{"id","BBD"},{"title","Big Blue Disk"},{"description","Menú DOS"},{"kind","dos"},{"count",1},{"launchId","BBD"}};
        write(base+"/revistas.json",QJsonDocument(QJsonObject{{"format",1},{"series",QJsonArray{collection,dos}},{"entries",QJsonArray{QJsonObject{{"id","uno"},{"series","CGW"},{"title","Número 1"},{"kind","pdf"},{"path","CGW/uno.pdf"}},QJsonObject{{"id","BBD"},{"series","BBD"},{"title","Big Blue Disk"},{"kind","dos"},{"path","BBD"},{"revision",QString(64,'a')}}}}}).toJson());
        f.backend=std::make_unique<Backend>(f.temp.path());auto &b=*f.backend;
        QSignalSpy ready(&b,&Backend::extraReady);b.openMagazine("uno");QCOMPARE(ready.size(),1);QCOMPARE(ready[0][0].toUrl(),QUrl::fromLocalFile(base+"/CGW/uno.pdf"));QVERIFY(ready[0][2].toBool());
        QCOMPARE(b.saveEmulatorSettings(true,"",{{"engine","classic"},{"options",QVariantMap{{"classic",QVariantMap{{"cpu.cycles","9000"}}}}}},{{"classic","/usr/bin/true"}}),QString());
        b.openMagazine("BBD");QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),10000);QVERIFY(b.status().startsWith("Revista cerrada"));QVERIFY(f.row("recent").isEmpty());
        QDir configs(f.temp.path()+"/config/sessions/revistas");const auto names=configs.entryList({"*.conf"},QDir::Files);QCOMPARE(names.size(),1);
        QFile config(configs.filePath(names.first()));QVERIFY(config.open(QIODevice::ReadOnly));const auto text=config.readAll();QVERIFY(text.contains("cycles=9000"));QVERIFY(text.contains("/data/magazines/BBD-"));QVERIFY(!text.contains("/source/Revistas"));QVERIFY(text.contains("call run.bat"));
        QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("vetust",&b);engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->resize(1050,720);
        QVERIFY(QMetaObject::invokeMethod(window,"navigate",Q_ARG(QVariant,QVariant("magazines"))));QTest::qWait(150);
        auto grid=window->findChild<QQuickItem*>("magazinesGrid"),search=window->findChild<QQuickItem*>("searchField");QVERIFY(grid&&search);QCOMPARE(grid->property("count").toInt(),2);
        search->forceActiveFocus();for(char c:QByteArray("Computer")){QTest::keyClick(window,c);QVERIFY(search->hasActiveFocus());}QTRY_COMPARE(grid->property("count").toInt(),1);
        auto page=window->findChild<QObject*>("magazinesView");QVERIFY(page);search->setProperty("text",QString());
        QVERIFY(QMetaObject::invokeMethod(page,"selectCollection",Q_ARG(QVariant,collection.toVariantMap())));QTRY_COMPARE(grid->property("count").toInt(),1);
        QVERIFY(QMetaObject::invokeMethod(page,"selectCollection",Q_ARG(QVariant,dos.toVariantMap())));QTRY_VERIFY(window->findChild<QQuickItem*>("openDosMagazine")->isVisible());
        QVERIFY(QMetaObject::invokeMethod(window,"navigate",Q_ARG(QVariant,QVariant("all"))));QVERIFY(!grid->isVisible());QCOMPARE(b.count(),40);
    }
    void extrasUi(){
        Fixture f;f.backend.reset();QSqlDatabase::removeDatabase("vetust-native");
        QDir().mkpath(f.temp.path()+"/source/Complementos");
        QFile index(f.temp.path()+"/source/Complementos/extras.json");QVERIFY(index.open(QIODevice::WriteOnly));
        const QJsonArray entries{QJsonObject{{"name","Mapa.pdf"},{"kind","Documento"},{"bytes",100}},QJsonObject{{"name","Foto.png"},{"kind","Imagen"},{"bytes",200}}};
        index.write(QJsonDocument(QJsonObject{{"format",1},{"games",QJsonObject{{Seed,entries}}}}).toJson());index.close();
        f.backend=std::make_unique<Backend>(f.temp.path());
        QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->resize(1050,720);window->requestActivate();QTest::qWait(150);
        QVERIFY(QMetaObject::invokeMethod(window,"openGame",Q_ARG(QVariant,QVariant(Seed))));
        auto button=window->findChild<QQuickItem*>("extrasButton");QVERIFY(button);QTRY_VERIFY(button->isVisible());QTest::qWait(100);
        const auto point=button->mapToScene({button->width()/2,button->height()/2});QVERIFY(point.x()>0&&point.x()<window->width());QVERIFY(point.y()>0&&point.y()<window->height());
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point.toPoint());
        auto dialog=window->findChild<QObject*>("extrasDialog");QVERIFY(dialog);QTRY_VERIFY(dialog->property("opened").toBool());
        auto list=window->findChild<QQuickItem*>("extrasList"),search=window->findChild<QQuickItem*>("extrasSearch");QVERIFY(list&&search);QTRY_COMPARE(list->property("count").toInt(),2);
        search->forceActiveFocus();for(char c:QByteArray("Foto")){QTest::keyClick(window,c);QVERIFY(search->hasActiveFocus());}
        QTRY_COMPARE(list->property("count").toInt(),1);QVERIFY(!f.backend->selected()["favorite"].toBool());
        QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!dialog->property("visible").toBool());
        f.backend->selectGame("1");QTRY_VERIFY(!button->isVisible());
    }
    void uiSearchAndDetails(){
        Fixture f;QQmlApplicationEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->requestActivate();QTest::qWait(300);
        QCOMPARE(window->property("view").toString(),QString("home"));
        auto field=window->findChild<QQuickItem*>("searchField");QVERIFY(field);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,field->mapToScene({field->width()/2,field->height()/2}).toPoint());
        for(char c:QByteArray("Monkey")){QTest::keyClick(window,c);QTest::qWait(200);QVERIFY(field->hasActiveFocus());}
        QCOMPARE(field->property("text").toString(),QString("Monkey"));QCOMPARE(window->property("view").toString(),QString("all"));QCOMPARE(f.backend->count(),1);
        auto grid=window->findChild<QQuickItem*>("catalogGrid");QVERIFY(grid);
        auto card=qvariant_cast<QQuickItem*>(grid->property("currentItem"));QVERIFY(card);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,card->mapToScene({card->width()/2,card->height()/2}).toPoint());
        auto details=window->findChild<QObject*>("gameDetails");QVERIFY(details);QTRY_VERIFY(details->property("opened").toBool());
        auto tools=window->findChild<QQuickItem*>("gameToolsButton");QVERIFY(tools);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,tools->mapToScene({tools->width()/2,tools->height()/2}).toPoint());
        auto menu=window->findChild<QObject*>("gameToolsMenu");QVERIFY(menu);QTRY_VERIFY(menu->property("opened").toBool());
        QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!menu->property("opened").toBool());

        QCOMPARE(f.backend->selected()["id"].toString(),Seed);QVERIFY(!f.backend->busy());
        QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!details->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(window,"navigate",Q_ARG(QVariant,QVariant("home"))));QTest::qWait(200);
        auto shelf=findVisual(window->contentItem(),"shelf-recommendations");QVERIFY(shelf);
        auto first=qvariant_cast<QQuickItem*>(shelf->property("currentItem"));QVERIFY(first);
        auto feed=window->findChild<QQuickItem*>("homeFeed");QVERIFY(feed);
        const auto point=first->mapToScene({first->width()/2,first->height()/2});
        const auto before=feed->property("contentY").toDouble();
        QWheelEvent wheel(point,window->mapToGlobal(point.toPoint()),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QCoreApplication::sendEvent(window,&wheel);QTest::qWait(250);
        QVERIFY(feed->property("contentY").toDouble()>before);
        feed->setProperty("contentY",before);first->forceActiveFocus();
        QTest::keyClick(window,Qt::Key_Right);QCOMPARE(shelf->property("currentIndex").toInt(),1);
        QTest::keyClick(window,Qt::Key_Return);QTRY_VERIFY(details->property("opened").toBool());QVERIFY(!f.backend->busy());
        QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!details->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(window,"openCollection",Q_ARG(QVariant,QVariant(f.row("spanish")))));
        QCOMPARE(window->property("collectionKey").toString(),QString("spanish"));QCOMPARE(f.backend->count(),2);
        window->resize(1050,720);QTest::qWait(200);
        QVERIFY(field->mapToScene({field->width(),0}).x()<=window->width());
        QVERIFY(field->mapToScene({0,0}).x()>=0);
    }
};
QTEST_MAIN(HomeTests)
#include "home_tests.moc"
