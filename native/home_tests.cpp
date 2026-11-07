#include "backend.h"
#include "i18n.h"
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
#include <QPdfWriter>
#include <QPainter>
#include <QDesktopServices>

class FolderReceiver : public QObject {
    Q_OBJECT
public:
    QList<QUrl> urls;
    FolderReceiver(){QDesktopServices::setUrlHandler("file",this,"opened");}
    ~FolderReceiver(){QDesktopServices::unsetUrlHandler("file");}
public slots:
    void opened(const QUrl &url){urls.append(url);}
};

class LocalizedEngine : public QQmlApplicationEngine {
public:
    QTemporaryDir settings;
    Languages languages;
    ~LocalizedEngine(){for(auto object:rootObjects())delete object;}
    LocalizedEngine():languages(settings.path(),QStringLiteral(VETUST_SOURCE)+"/../translations"){
        rootContext()->setContextProperty("languages",&languages);
        connect(&languages,&Languages::changed,this,&QQmlEngine::retranslate);
    }
};

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
    void unifiedCatalogPreview(){
        const auto source=qEnvironmentVariable("VETUST_CATALOG_PREVIEW"),shots=qEnvironmentVariable("VETUST_SCREENSHOT_DIR");
        if(source.isEmpty()||shots.isEmpty())QSKIP("Vista previa opcional con una instalación local");
        QTemporaryDir temporary;QVERIFY(temporary.isValid());QVERIFY(QDir().mkpath(temporary.path()+"/config"));
        for(const auto &name:QDir(source+"/config").entryList(QDir::Files))QVERIFY(QFile::copy(source+"/config/"+name,temporary.path()+"/config/"+name));
        Backend backend(temporary.path());LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",&backend);engine.addImageProvider("art",new ArtProvider(temporary.path(),backend.assets()));
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        engine.languages.setLanguage("es");QVERIFY(QMetaObject::invokeMethod(window,"navigate",Q_ARG(QVariant,QVariant("all"))));QTest::qWait(500);
        auto filter=window->findChild<QObject*>("catalogEngineFilter");QVERIFY(filter);
        for(int index=0;index<5;++index){
            filter->setProperty("currentIndex",index);QVERIFY(QMetaObject::invokeMethod(filter,"activated",Q_ARG(int,index)));QTest::qWait(700);
            const auto motor=filter->property("currentValue").toString();qInfo()<<motor<<backend.unifiedCatalog("",motor,0)["count"];
            QVERIFY(window->grabWindow().save(shots+"/catalog-"+(motor.isEmpty()?"all":motor)+".png"));
        }
        window->resize(1050,720);filter->setProperty("currentIndex",0);QVERIFY(QMetaObject::invokeMethod(filter,"activated",Q_ARG(int,0)));QTest::qWait(500);
        QVERIFY(window->grabWindow().save(shots+"/catalog-small.png"));
        for(const auto &entry:warnings)for(const auto &warning:qvariant_cast<QList<QQmlError>>(entry[0]))QFAIL(qPrintable(warning.description()));
    }
    void unifiedCatalogAndDetails(){
        Fixture f;const auto lib=f.temp.path()+"/collections";
        auto put=[](const QString &path,const QByteArray &data){QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);if(!file.open(QIODevice::WriteOnly))qFatal("No se pudo preparar la prueba");file.write(data);};
        put(lib+"/library.json","{}");put(lib+"/games/scummvm/Adventure/game.dat","game");
        put(lib+"/games/atari2600/roms/a.bin","rom");
        QJsonObject atariVariant{{"id","rom"},{"files",QJsonArray{"roms/a.bin"}}};
        put(lib+"/games/atari2600/metadata/catalog.json",QJsonDocument(QJsonObject{{"format",1},{"games",QJsonArray{QJsonObject{{"id",Seed},{"title","Monkey Island"},{"variants",QJsonArray{atariVariant}}},QJsonObject{{"id","hidden"},{"title","Hidden Atari"},{"variants",QJsonArray{atariVariant}}}}}}).toJson());
        put(lib+"/games/mame/roms/a.zip","rom");
        QJsonArray arcadeGames;
        for(const auto &id:{"playable","missing","preliminary"}){
            const QJsonObject variant{{"id",id},{"title",id},{"playable",QString(id)!="missing"},{"status",QString(id)=="preliminary"?"preliminary":"good"}};
            arcadeGames<<QJsonObject{{"id",id},{"title",id},{"variants",QJsonArray{variant}}};
        }
        put(lib+"/games/mame/metadata/catalog.json",QJsonDocument(QJsonObject{{"format",1},{"engine","0.241"},{"games",arcadeGames}}).toJson());
        f.backend->scumm()->setLibrary(lib);f.backend->atari()->setLibrary(lib);f.backend->arcade()->setLibrary(lib);
        f.backend->atari()->setHidden("hidden",true);
        auto result=f.backend->unifiedCatalog("","",0);QCOMPARE(result["count"].toInt(),43);QCOMPARE(result["pages"].toInt(),2);
        QCOMPARE(result["games"].toList().size(),28);
        QCOMPARE(f.backend->unifiedCatalog("","dosbox",1)["games"].toList().size(),12);
        QCOMPARE(f.backend->unifiedCatalog("","dosbox",0,"Adventure")["count"].toInt(),4);
        QCOMPARE(f.backend->unifiedCatalog("","dosbox",0,"",true)["count"].toInt(),0);
        QCOMPARE(f.backend->unifiedCatalog("JUEGÓ","dosbox",0)["count"].toInt(),39);
        QCOMPARE(f.backend->unifiedCatalog("","arcade",0)["count"].toInt(),1);
        QCOMPARE(f.backend->unifiedCatalog("","scummvm",0)["count"].toInt(),1);
        QCOMPARE(f.backend->unifiedCatalog("absent","",20)["page"].toInt(),0);
        result=f.backend->unifiedCatalog("Monkey Island","",0);QCOMPARE(result["count"].toInt(),2);
        QVERIFY(result["games"].toList()[0].toMap()["key"]!=result["games"].toList()[1].toMap()["key"]);
        f.backend->arcade()->setHidden("playable",true);QCOMPARE(f.backend->unifiedCatalog("","arcade",0)["count"].toInt(),0);f.backend->arcade()->setHidden("playable",false);
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->requestActivate();
        QVERIFY(QMetaObject::invokeMethod(window,"navigate",Q_ARG(QVariant,QVariant("all"))));
        QTRY_VERIFY(window->findChild<QQuickItem*>("unifiedCatalogGrid"));
        auto grid=window->findChild<QQuickItem*>("unifiedCatalogGrid");QTRY_COMPARE(grid->property("count").toInt(),28);
        auto filter=window->findChild<QObject*>("catalogEngineFilter");QVERIFY(filter);
        filter->setProperty("currentIndex",3);QVERIFY(QMetaObject::invokeMethod(filter,"activated",Q_ARG(int,3)));QTRY_COMPARE(grid->property("count").toInt(),1);
        for(const auto &motor:{"scummvm","atari","arcade","dosbox"}){
            const QString id=QString(motor)=="scummvm"?"Adventure":QString(motor)=="arcade"?"playable":Seed;
            QVERIFY(QMetaObject::invokeMethod(window,"openCatalogGame",Q_ARG(QVariant,QVariant(motor)),Q_ARG(QVariant,QVariant(id))));
            const QString name=QString(motor)=="scummvm"?"scummDetails":QString(motor)=="atari"?"atariDetails":QString(motor)=="arcade"?"arcadeDetails":"gameDetails";
            QTRY_VERIFY(window->findChild<QObject*>(name));auto details=window->findChild<QObject*>(name);QTRY_VERIFY(details->property("opened").toBool());
            QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!details->property("visible").toBool());QCOMPARE(window->property("view").toString(),QString("all"));
        }
        for(const auto &entry:warnings)for(const auto &warning:qvariant_cast<QList<QQmlError>>(entry[0]))QFAIL(qPrintable(warning.description()));
    }
    void independentLibraryFolders(){
        Fixture f;
        const QString lib=f.temp.path()+"/source",other=f.temp.path()+"/other";
        auto write=[&](const QString &name,const QByteArray &bytes){QDir().mkpath(QFileInfo(name).absolutePath());QFile file(name);QVERIFY(file.open(QIODevice::WriteOnly));file.write(bytes);};
        auto read=[](const QString &name){QFile file(name);if(!file.open(QIODevice::ReadOnly))return QByteArray();return file.readAll();};
        write(lib+"/library.json","{}");
        for(const auto &part:{"dosbox/archives","scummvm","atari2600/roms","mame/roms"})QVERIFY(QDir().mkpath(other+'/'+part));
        write(other+"/magazines/revistas.json",R"({"format":1,"series":[{"id":"Test","title":"Test","kind":"pdf"}],"entries":[{"id":"test-pdf","kind":"pdf","series":"Test","path":"issue.pdf"}]})");
        write(other+"/magazines/issue.pdf","test");
        write(f.temp.path()+"/config/atari.json",R"({"fullscreen":false,"crt":true,"favorites":{"keep":true}})");
        write(f.temp.path()+"/config/arcade.json",R"({"fullscreen":false,"artwork":false,"hidden":{"keep":true}})");
        QVariantMap paths{{"dosbox",other+"/dosbox"},{"atari2600",other+"/atari2600"},{"mame",other+"/mame"},{"magazines",other+"/magazines"},{"scummvm",other+"/scummvm"}};
        const auto found=f.backend->inspectLibraries(lib,paths);QCOMPARE(found.size(),5);
        for(const auto &row:found)if(paths.contains(row.toMap()["key"].toString()))QVERIFY(row.toMap()["available"].toBool());
        const auto before=read(f.temp.path()+"/config/storage.json");
        auto invalid=paths;invalid["mame"]=other+"/magazines";
        QVERIFY(!f.backend->saveStorage(QUrl::fromLocalFile(lib),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl::fromLocalFile(other+"/scummvm"),invalid).isEmpty());
        QCOMPARE(read(f.temp.path()+"/config/storage.json"),before);
        QCOMPARE(f.backend->saveStorage(QUrl::fromLocalFile(lib),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl::fromLocalFile(other+"/scummvm"),paths),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!f.backend->busy(),10000);
        Storage stored(f.temp.path());
        QCOMPARE(stored.dosboxPath("archives"),other+"/dosbox/archives");
        QCOMPARE(stored.dosboxPath("versiones-especiales.zip"),other+"/dosbox/versiones-especiales.zip");
        QCOMPARE(f.backend->atari()->settings()["resolvedFolder"].toString(),other+"/atari2600");
        QCOMPARE(f.backend->arcade()->settings()["resolvedFolder"].toString(),other+"/mame");
        QVERIFY(!f.backend->atari()->settings()["fullscreen"].toBool());QVERIFY(f.backend->atari()->settings()["crt"].toBool());
        QVERIFY(!f.backend->arcade()->settings()["artwork"].toBool());
        QVERIFY(QJsonDocument::fromJson(read(f.temp.path()+"/config/atari.json")).object()["favorites"].toObject()["keep"].toBool());
        QVERIFY(QJsonDocument::fromJson(read(f.temp.path()+"/config/arcade.json")).object()["hidden"].toObject()["keep"].toBool());
        Magazines magazines;magazines.load(lib,stored.data["magazines"].toString());QCOMPARE(magazines.pdf("test-pdf"),other+"/magazines/issue.pdf");
        QSignalSpy pdf(f.backend.get(),&Backend::extraReady);f.backend->openMagazine("test-pdf");QCOMPARE(pdf.size(),1);
        QCOMPARE(pdf[0][0].toUrl().toLocalFile(),other+"/magazines/issue.pdf");
        QVariantMap automatic;for(auto it=paths.begin();it!=paths.end();++it)automatic[it.key()]="";
        QCOMPARE(f.backend->saveStorage(QUrl::fromLocalFile(lib),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl(),automatic),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!f.backend->busy(),10000);
        Storage reset(f.temp.path());QVERIFY(!reset.data.contains("dosbox"));QVERIFY(!reset.data.contains("magazines"));
        QCOMPARE(reset.dosboxPath("archives"),lib+"/games/dosbox/archives");
        QCOMPARE(f.backend->atari()->settings()["resolvedFolder"].toString(),lib+"/games/atari2600");
        QCOMPARE(f.backend->arcade()->settings()["resolvedFolder"].toString(),lib+"/games/mame");
    }
    void sectionBackgrounds(){
        Fixture f;
        const auto root=f.temp.path();
        const QString path=root+"/fondo de prueba.png";
        QImage picture(80,60,QImage::Format_RGB32);
        picture.fill(Qt::red);
        QVERIFY(picture.save(path));
        const auto source=QUrl::fromLocalFile(path);
        QVERIFY(f.backend->setSectionBackground("all",source).isEmpty());
        QVERIFY(f.backend->setSectionBackground("all",QUrl::fromLocalFile(root+"/missing.png")).size()>0);
        QVERIFY(f.backend->setSectionBackground("../all",source).size()>0);
        QVERIFY(QFile::remove(path));
        f.backend.reset();
        f.backend=std::make_unique<Backend>(root);
        QCOMPARE(f.backend->sectionBackgrounds().keys(),QStringList{"all"});
        LocalizedEngine engine;
        engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.addImageProvider("art",new ArtProvider(root,{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto window=engine.rootObjects().first();
        auto dialog=window->findChild<QObject*>("backgroundsDialog");
        QVERIFY(dialog);
        auto image=dialog->findChild<QQuickItem*>("selectedBackgroundPreview");
        QVERIFY(image);
        QVERIFY(QMetaObject::invokeMethod(dialog,"begin",Q_ARG(QVariant,QVariant("all"))));
        const auto url=f.backend->sectionBackgrounds()["all"].toUrl();
        QTRY_COMPARE(image->property("source").toUrl(),url);
        QTRY_COMPARE(image->property("status").toInt(),1);
        QVERIFY(image->isVisible());
        QVERIFY(image->property("paintedWidth").toReal()<=image->width()+1);
        QVERIFY(image->property("paintedHeight").toReal()<=image->height()+1);
        picture.fill(Qt::blue);
        QVERIFY(picture.save(path));
        QVERIFY(f.backend->setSectionBackground("all",source).isEmpty());
        QTRY_VERIFY(image->property("source").toUrl()!=url);
        QTRY_COMPARE(image->property("status").toInt(),1);
        QVERIFY(f.backend->setSectionBackground("all",{}).isEmpty());
        QTRY_VERIFY(image->property("source").toUrl().path().endsWith("/retro-library.jpg"));
        QVERIFY(!QFile::exists(url.toLocalFile()));
        const auto catalogDefault=image->property("source").toUrl();
        QVERIFY(QMetaObject::invokeMethod(dialog,"begin",Q_ARG(QVariant,QVariant("home"))));
        QTRY_COMPARE(image->property("source").toUrl(),catalogDefault);
        f.backend->refreshHome(true);
        QCOMPARE(image->property("source").toUrl(),catalogDefault);
        QVERIFY(f.backend->setSectionBackground("home",source).isEmpty());
        QTRY_COMPARE(image->property("source").toUrl(),f.backend->sectionBackgrounds()["home"].toUrl());
        QVERIFY(f.backend->setSectionBackground("home",{}).isEmpty());
        QTRY_COMPARE(image->property("source").toUrl(),catalogDefault);
        QVERIFY(QMetaObject::invokeMethod(dialog,"begin",Q_ARG(QVariant,QVariant("magazines"))));
        const auto magazineDefault=image->property("source").toUrl();
        QVERIFY(magazineDefault.path().endsWith("/assets/magazines.jpg"));
        QTRY_COMPARE(image->property("status").toInt(),1);
        QVERIFY(f.backend->setSectionBackground("magazines",source).isEmpty());
        QTRY_COMPARE(image->property("source").toUrl(),f.backend->sectionBackgrounds()["magazines"].toUrl());
        QVERIFY(f.backend->setSectionBackground("magazines",{}).isEmpty());
        QTRY_COMPARE(image->property("source").toUrl(),magazineDefault);
    }
    void storageDialogLayout(){
        Fixture f;LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        auto dialog=window->findChild<QObject*>("storageDialog");QVERIFY(dialog);
        auto save=window->findChild<QQuickItem*>("saveStorageButton");QVERIFY(save);
        auto advanced=window->findChild<QObject*>("storageAdvanced");QVERIFY(advanced);
        for(const QSize size:{QSize(1480,920),QSize(1050,720)}){
            window->resize(size);QVERIFY(QMetaObject::invokeMethod(dialog,"begin"));QTest::qWait(250);
            const auto preview=qEnvironmentVariable("VETUST_LIBRARY_PREVIEW");
            if(!preview.isEmpty()){
                const auto rows=f.backend->inspectLibraries(preview,{});
                for(const auto &row:rows){
                    const auto values=row.toMap();const auto key=values["key"].toString();
                    auto field=window->findChild<QObject*>(key=="scummvm"?"scummFolder":key+"Folder");QVERIFY(field);
                    field->setProperty("text",values["path"]);
                }
                QTest::qWait(250);
            }
            QCOMPARE(dialog->property("detected").toList().size(),5);
            QTRY_COMPARE(dialog->property("width").toInt(),window->width());QTRY_COMPARE(dialog->property("height").toInt(),window->height());
            QCOMPARE(dialog->property("x").toInt(),0);QCOMPARE(dialog->property("y").toInt(),0);

            const auto pos=save->mapToScene({save->width()/2,save->height()/2});
            QVERIFY(pos.x()>0&&pos.x()<window->width());QVERIFY(pos.y()>0&&pos.y()<window->height());
            advanced->setProperty("checked",true);QTest::qWait(100);
            QVERIFY(save->mapToScene({0,save->height()}).y()<=window->height());
            advanced->setProperty("checked",false);
            const auto shots=qEnvironmentVariable("VETUST_SCREENSHOT_DIR");
            if(!shots.isEmpty()){
                for(const auto &lang:{"en","es","pt_BR"}){
                    engine.languages.setLanguage(lang);QTest::qWait(150);
                    QVERIFY(window->grabWindow().save(shots+"/storage-"+lang+"-"+QString::number(size.width())+".png"));
                }
            }
            auto close=window->findChild<QQuickItem*>("closeStorageButton");QVERIFY(close);
            QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,close->mapToScene({close->width()/2,close->height()/2}).toPoint());QTRY_VERIFY(!dialog->property("visible").toBool());
            QVERIFY(QMetaObject::invokeMethod(dialog,"begin"));
            QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!dialog->property("visible").toBool());
            auto emulator=window->findChild<QObject*>("emulatorDialog");QVERIFY(emulator);
            QVERIFY(QMetaObject::invokeMethod(emulator,"begin",Q_ARG(QVariant,QVariant(true))));
            QTRY_COMPARE(emulator->property("width").toInt(),window->width());QTRY_COMPARE(emulator->property("height").toInt(),window->height());
            auto closeEmulator=window->findChild<QQuickItem*>("closeEmulatorButton");QVERIFY(closeEmulator);
            QTRY_VERIFY(closeEmulator->mapToScene({closeEmulator->width()/2,closeEmulator->height()/2}).x()<window->width());
            QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,closeEmulator->mapToScene({closeEmulator->width()/2,closeEmulator->height()/2}).toPoint());
            QTRY_VERIFY(!emulator->property("visible").toBool());
            QVERIFY(QMetaObject::invokeMethod(emulator,"begin",Q_ARG(QVariant,QVariant(true))));
            QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!emulator->property("visible").toBool());
        }
        for(const auto &entry:warnings)for(const auto &warning:qvariant_cast<QList<QQmlError>>(entry[0]))QFAIL(qPrintable(warning.description()));
    }
    void openDosFolderSelectedEdition(){
        Fixture f;FolderReceiver receiver;
        const QString original=f.temp.path()+"/data/games/2";
        QVERIFY(QDir().mkpath(original));
        QFile save(original+"/SAVE.DAT");QVERIFY(save.open(QIODevice::WriteOnly));save.write("partida conservada");save.close();
        f.backend->selectGame("2");f.backend->openGameFolder();
        QCOMPARE(receiver.urls.size(),1);QCOMPARE(receiver.urls.last().toLocalFile(),f.temp.path()+"/spanish");
        f.backend->setSpanish(false);f.backend->openGameFolder();
        QCOMPARE(receiver.urls.size(),2);QCOMPARE(QDir::cleanPath(receiver.urls.last().toLocalFile()),original);
        QVERIFY(save.open(QIODevice::ReadOnly));QCOMPARE(save.readAll(),QByteArray("partida conservada"));
        QVERIFY(!f.backend->busy());QVERIFY(f.row("recent").isEmpty());
        f.backend->selectGame("3");f.backend->openGameFolder();
        QCOMPARE(receiver.urls.size(),2);QVERIFY(f.backend->status().contains("No configuration"));
    }
    void openDosFolderRestoresWithoutEmulator_data(){
        QTest::addColumn<bool>("external");
        QTest::newRow("default")<<false;QTest::newRow("external")<<true;
    }
    void openDosFolderRestoresWithoutEmulator(){
        QFETCH(bool,external);
        Fixture f;FolderReceiver receiver;f.backend.reset();QSqlDatabase::removeDatabase("vetust-native");
        const QString custom=f.temp.path()+(external?"/other/dosbox":"/source/Personalizaciones");QVERIFY(QDir().mkpath(custom));
        if(external){Storage storage(f.temp.path());QCOMPARE(storage.save(storage.library,storage.games,QString(),{{"dosbox",custom}}),QString());}
        int error=0;zip_t *archive=zip_open(QFile::encodeName(custom+(external?"/versiones-especiales.zip":"/juegos.zip")).constData(),ZIP_CREATE|ZIP_TRUNCATE,&error);QVERIFY(archive);
        static const char payload[]="contenido";
        for(const QString &name:QStringList{"2-es/game/.ready","2-es/game/SAVE.DAT"}){
            auto source=zip_source_buffer(archive,payload,sizeof(payload)-1,0);QVERIFY(source);QVERIFY(zip_file_add(archive,name.toUtf8().constData(),source,ZIP_FL_ENC_UTF_8)>=0);
        }
        QCOMPARE(zip_close(archive),0);
        QJsonObject profile{{"preparedRoot","2-es"},{"editionDirectory","${GAMES}/2-es/game"},{"config","[autoexec]\nmount c \"${GAMES}/2-es/game\"\nc:\nGAME.EXE\n"}};
        QFile profiles(f.temp.path()+"/data/spanish-profiles.json");QVERIFY(profiles.open(QIODevice::WriteOnly));profiles.write(QJsonDocument(QJsonObject{{"2",profile}}).toJson());profiles.close();
        f.backend=std::make_unique<Backend>(f.temp.path());f.backend->selectGame("2");f.backend->openGameFolder();
        QTRY_VERIFY(!f.backend->busy());QCOMPARE(receiver.urls.size(),1);
        const QString restored=f.temp.path()+"/data/games/2-es/game";
        QCOMPARE(receiver.urls.last().toLocalFile(),restored);QVERIFY(QFileInfo::exists(restored+"/SAVE.DAT"));
        QVERIFY(!QFileInfo::exists(f.temp.path()+"/logs/2-es-native.log"));QVERIFY(f.row("recent").isEmpty());
    }
    void initTestCase(){QQuickStyle::setStyle("Basic");}
    void homeCountIgnoresFilters(){
        Fixture f;
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        QQuickItem *label=nullptr;
        QTRY_VERIFY((label=findVisual(window->contentItem(),"catalogTotalLabel"))!=nullptr);
        QCOMPARE(label->property("text").toString(),QString("40 stories to discover"));
        f.backend->filter("Monkey Island","","all",false);QCOMPARE(f.backend->count(),1);
        QCOMPARE(label->property("text").toString(),QString("40 stories to discover"));
        f.backend->filter("","","favorites",false);QCOMPARE(f.backend->count(),0);
        QCOMPARE(label->property("text").toString(),QString("40 stories to discover"));
        QCOMPARE(engine.languages.setLanguage("es"),QString());
        QCOMPARE(label->property("text").toString(),QString("40 historias por descubrir"));
    }
    void scummLibraryUi(){
        Fixture f;
        const QString base=f.temp.path()+"/source/scummvm";
        QDir().mkpath(base+"/Edición A");QDir().mkpath(base+"/Otra carpeta");
        QImage cover(60,90,QImage::Format_RGB32);cover.fill(Qt::darkCyan);QVERIFY(cover.save(base+"/Edición A/portada.jpg"));
        {QPdfWriter writer(base+"/Edición A/manual.pdf");QPainter painter(&writer);painter.drawText(100,100,"Manual ScummVM");}
        ScummConfig config{{"scummvm",{{"fullscreen","true"}}},
            {"test-a",{{"gameid","monkey"},{"path",base+"/Edición A"},{"description","Monkey Island (Spanish)"}}},
            {"test-b",{{"gameid","monkey"},{"platform","amiga"},{"path",base+"/Edición A"},{"description","Monkey Island (Amiga)"}}}};
        QFile file(f.backend->scumm()->configPath());QVERIFY(file.open(QIODevice::WriteOnly));file.write(ScummLibrary::serialize(config));file.close();
        f.backend->scumm()->setLibrary(f.temp.path()+"/source");
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        window->setProperty("view","scummvm");QTest::qWait(100);
        auto grid=window->findChild<QQuickItem*>("scummGrid");auto page=window->findChild<QObject*>("scummView");QVERIFY(grid&&page);
        QTRY_COMPARE(grid->property("count").toInt(),2);QVERIFY(grid->isVisible());
        auto search=window->findChild<QQuickItem*>("searchField");QVERIFY(search);search->forceActiveFocus();
        for(char ch:QByteArray("Monkey"))QTest::keyClick(window,ch);QTRY_COMPARE(grid->property("count").toInt(),1);QVERIFY(search->hasActiveFocus());
        const auto row=f.backend->scumm()->games().first();QVERIFY(QMetaObject::invokeMethod(page,"showGame",Q_ARG(QVariant,row)));QTest::qWait(80);
        auto choice=window->findChild<QObject*>("scummEdition");auto play=window->findChild<QObject*>("playScummVM");QVERIFY(choice&&play);
        QCOMPARE(choice->property("currentIndex").toInt(),-1);QVERIFY(!play->property("enabled").toBool());
        if(f.backend->scumm()->available()){
            choice->setProperty("currentIndex",0);QVERIFY(QMetaObject::invokeMethod(choice,"activated",Q_ARG(int,0)));
            QTRY_VERIFY(play->property("enabled").toBool());
        }
        auto manual=window->findChild<QQuickItem*>("scummManual");QVERIFY(manual);QVERIFY(manual->isVisible());
        QVERIFY(QMetaObject::invokeMethod(manual,"clicked"));
        auto pdfDialog=window->findChild<QObject*>("documentDialog");auto pdf=window->findChild<QObject*>("pdfDocument");
        QTRY_VERIFY(pdfDialog->property("visible").toBool());
        QCOMPARE(pdf->property("source").toUrl().toLocalFile(),base+"/Edición A/manual.pdf");
        auto reader=window->findChild<QObject*>("documentView");QTRY_VERIFY(reader->property("ready").toBool());
        QVERIFY(warnings.isEmpty());
        QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!pdfDialog->property("visible").toBool());
        window->resize(1050,720);QTest::qWait(100);
        QVERIFY(search->mapToScene(QPointF(search->width(),0)).x()<=window->width());
        QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!page->property("detailsOpen").toBool());
        QVERIFY(QMetaObject::invokeMethod(page,"showGame",Q_ARG(QVariant,row)));
        auto hide=window->findChild<QObject*>("hideScummGame");auto showHidden=window->findChild<QObject*>("showHiddenScummVM");QVERIFY(hide&&showHidden);
        QVERIFY(QMetaObject::invokeMethod(hide,"clicked"));QTRY_COMPARE(grid->property("count").toInt(),0);
        QVERIFY(!page->property("detailsOpen").toBool());QCOMPARE(search->property("text").toString(),QString("Monkey"));
        showHidden->setProperty("checked",true);QVERIFY(QMetaObject::invokeMethod(showHidden,"clicked"));QTRY_COMPARE(grid->property("count").toInt(),1);
        const auto hiddenRow=f.backend->scumm()->games().first();QVERIFY(hiddenRow.toMap()["hidden"].toBool());
        QVERIFY(QMetaObject::invokeMethod(page,"showGame",Q_ARG(QVariant,hiddenRow)));QVERIFY(QMetaObject::invokeMethod(hide,"clicked"));
        QVERIFY(!f.backend->scumm()->games().first().toMap()["hidden"].toBool());
        showHidden->setProperty("checked",false);QVERIFY(QMetaObject::invokeMethod(showHidden,"clicked"));QTRY_COMPARE(grid->property("count").toInt(),1);
        for(const auto &entry:warnings)for(const auto &warning:qvariant_cast<QList<QQmlError>>(entry[0]))
            QVERIFY2(warning.description()=="QML PdfDocument: Cannot open: " ||
                     warning.description()=="QML PdfPageImage: document and source properties in conflict: preferring document source QUrl(\"\")" ||
                     warning.description()=="QML PdfPageImage: Protocol \"\" is unknown",qPrintable(warning.description()));
    }
    void arcadeLibraryUi(){
        Fixture f;const QString library=qEnvironmentVariable("VETUST_ARCADE_LIBRARY");
        const QString base=library.isEmpty()?f.temp.path()+"/source/mame":library;
        auto put=[&](const QString &path,const QByteArray &bytes){QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(bytes);};
        if(library.isEmpty()){
            QDir().mkpath(base+"/roms");QDir().mkpath(base+"/flyers");QImage art(100,160,QImage::Format_RGB32);art.fill(Qt::darkCyan);QVERIFY(art.save(base+"/flyers/sf2.png"));
            QJsonObject v{{"id","sf2"},{"title","Street Fighter II"},{"playable",true},{"status","good"},{"audit","good"},{"cover","flyers/sf2.png"},{"savestate",true},{"history","History"}};
            QJsonObject bad=v;bad["id"]="sf2bad";bad["playable"]=false;bad["audit"]="bad";
            put(base+"/catalog.json",QJsonDocument(QJsonObject{{"format",1},{"engine","0.241"},{"verified",1},{"games",QJsonArray{QJsonObject{{"id","sf2"},{"title","Street Fighter II"},{"variants",QJsonArray{v,bad}}}}}}).toJson());
        }
        QVERIFY(f.backend->arcade()->configure(QUrl::fromLocalFile(base),false,true).isEmpty());
        const auto exe=f.temp.path()+"/runtime/mame-0.241/mame";put(exe,"#!/bin/sh\nexit 0\n");QFile::setPermissions(exe,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
        put(f.temp.path()+"/helpers/mame_launch.py","from pathlib import Path\nPath('clicked-game').write_text('ok')\n");
        f.backend->arcade()->enter();
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->setProperty("view","arcade");QTest::qWait(150);
        auto grid=window->findChild<QQuickItem*>("arcadeGrid");auto page=window->findChild<QObject*>("arcadeView");QVERIFY(grid&&page);QVERIFY(grid->property("count").toInt()>0);
        auto search=window->findChild<QQuickItem*>("searchField");search->forceActiveFocus();
        for(char ch:QByteArray("Street Fighter II"))QTest::keyClick(window,ch);QVERIFY(search->hasActiveFocus());QTRY_VERIFY(grid->property("count").toInt()>0);
        QVariant row;for(const auto &g:f.backend->arcade()->games())if(g.toMap()["id"].toString()=="sf2")row=g;QVERIFY(row.isValid());
        const auto shots=qEnvironmentVariable("VETUST_SCREENSHOT_DIR");
        for(const QString lang:QStringList{"en","es","pt_BR"}){
            engine.languages.setLanguage(lang);QTest::qWait(150);
            if(!shots.isEmpty())QVERIFY(window->grabWindow().save(shots+"/arcade-"+lang+".png"));
            QVERIFY(QMetaObject::invokeMethod(page,"showGame",Q_ARG(QVariant,row)));QTest::qWait(150);
            if(!shots.isEmpty())QVERIFY(window->grabWindow().save(shots+"/arcade-detail-"+lang+".png"));
            auto close=window->findChild<QObject*>("closeArcadeDetails");QVERIFY(close);QVERIFY(QMetaObject::invokeMethod(close,"clicked"));
        }
        window->resize(1050,720);QTest::qWait(150);QVERIFY(search->mapToScene(QPointF(search->width(),0)).x()<=window->width());
        if(!shots.isEmpty())QVERIFY(window->grabWindow().save(shots+"/arcade-small.png"));
        QVERIFY(QMetaObject::invokeMethod(page,"showGame",Q_ARG(QVariant,row)));QTest::qWait(100);
        auto image=findVisual(window->contentItem(),"arcadeDetailPlayImage");QVERIFY(image);QVERIFY(image->isEnabled());
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,image->mapToScene(QPointF(image->width()/2,image->height()/2)).toPoint());
        QTRY_VERIFY(QFileInfo(f.temp.path()+"/data/mame/0.241/clicked-game").isFile());QTRY_VERIFY(!f.backend->busy());QVERIFY(!page->property("detailsOpen").toBool());
        for(const auto &entry:warnings)for(const auto &warning:qvariant_cast<QList<QQmlError>>(entry[0]))QFAIL(qPrintable(warning.description()));
    }
    void interfaceLanguageUi(){
        Fixture f;f.backend->selectGame("2");f.backend->setSpanish(false);LocalizedEngine engine;
        connect(&engine.languages,&Languages::changed,f.backend.get(),&Backend::retranslate);
        engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        window->resize(1480,920);
        auto dialog=window->findChild<QObject*>("emulatorDialog");QVERIFY(dialog);
        auto search=window->findChild<QObject*>("searchField");QVERIFY(search);
        auto picker=window->findChild<QObject*>("languagePicker");QVERIFY(picker);
        auto style=window->findChild<QObject*>("imageStylePicker");QVERIFY(style);
        auto editions=window->findChild<QObject*>("editionPicker");QVERIFY(editions);
        QCOMPARE(editions->property("currentText").toString(),QString("original"));
        const auto edition=f.backend->selected()["spanish"];
        search->setProperty("text","Monkey");
        QVERIFY(QMetaObject::invokeMethod(window,"applyFilter"));
        const int count=f.backend->count();
        QVERIFY(QMetaObject::invokeMethod(dialog,"begin",Q_ARG(QVariant,QVariant(true))));
        QTest::qWait(150);
        QCOMPARE(dialog->property("title").toString(),QString("Global emulation settings"));
        QCOMPARE(style->property("displayText").toString(),QString("Sharp"));
        const auto draft=dialog->property("draft").toMap();
        const QString screenshots=qEnvironmentVariable("VETUST_SCREENSHOT_DIR");
        if(!screenshots.isEmpty())QVERIFY(window->grabWindow().save(screenshots+"/settings-en.png"));
        picker->setProperty("currentIndex",1);
        QVERIFY(QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,1)));
        QTRY_COMPARE(engine.languages.current(),QString("es"));
        QCOMPARE(dialog->property("title").toString(),QString("Ajustes globales de emulación"));
        QCOMPARE(style->property("displayText").toString(),QString("Nítido"));
        QCOMPARE(search->property("text").toString(),QString("Monkey"));
        QCOMPARE(f.backend->count(),count);QCOMPARE(f.backend->selected()["spanish"],edition);
        QCOMPARE(editions->property("currentText").toString(),QString("original"));
        QCOMPARE(dialog->property("draft").toMap(),draft);
        QCOMPARE(f.row("adventure")["title"].toString(),QString("Aventuras gráficas"));
        QTest::qWait(150);
        if(!screenshots.isEmpty())QVERIFY(window->grabWindow().save(screenshots+"/settings-es.png"));
        picker->setProperty("currentIndex",0);
        QVERIFY(QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,0)));
        QCOMPARE(engine.languages.current(),QString("en"));
        QCOMPARE(f.row("adventure")["title"].toString(),QString("Graphic adventures"));
        QCOMPARE(dialog->property("title").toString(),QString("Global emulation settings"));
        QCOMPARE(dialog->property("draft").toMap(),draft);
    }
    void pdfAutomaticLayout(){
        Fixture f;LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);
        auto document=window->findChild<QObject*>("pdfDocument");QVERIFY(document);
        auto reader=window->findChild<QQuickItem*>("documentView");QVERIFY(reader);
        const QList<QSizeF> sizes{{210,280},{280,210},{210,210},{210,280},{210,297}};
        for(int index=0;index<sizes.size();++index){
            const int pages=index==3?1:3;const bool expected=index==0||index==4;
            const QString path=f.temp.path()+QString("/layout-%1.pdf").arg(index);
            {
                QPdfWriter writer(path);writer.setResolution(72);writer.setPageSize(QPageSize(sizes[index],QPageSize::Millimeter));
                QPainter painter(&writer);
                for(int page=0;page<pages;++page){if(page)QVERIFY(writer.newPage());painter.drawText(20,40,QString::number(page+1));}
            }
            document->setProperty("source",QUrl::fromLocalFile(path));
            QTRY_VERIFY(reader->property("ready").toBool());
            QCOMPARE(reader->property("doublePage").toBool(),expected);
            QCOMPARE(reader->property("currentPage").toInt(),0);
            QVERIFY(!reader->property("hasSecondPage").toBool());
            reader->setProperty("doublePage",!expected);
            if(pages>1)QVERIFY(QMetaObject::invokeMethod(reader,"nextPage"));
            window->resize(1100+index*10,760);QTest::qWait(10);
            QCOMPARE(reader->property("doublePage").toBool(),!expected);
        }
    }
    void pdfSpreads_data(){
        QTest::addColumn<int>("pages");
        QTest::newRow("portada")<<1;
        QTest::newRow("dos-paginas")<<2;
        QTest::newRow("impar")<<5;
        QTest::newRow("par")<<6;
    }
    void pdfSpreads(){
        QFETCH(int,pages);
        Fixture f;const QString pdf=f.temp.path()+"/reader.pdf";
        {
            QPdfWriter writer(pdf);writer.setResolution(72);writer.setPageSize(QPageSize(QPageSize::A4));
            QPainter painter(&writer);
            for(int page=0;page<pages;++page){
                if(page>0){
                    writer.setPageSize(QPageSize(page==2?QSizeF(297,210):QSizeF(210,297),QPageSize::Millimeter));
                    QVERIFY(writer.newPage());
                }
                painter.fillRect(0,0,writer.width(),writer.height(),page%2?QColor("#e0c390"):QColor("#90c5d0"));
                painter.setPen(Qt::black);painter.setFont(QFont("Sans",28));
                painter.drawText(QRect(0,0,writer.width(),writer.height()),Qt::AlignCenter,QString("PDF test\nPage %1").arg(page+1));
            }
        }
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        QSignalSpy warnings(&engine,&QQmlEngine::warnings);
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->resize(1480,920);
        auto document=window->findChild<QObject*>("pdfDocument");QVERIFY(document);
        auto dialog=window->findChild<QObject*>("documentDialog");QVERIFY(dialog);
        auto reader=window->findChild<QQuickItem*>("documentView");QVERIFY(reader);
        auto picker=window->findChild<QObject*>("pdfPageMode");QVERIFY(picker);
        auto next=window->findChild<QQuickItem*>("pdfNext");QVERIFY(next);
        auto previous=window->findChild<QQuickItem*>("pdfPrevious");QVERIFY(previous);
        auto widthButton=window->findChild<QQuickItem*>("pdfFitWidth");QVERIFY(widthButton);
        auto fitButton=window->findChild<QQuickItem*>("pdfFitPage");QVERIFY(fitButton);
        auto click=[&](QQuickItem *button){QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,button->mapToScene({button->width()/2,button->height()/2}).toPoint());};
        document->setProperty("source",QUrl::fromLocalFile(pdf));
        QVERIFY(QMetaObject::invokeMethod(dialog,"open"));
        QTRY_VERIFY(dialog->property("opened").toBool());QTRY_VERIFY(reader->property("ready").toBool());
        QTRY_COMPARE(document->property("pageCount").toInt(),pages);
        auto left=window->findChild<QQuickItem*>("pdfLeftPage");QVERIFY(left);
        auto right=window->findChild<QQuickItem*>("pdfRightPage");QVERIFY(right);
        auto spread=window->findChild<QQuickItem*>("pdfSpread");QVERIFY(spread);
        auto scroll=window->findChild<QObject*>("pdfSpreadView");QVERIFY(scroll);
        QCOMPARE(reader->property("doublePage").toBool(),pages>1);
        QCOMPARE(reader->property("pageLabel").toString(),QString("1 / %1").arg(pages));
        QVERIFY(!reader->property("hasSecondPage").toBool());
        picker->setProperty("currentIndex",0);QVERIFY(QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,0)));
        QTest::keyClick(window,Qt::Key_Left);QCOMPARE(reader->property("currentPage").toInt(),0);
        for(int page=1;page<pages;++page){
            QTest::keyClick(window,Qt::Key_Right);QTRY_COMPARE(reader->property("currentPage").toInt(),page);
        }
        QTest::keyClick(window,Qt::Key_Right);QCOMPARE(reader->property("currentPage").toInt(),pages-1);
        for(int page=pages-2;page>=0;--page){
            QTest::keyClick(window,Qt::Key_Left);QTRY_COMPARE(reader->property("currentPage").toInt(),page);
        }
        picker->setProperty("currentIndex",1);QVERIFY(QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,1)));
        QTRY_VERIFY(reader->property("doublePage").toBool());
        QTRY_COMPARE(left->property("status").toInt(),1);
        QVERIFY(!right->isVisible());QVERIFY(!previous->isEnabled());
        QCOMPARE(picker->property("displayText").toString(),QString("Two pages"));
        QCOMPARE(engine.languages.setLanguage("es"),QString());
        QTRY_COMPARE(picker->property("displayText").toString(),QString("Doble página"));
        QVERIFY(reader->property("doublePage").toBool());
        reader->forceActiveFocus();
        for(int page=1;page<pages;page+=2){
            QTest::keyClick(window,Qt::Key_Right);QTRY_COMPARE(reader->property("firstPage").toInt(),page);
        }
        const int lastSpread=reader->property("firstPage").toInt();
        QTest::keyClick(window,Qt::Key_Right);QCOMPARE(reader->property("firstPage").toInt(),lastSpread);
        for(int page=lastSpread;page>0;page=std::max(0,page-2)){
            QTest::keyClick(window,Qt::Key_Left);QTRY_COMPARE(reader->property("firstPage").toInt(),std::max(0,page-2));
        }
        QTest::keyClick(window,Qt::Key_Left);QCOMPARE(reader->property("firstPage").toInt(),0);
        auto popup=qvariant_cast<QObject*>(picker->property("popup"));QVERIFY(popup);
        QVERIFY(QMetaObject::invokeMethod(popup,"open"));QTRY_VERIFY(popup->property("opened").toBool());
        QTest::keyClick(window,Qt::Key_Right);QCOMPARE(reader->property("firstPage").toInt(),0);
        QVERIFY(QMetaObject::invokeMethod(popup,"close"));QTRY_VERIFY(!popup->property("visible").toBool());
        next->forceActiveFocus();
        QTest::keyClick(window,Qt::Key_Right);QTRY_COMPARE(reader->property("firstPage").toInt(),std::min(1,pages-1));
        QTest::keyClick(window,Qt::Key_Left);QTRY_COMPARE(reader->property("firstPage").toInt(),0);
        const QString screenshots=qEnvironmentVariable("VETUST_SCREENSHOT_DIR");
        if(!screenshots.isEmpty()&&pages==6)QVERIFY(window->grabWindow().save(screenshots+"/pdf-cover.png"));
        int start=1;
        while(next->isEnabled()){
            click(next);
            QTRY_COMPARE(reader->property("firstPage").toInt(),start);
            const int end=std::min(start+1,pages-1);
            QCOMPARE(reader->property("lastPage").toInt(),end);
            QCOMPARE(reader->property("pageLabel").toString(),(end>start?QString("%1–%2 / %3").arg(start+1).arg(end+1).arg(pages):QString("%1 / %2").arg(start+1).arg(pages)));
            QTRY_COMPARE(left->property("status").toInt(),1);
            QCOMPARE(right->isVisible(),end>start);
            if(end>start){
                QTRY_COMPARE(right->property("status").toInt(),1);
                QCOMPARE(right->property("currentFrame").toInt(),end);
                QVERIFY(right->x()+right->width()<=reader->width()+1);
            }
            QVERIFY(spread->width()<=reader->width()+1);QVERIFY(spread->height()<=reader->height()+1);
            if(!screenshots.isEmpty()&&pages==6&&start==1){QTest::qWait(100);QVERIFY(window->grabWindow().save(screenshots+"/pdf-spread.png"));}
            start+=2;
        }
        QVERIFY(!reader->property("canNext").toBool());
        while(previous->isEnabled())click(previous);
        QTRY_COMPARE(reader->property("firstPage").toInt(),0);
        if(pages>=5){
            QVERIFY(QMetaObject::invokeMethod(reader,"showPage",Q_ARG(QVariant,QVariant(2))));
            QCOMPARE(reader->property("pageLabel").toString(),QString("2–3 / %1").arg(pages));
            picker->setProperty("currentIndex",0);QVERIFY(QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,0)));
            QTRY_COMPARE(reader->property("pageLabel").toString(),QString("3 / %1").arg(pages));
            click(next);QTRY_COMPARE(reader->property("currentPage").toInt(),3);
            picker->setProperty("currentIndex",1);QVERIFY(QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,1)));
            QTRY_COMPARE(reader->property("pageLabel").toString(),QString("4–5 / %1").arg(pages));
            click(widthButton);QVERIFY(reader->property("fitWidth").toBool());
            QTRY_VERIFY(spread->height()>reader->height());
            scroll->setProperty("contentY",40);QVERIFY(scroll->property("contentY").toDouble()>0);
            window->resize(1050,720);QTest::qWait(100);
            QVERIFY(spread->width()<=reader->width()+1);
            click(fitButton);QVERIFY(!reader->property("fitWidth").toBool());
            QTRY_VERIFY(spread->height()<=reader->height()+1);
            QCOMPARE(scroll->property("contentY").toDouble(),0.0);
        }
        QTest::keyClick(window,Qt::Key_Escape);QTRY_VERIFY(!dialog->property("visible").toBool());
        QTRY_VERIFY(document->property("source").toUrl().isEmpty());
        const int closedPage=reader->property("currentPage").toInt();
        QTest::keyClick(window,Qt::Key_Right);QTest::keyClick(window,Qt::Key_Left);
        QCOMPARE(reader->property("currentPage").toInt(),closedPage);
        document->setProperty("source",QUrl::fromLocalFile(pdf));QVERIFY(QMetaObject::invokeMethod(dialog,"open"));
        QTRY_VERIFY(reader->property("ready").toBool());
        QTRY_COMPARE(reader->property("firstPage").toInt(),0);
        QCOMPARE(reader->property("doublePage").toBool(),pages>1);
        auto close=window->findChild<QQuickItem*>("closePdfButton");QVERIFY(close);click(close);
        QTRY_VERIFY(!dialog->property("visible").toBool());
        // Qt avisa al liberar un documento asignando la URL vacía, también en el visor original.
        for(const auto &entry:warnings){
            for(const auto &warning:qvariant_cast<QList<QQmlError>>(entry[0]))
                QCOMPARE(warning.description(),QString("QML PdfDocument: Cannot open: "));
        }
    }
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
    void spotlightRowsResize(){
        Fixture f(true);
        LocalizedEngine engine;
        engine.rootContext()->setContextProperty("vetust",f.backend.get());
        engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window);
        const auto first=f.backend->homeSpotlight()["id"].toString();
        QSet<QString> previous;
        for(const auto width:{1920,2560,1480,2560}){
            window->resize(width,1080);
            const int expected=width>=2300 ? 3 : width>=1700 ? 2 : 1;
            QTRY_COMPARE(f.backend->homeSpotlights().size(),expected);
            QCOMPARE(f.backend->homeSpotlight()["id"].toString(),first);
            QSet<QString> ids;
            for(int index=0;index<expected;++index){
                ids.insert(f.backend->homeSpotlights()[index].toMap()["id"].toString());
                QQuickItem *card=nullptr;
                QTRY_VERIFY((card=findVisual(window->contentItem(),"spotlightCard-"+QString::number(index)))!=nullptr);
                QVERIFY(card->width()>600);
                QVERIFY(card->mapToScene({card->width(),0}).x()<=width);
            }
            QCOMPARE(ids.size(),expected);
            previous=ids;
        }
        const auto stable=f.backend->homeSpotlights();
        f.backend->refreshHome();
        QCOMPARE(f.backend->homeSpotlights(),stable);
        f.backend->refreshHome(true,3);
        for(const auto &entry:f.backend->homeSpotlights())QVERIFY(!previous.contains(entry.toMap()["id"].toString()));
        QQuickItem *lastCard=nullptr;
        QTRY_VERIFY((lastCard=findVisual(window->contentItem(),"spotlightCard-2"))!=nullptr);
        QTRY_VERIFY(lastCard->width()>600 && lastCard->height()>100);
        QTest::qWait(100);
        const auto target=f.backend->homeSpotlights()[2].toMap()["id"].toString();
        const auto point=lastCard->mapToScene({lastCard->width()-40,lastCard->height()/2});
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point.toPoint());
        QTRY_COMPARE(f.backend->selected()["id"].toString(),target);
    }
    void spotlightClassics(){
        Fixture f(true);
        f.backend.reset();QSqlDatabase::removeDatabase("vetust-native");
        QFile catalog(f.temp.path()+"/data/catalog.json");
        QVERIFY(catalog.open(QIODevice::ReadOnly));
        auto data=QJsonDocument::fromJson(catalog.readAll()).object();catalog.close();
        auto entries=data["games"].toArray();
        const QHash<int,QString> titles={{0,"DOOM"},{1,"The Secret of Monkey Island"},
            {4,"Maniac Mansion (Enhanced Version)"},{6,"Maniac Mansion"},
            {7,"Racter"},{8,"DOOM: Unofficial Levels"}};
        for(auto it=titles.cbegin();it!=titles.cend();++it){
            auto entry=entries[it.key()].toObject();entry["title"]=it.value();entries[it.key()]=entry;
        }
        data["games"]=entries;QVERIFY(catalog.open(QIODevice::WriteOnly|QIODevice::Truncate));
        catalog.write(QJsonDocument(data).toJson());catalog.close();
        f.backend=std::make_unique<Backend>(f.temp.path());
        f.backend->selectGame(Seed);f.backend->toggleFavorite();
        const QSet<QString> expected={Seed,"1","4"};
        for(int visit=0;visit<15;++visit){
            f.backend->refreshHome(true,3);
            QSet<QString> ids;
            for(const auto &value:f.backend->homeSpotlights()){
                const auto entry=value.toMap();ids.insert(entry["id"].toString());
                QCOMPARE(entry["reason"].toString(),QString("Classics to rediscover"));
            }
            QCOMPARE(ids,expected);
            QCOMPARE(f.backend->homeSpotlights().size(),3);
        }
        // Con pocos clásicos, reducir la fila es preferible a rellenarla con desconocidos.
        f.backend.reset();QSqlDatabase::removeDatabase("vetust-native");
        data["games"]=QJsonArray{entries[1],entries[7]};
        QVERIFY(catalog.open(QIODevice::WriteOnly|QIODevice::Truncate));
        catalog.write(QJsonDocument(data).toJson());catalog.close();
        f.backend=std::make_unique<Backend>(f.temp.path());
        f.backend->refreshHome(true,3);
        QCOMPARE(f.backend->homeSpotlights().size(),1);
        QCOMPARE(f.backend->homeSpotlight()["id"].toString(),QString("1"));
    }
    void spotlightRotation(){
        Fixture f(true);
        f.backend->refreshHome(true);
        QSet<QString> seen;
        for(int visit=0;visit<12;++visit){
            const auto spotlight=f.backend->homeSpotlight();
            const auto id=spotlight["id"].toString();
            QVERIFY(!id.isEmpty());QVERIFY(!seen.contains(id));
            QVERIFY(id!="1" && id!="4");
            seen.insert(id);
            f.backend->refreshHome();
            QCOMPARE(f.backend->homeSpotlight(),spotlight);
            f.backend->retranslate();
            QCOMPARE(f.backend->homeSpotlight()["id"].toString(),id);
            f.backend->refreshHome(true);
        }
        const auto latest=f.backend->homeSpotlight()["id"].toString();
        QVERIFY(!seen.contains(latest));
        QFile history(f.temp.path()+"/data/spotlight-history.json");
        QVERIFY(history.open(QIODevice::ReadOnly));
        const auto saved=QJsonDocument::fromJson(history.readAll()).array();
        QCOMPARE(saved.size(),12);QCOMPARE(saved.first().toString(),latest);
        history.close();
        f.backend.reset();QSqlDatabase::removeDatabase("vetust-native");
        f.backend=std::make_unique<Backend>(f.temp.path());
        f.backend->refreshHome(true);
        QVERIFY(!saved.contains(f.backend->homeSpotlight()["id"].toString()));
    }
    void consoleLaunch(){
        Fixture f(false,true);auto &b=*f.backend;b.selectGame(Seed);
        b.openConsole();QVERIFY(b.busy());QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);
        QVERIFY(b.status().startsWith("Console closed"));QVERIFY(f.row("recent").isEmpty());
        QSqlQuery query(QSqlDatabase::database("vetust-native"));QVERIFY(query.exec("SELECT COUNT(*) FROM games WHERE last_played > 0"));QVERIFY(query.next());QCOMPARE(query.value(0).toInt(),0);
        QDir spanish(f.temp.path()+"/config/sessions/"+Seed+"-es");
        const auto configs=spanish.entryList({"vetust-console-*.conf"},QDir::Files);QCOMPARE(configs.size(),1);
        QFile config(spanish.filePath(configs.first()));QVERIFY(config.open(QIODevice::ReadOnly));const QString text=QString::fromUtf8(config.readAll());
        QVERIFY(text.contains("cycles=9000"));QVERIFY(!text.contains("GAME.EXE"));QVERIFY(text.contains("c:\ncd \\\n"));
        QVERIFY(QFileInfo::exists(f.temp.path()+"/logs/"+Seed+"-es-console-native.log"));
        b.setSpanish(false);b.openConsole();QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);
        QDir original(f.temp.path()+"/config/sessions/"+Seed);QCOMPARE(original.entryList({"vetust-console-*.conf"},QDir::Files).size(),1);
        QVERIFY(!b.xTreeAvailable());b.openConsole("xtree");QVERIFY(!b.busy());QVERIFY(b.status().contains("is not installed"));
        const QString tools=f.temp.path()+"/runtime/dos-tools/xtgold";QDir().mkpath(tools);
        for(const QString &name:QStringList{"xtgold.com","xtg.exe","XTREE.BAT"}){QFile file(tools+'/'+name);QVERIFY(file.open(QIODevice::WriteOnly));file.write("prueba");}
        QVERIFY(b.xTreeAvailable());b.openConsole("xtree");QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),5000);QVERIFY(f.row("recent").isEmpty());
        bool launchesXTree=false;
        for(const QString &name:original.entryList({"vetust-console-*.conf"},QDir::Files)){QFile file(original.filePath(name));QVERIFY(file.open(QIODevice::ReadOnly));if(file.readAll().contains("T:\\XTGOLD.COM C: /R"))launchesXTree=true;}
        QVERIFY(launchesXTree);
        QVERIFY(!b.nortonAvailable());b.openConsole("norton");QVERIFY(!b.busy());QVERIFY(b.status().contains("is not installed"));
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
    void scummFolderSettingsUi(){
        Fixture f;auto &b=*f.backend;const QString lib=f.temp.path()+"/source";
        QTemporaryDir external;const QString custom=external.path()+"/Aventuras";
        QDir().mkpath(lib+"/scummvm/Juego");
        ScummConfig config{{"scummvm",{{"vetust_hidden_folders","[\"Juego\"]"}}},
            {"kept",{{"gameid","monkey"},{"path",lib+"/scummvm/Juego"},{"vetust_relative_path","Juego"},{"music_volume","99"},{"savepath",f.temp.path()+"/saves/kept"}}}};
        {QFile file(b.scumm()->configPath());QVERIFY(file.open(QIODevice::WriteOnly));file.write(ScummLibrary::serialize(config));}
        b.scumm()->setLibrary(lib);QVERIFY(QDir().rename(lib+"/scummvm",custom));
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",&b);engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=engine.rootObjects().first();auto dialog=window->findChild<QObject*>("storageDialog");
        auto field=window->findChild<QObject*>("scummFolder");auto save=window->findChild<QObject*>("saveStorageButton");auto reset=window->findChild<QObject*>("resetScummFolder");QVERIFY(dialog&&field&&save&&reset);
        QVERIFY(QMetaObject::invokeMethod(dialog,"begin"));field->setProperty("text",custom+"/missing");QVERIFY(QMetaObject::invokeMethod(save,"clicked"));QVERIFY(!dialog->property("error").toString().isEmpty());QVERIFY(dialog->property("visible").toBool());
        field->setProperty("text",custom);QVERIFY(QMetaObject::invokeMethod(save,"clicked"));QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),10000);QVERIFY(!dialog->property("visible").toBool());
        QCOMPARE(b.storageSettings()["scummvm"].toString(),custom);QCOMPARE(Storage(f.temp.path()).data["scummvm"].toString(),custom);
        QCOMPARE(b.scumm()->games().size(),1);QVERIFY(b.scumm()->games().first().toMap()["hidden"].toBool());
        auto saved=ScummLibrary::readConfig(b.scumm()->configPath());QCOMPARE(saved["kept"]["music_volume"],QString("99"));QCOMPARE(saved["kept"]["savepath"],f.temp.path()+"/saves/kept");
        ScummLibrary reopened(f.temp.path());reopened.setLibrary(lib,Storage(f.temp.path()).data["scummvm"].toString());QCOMPARE(reopened.games().size(),1);
        QVERIFY(QDir().rename(custom,lib+"/scummvm"));QVERIFY(QMetaObject::invokeMethod(dialog,"begin"));QCOMPARE(field->property("text").toString(),custom);
        QVERIFY(QMetaObject::invokeMethod(reset,"clicked"));QVERIFY(field->property("text").toString().isEmpty());QVERIFY(QMetaObject::invokeMethod(save,"clicked"));QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),10000);
        QVERIFY(!Storage(f.temp.path()).data.contains("scummvm"));QCOMPARE(b.scumm()->games().size(),1);QVERIFY(b.scumm()->games().first().toMap()["hidden"].toBool());
    }
    void organizedLibraryKeepsMediaMusicAndSaves(){
        Fixture f;const QString lib=f.temp.path()+"/source";
        auto put=[](const QString &path,const QByteArray &bytes){QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);if(!file.open(QIODevice::WriteOnly))qFatal("No se pudo preparar la prueba");file.write(bytes);};
        put(lib+"/library.json","{\"format\":1,\"layout\":\"organized\"}");
        const QString target=f.temp.path()+"/data/games/organized";
        put(target+"/.ready","");put(target+"/Game/SAVE.DAT","partida intacta");
        QJsonObject game{{"id","organized"},{"folder","Game"}};
        QJsonObject profile{{"config","[midi]\nmididevice=mt32\n[autoexec]\nmount c ./eXoDOS/Game\nc:\nGAME.EXE\nexit\n"}};
        for(const auto &engine:QStringList{"staging","dosbox-x"}){
            const auto plan=f.backend->prepare(game,profile,engine,{},[](int){},true);
            QVERIFY2(plan.error.isEmpty(),qPrintable(plan.error));
            QFile file(plan.config);QVERIFY(file.open(QIODevice::ReadOnly));const auto text=file.readAll();
            QVERIFY(text.contains("/resources/audio/roland-mt32"));QVERIFY(text.contains("/resources/audio/soundfonts/SoundCanvas.sf2"));
            QVERIFY(text.contains("mididevice=mt32"));QVERIFY(text.contains("/games/Game"));QVERIFY(!text.contains("eXo"));
            QVERIFY(!QFileInfo::exists(plan.directory+"/eXoDOS"));
        }
        QFile save(target+"/Game/SAVE.DAT");QVERIFY(save.open(QIODevice::ReadOnly));QCOMPARE(save.readAll(),QByteArray("partida intacta"));
        put(lib+"/games/atari2600/roms/a.bin","ROM");put(lib+"/metadata/atari2600/boxes/a.png","image");
        QJsonObject variant{{"id","rom"},{"files",QJsonArray{"roms/a.bin"}},{"cover","boxes/a.png"}};
        QJsonObject catalog{{"format",1},{"games",QJsonArray{QJsonObject{{"id","game"},{"variants",QJsonArray{variant}}}}}};
        put(lib+"/metadata/atari2600/catalog.json",QJsonDocument(catalog).toJson());
        AtariLibrary atari(f.temp.path());atari.setLibrary(lib);QCOMPARE(atari.games().size(),1);
        QVERIFY(atari.games().first().toMap()["cover"].toString().contains("/metadata/atari2600/"));
        QVERIFY(atari.games().first().toMap()["selected"].toMap()["path"].toString().contains("/games/atari2600/"));
        atari.setHidden("game",true);
        put(lib+"/games/mame/roms/a.zip","ROM");put(lib+"/metadata/mame/flyers/a.png","image");
        variant=QJsonObject{{"id","a"},{"cover","flyers/a.png"},{"playable",true}};
        catalog=QJsonObject{{"format",1},{"engine","0.241"},{"games",QJsonArray{QJsonObject{{"id","a"},{"variants",QJsonArray{variant}}}}}};
        put(lib+"/metadata/mame/catalog.json",QJsonDocument(catalog).toJson());
        ArcadeLibrary arcade(f.temp.path());arcade.setLibrary(lib);QCOMPARE(arcade.games().size(),1);
        QVERIFY(arcade.games().first().toMap()["cover"].toString().contains("/metadata/mame/"));
        const auto args=arcade.launchArguments("a");QCOMPARE(args.value(args.indexOf("--metadata")+1),lib+"/metadata/mame");
        put(lib+"/games/scummvm/A/game.dat","game");put(lib+"/metadata/scummvm/A/manual.pdf","pdf");put(lib+"/metadata/scummvm/A/portada.png","image");
        ScummLibrary scumm(f.temp.path());scumm.setLibrary(lib);QCOMPARE(scumm.games().size(),1);
        QVERIFY(scumm.games().first().toMap()["manual"].toString().contains("/metadata/scummvm/"));
        QVERIFY(scumm.games().first().toMap()["cover"].toString().contains("/metadata/scummvm/"));
        const QString relocated=f.temp.path()+"/otro disco";QVERIFY(QDir().rename(lib,relocated));
        atari.setLibrary(relocated);arcade.setLibrary(relocated);scumm.setLibrary(relocated);
        QVERIFY(atari.games().first().toMap()["hidden"].toBool());QCOMPARE(arcade.games().size(),1);QCOMPARE(scumm.games().size(),1);
        QVERIFY(!QFileInfo::exists(relocated+"/eXo"));

        // Las colecciones y los recursos ya no necesitan compartir una raíz.
        put(relocated+"/resources/audio/roland-mt32/CM32L_CONTROL.ROM","ROM");
        put(relocated+"/resources/audio/soundfonts/SoundCanvas.sf2","soundfont");
        const QString media=f.temp.path()+"/media-disk",resources=f.temp.path()+"/audio-disk";
        QVERIFY(QDir().rename(relocated+"/metadata",media));
        QVERIFY(QDir().rename(relocated+"/resources",resources));
        const QString atariGames=f.temp.path()+"/atari-disk",arcadeGames=f.temp.path()+"/arcade-disk",scummGames=f.temp.path()+"/adventure-disk";
        QVERIFY(QDir().rename(relocated+"/games/atari2600",atariGames));
        QVERIFY(QDir().rename(relocated+"/games/mame",arcadeGames));
        QVERIFY(QDir().rename(relocated+"/games/scummvm",scummGames));
        QVERIFY(QDir(relocated).removeRecursively());
        QVariantMap paths{{"metadata",media},{"resources",resources},{"atari2600",atariGames},{"mame",arcadeGames},{"dosbox",""},{"magazines",""}};
        QCOMPARE(f.backend->saveStorage(QUrl(),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl::fromLocalFile(scummGames),paths),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!f.backend->busy(),10000);
        QVERIFY(Storage(f.temp.path()).library.isEmpty());
        QCOMPARE(f.backend->atari()->games().size(),1);
        QVERIFY(f.backend->atari()->games().first().toMap()["hidden"].toBool());
        QVERIFY(f.backend->atari()->games().first().toMap()["cover"].toString().contains(media));
        QCOMPARE(f.backend->arcade()->games().size(),1);
        QVERIFY(f.backend->arcade()->games().first().toMap()["cover"].toString().contains(media));
        QCOMPARE(f.backend->scumm()->games().size(),1);
        QVERIFY(f.backend->scumm()->games().first().toMap()["manual"].toString().contains(media));
        const auto detachedArgs=f.backend->arcade()->launchArguments("a");
        QCOMPARE(detachedArgs.value(detachedArgs.indexOf("--metadata")+1),media+"/mame");
        for(const auto &engine:QStringList{"staging","dosbox-x"}){
            const auto plan=f.backend->prepare(game,profile,engine,{},[](int){},true);QVERIFY2(plan.error.isEmpty(),qPrintable(plan.error));
            QFile file(plan.config);QVERIFY(file.open(QIODevice::ReadOnly));const auto config=file.readAll();
            QVERIFY(config.contains((resources+"/audio/roland-mt32").toUtf8()));
            QVERIFY(config.contains((resources+"/audio/soundfonts/SoundCanvas.sf2").toUtf8()));
            QVERIFY(!config.contains(relocated.toUtf8()));
        }
        QFile intact(target+"/Game/SAVE.DAT");QVERIFY(intact.open(QIODevice::ReadOnly));QCOMPARE(intact.readAll(),QByteArray("partida intacta"));
        const QString imagePath=f.temp.path()+"/cover.png";QImage cover(80,120,QImage::Format_RGB32);cover.fill(Qt::cyan);QVERIFY(cover.save(imagePath));
        QFile imageFile(imagePath);QVERIFY(imageFile.open(QIODevice::ReadOnly));const auto imageData=imageFile.readAll();
        QDir().mkpath(media+"/dosbox/extras");int zipError=0;
        auto createZip=[&](const QString &path,const QString &entry,const QByteArray &bytes){
            zip_t *zip=zip_open(QFile::encodeName(path).constData(),ZIP_CREATE|ZIP_TRUNCATE,&zipError);QVERIFY(zip);
            auto data=zip_source_buffer(zip,bytes.constData(),bytes.size(),0);QVERIFY(data);
            QVERIFY(zip_file_add(zip,entry.toUtf8().constData(),data,ZIP_FL_ENC_UTF_8)>=0);QCOMPARE(zip_close(zip),0);
        };
        createZip(media+"/dosbox/recursos.zip","cover.png",imageData);
        ArtProvider art(f.temp.path(),{{"detached-cover","cover.png"}});QSize size;
        QVERIFY(!art.requestImage("detached-cover",&size,QSize(200,200)).isNull());
        const QByteArray pdf="manual preserved";
        createZip(media+"/dosbox/extras/manual.zip","manual.pdf",pdf);
        const QJsonObject item{{"name","manual.pdf"},{"archive","metadata/dosbox/extras/manual.zip"},{"entry","manual.pdf"},{"bytes",pdf.size()},{"sha256",QString::fromLatin1(QCryptographicHash::hash(pdf,QCryptographicHash::Sha256).toHex())}};
        put(media+"/dosbox/extras/extras.json",QJsonDocument(QJsonObject{{"format",1},{"games",QJsonObject{{"organized",QJsonArray{item}}}}}).toJson());
        Extras extras;extras.load(QString(),media);QCOMPARE(extras.list("organized").size(),1);
        QFile extra(extras.prepare("organized",0,f.temp.path()+"/extra-cache"));QVERIFY(extra.open(QIODevice::ReadOnly));QCOMPARE(extra.readAll(),pdf);
        const QString dosLibrary=f.temp.path()+"/DOS collection";
        QVERIFY(QDir().mkpath(dosLibrary+"/archives"));
        const QMap<QString,QString> separate{{"dosbox",dosLibrary+"/metadata"},{"scummvm",scummGames+"/metadata"},{"atari2600",atariGames+"/metadata"},{"mame",arcadeGames+"/metadata"}};
        paths.remove("metadata");paths["embeddedMetadata"]=true;paths["dosbox"]=dosLibrary;
        for(auto it=separate.begin();it!=separate.end();++it){
            QVERIFY(QDir().rename(media+'/'+it.key(),it.value()));

        }
        QVERIFY(QDir(media).removeRecursively());
        QCOMPARE(f.backend->saveStorage(QUrl(),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl::fromLocalFile(scummGames),paths),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!f.backend->busy(),10000);
        Storage independent(f.temp.path());QVERIFY(!independent.data.contains("metadata"));QVERIFY(independent.library.isEmpty());
        for(auto it=separate.begin();it!=separate.end();++it)QCOMPARE(independent.metadataDirectory(it.key()),it.value());
        QCOMPARE(independent.sharedPath("metadata/dosbox/recursos.zip","unused"),separate["dosbox"]+"/recursos.zip");
        QVERIFY(QUrl(f.backend->atari()->games().first().toMap()["cover"].toString()).toLocalFile().startsWith(separate["atari2600"]));
        QVERIFY(QUrl(f.backend->arcade()->games().first().toMap()["cover"].toString()).toLocalFile().startsWith(separate["mame"]));
        QVERIFY(QUrl(f.backend->scumm()->games().first().toMap()["manual"].toString()).toLocalFile().startsWith(separate["scummvm"]));
        QCOMPARE(f.backend->scumm()->games().size(),1);
        ArtProvider independentArt(f.temp.path(),{{"independent-cover","cover.png"}});
        QVERIFY(!independentArt.requestImage("independent-cover",&size,QSize(200,200)).isNull());
        Extras independentExtras;independentExtras.load(QString(),QString(),separate["dosbox"]);
        QFile independentExtra(independentExtras.prepare("organized",0,f.temp.path()+"/independent-extra"));
        QVERIFY(independentExtra.open(QIODevice::ReadOnly));QCOMPARE(independentExtra.readAll(),pdf);
        for(const auto &engine:QStringList{"staging","dosbox-x"}){
            const auto plan=f.backend->prepare(game,profile,engine,{},[](int){},true);QVERIFY2(plan.error.isEmpty(),qPrintable(plan.error));
            QFile config(plan.config);QVERIFY(config.open(QIODevice::ReadOnly));QVERIFY(config.readAll().contains((resources+"/audio/roland-mt32").toUtf8()));
        }

        // Algunos manuales ya están dentro del ZIP del juego; no se duplican en metadata.
        createZip(dosLibrary+"/archives/test.zip","manual.pdf",pdf);
        auto gameExtra=item;gameExtra["archive"]="games/dosbox/archives/test.zip";
        put(separate["dosbox"]+"/extras/extras.json",QJsonDocument(QJsonObject{{"format",1},{"games",QJsonObject{{"organized",QJsonArray{item,gameExtra,QJsonObject{{"name","manual.pdf"},{"archive","games/dosbox/metadata/extras/manual.zip"},{"entry","manual.pdf"},{"bytes",pdf.size()},{"sha256",item["sha256"]}}}}}}}).toJson());
        independentExtras.load(QString(),QString(),separate["dosbox"]);
        QFile packedManual(independentExtras.prepare("organized",1,f.temp.path()+"/game-extra"));
        QVERIFY(packedManual.open(QIODevice::ReadOnly));QCOMPARE(packedManual.readAll(),pdf);
        QFile nestedManual(independentExtras.prepare("organized",2,f.temp.path()+"/nested-extra"));
        QVERIFY(nestedManual.open(QIODevice::ReadOnly));QCOMPARE(nestedManual.readAll(),pdf);
        // La carpeta completa cambia de disco sin configurar otra ruta de metadatos.
        const QString movedDos=f.temp.path()+"/moved DOS";
        QVERIFY(QDir().rename(dosLibrary,movedDos));paths["dosbox"]=movedDos;
        QCOMPARE(f.backend->saveStorage(QUrl(),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl::fromLocalFile(scummGames),paths),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!f.backend->busy(),10000);
        Storage moved(f.temp.path());QCOMPARE(moved.metadataDirectory("dosbox"),movedDos+"/metadata");
        ArtProvider movedArt(f.temp.path(),{{"moved-cover","cover.png"}});
        QVERIFY(!movedArt.requestImage("moved-cover",&size,QSize(200,200)).isNull());
        // Audio y herramientas acompañan al juego incluso al trasladar una colección independiente.
        QVERIFY(QDir().rename(resources,movedDos+"/resources"));
        QVERIFY(QDir().mkpath(movedDos+"/resources/dos"));
        zip_t *tools=zip_open(QFile::encodeName(movedDos+"/resources/dos/herramientas.zip").constData(),ZIP_CREATE|ZIP_TRUNCATE,&zipError);QVERIFY(tools);
        const QByteArray toolData="tool";
        for(const auto &name:{"xtgold/XTREE.BAT","norton/NC.EXE"}){
            auto data=zip_source_buffer(tools,toolData.constData(),toolData.size(),0);QVERIFY(data);
            QVERIFY(zip_file_add(tools,name,data,ZIP_FL_ENC_UTF_8)>=0);
        }
        QCOMPARE(zip_close(tools),0);
        paths.remove("resources");paths["embeddedResources"]=true;
        QCOMPARE(f.backend->saveStorage(QUrl(),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl::fromLocalFile(scummGames),paths),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!f.backend->busy(),10000);
        QVERIFY(!Storage(f.temp.path()).data.contains("resources"));
        QVERIFY(f.backend->xTreeAvailable());QVERIFY(f.backend->nortonAvailable());
        for(const auto &engine:QStringList{"staging","dosbox-x"}){
            const auto plan=f.backend->prepare(game,profile,engine,{},[](int){},true);
            QVERIFY2(plan.error.isEmpty(),qPrintable(plan.error));
            QFile config(plan.config);QVERIFY(config.open(QIODevice::ReadOnly));const auto text=config.readAll();
            QVERIFY(text.contains((movedDos+"/resources/audio/roland-mt32").toUtf8()));
            QVERIFY(text.contains((movedDos+"/resources/audio/soundfonts/SoundCanvas.sf2").toUtf8()));
            QVERIFY(!text.contains(resources.toUtf8()));
        }
        const QString complete=f.temp.path()+"/complete DOS";
        QVERIFY(QDir().rename(movedDos,complete));paths["dosbox"]=complete;
        QCOMPARE(f.backend->saveStorage(QUrl(),QUrl::fromLocalFile(f.temp.path()+"/data/games"),QUrl::fromLocalFile(scummGames),paths),QString());
        QTRY_VERIFY_WITH_TIMEOUT(!f.backend->busy(),10000);
        Storage completeStorage(f.temp.path());
        QCOMPARE(completeStorage.sharedPath("resources/dos/herramientas.zip","unused"),complete+"/resources/dos/herramientas.zip");
        QVERIFY(QFileInfo(completeStorage.sharedPath("resources/audio/roland-mt32/CM32L_CONTROL.ROM","unused")).isFile());
        QVERIFY(f.backend->xTreeAvailable());
        // Desconectar el disco no sustituye la ubicación elegida por la de otra biblioteca.
        QVERIFY(QDir().rename(complete,complete+"-offline"));
        QCOMPARE(Storage(f.temp.path()).sharedPath("resources/dos/herramientas.zip","unused"),complete+"/resources/dos/herramientas.zip");

        const QString magazineRoot=f.temp.path()+"/magazines";
        put(magazineRoot+"/issues/a.pdf","PDF");put(magazineRoot+"/metadata/covers/a.png","cover");
        put(magazineRoot+"/metadata/revistas.json",R"({"format":1,"series":[{"id":"A","cover":"metadata/covers/a.png"}],"entries":[{"id":"a","kind":"pdf","path":"issues/a.pdf"}]})");
        Magazines mag;mag.load(QString(),magazineRoot);
        QCOMPARE(mag.pdf("a"),magazineRoot+"/issues/a.pdf");
        QCOMPARE(mag.series().first().toMap()["cover"].toUrl().toLocalFile(),magazineRoot+"/metadata/covers/a.png");
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
        plan=backend.prepare(game,profile,"staging",{},[](int){});QVERIFY(plan.error.contains("is unavailable"));QVERIFY(!QFileInfo::exists(external.path()));
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
        LocalizedEngine engine;
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
        window->setProperty("screens",QVariantList{a});QVERIFY(!section->isVisible());QVERIFY(picker->property("currentText").toString().contains("Disconnected"));
        window->setProperty("screens",QVariantList{a,b});QCOMPARE(picker->property("currentIndex").toInt(),2);
        QVERIFY(begin(false));QVERIFY(!section->isVisible());
    }
    void imageStyleUi(){
        Fixture f;LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());
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
        QVERIFY(picker->isVisible());QCOMPARE(picker->property("displayText").toString(),QString("Sharp"));
        auto choose=[&](int index){picker->setProperty("currentIndex",index);return QMetaObject::invokeMethod(picker,"activated",Q_ARG(int,index));};
        auto save=[&](bool global){return f.backend->saveEmulatorSettings(global,f.backend->selected()["id"].toString(),dialog->property("draft").toMap(),dialog->property("paths").toMap());};
        QVERIFY(choose(1));QCOMPARE(save(true),QString());
        QVERIFY(begin(true));QCOMPARE(picker->property("displayText").toString(),QString("Sharp"));
        QVERIFY(begin(false));QCOMPARE(picker->property("displayText").toString(),QString("Use global setting · Sharp"));
        QVERIFY(choose(1));QCOMPARE(save(false),QString());
        QVERIFY(begin(false));QCOMPARE(picker->property("displayText").toString(),QString("Classic monitor (CRT)"));
        QVERIFY(choose(0));QCOMPARE(save(false),QString());
        QVERIFY(begin(false));QCOMPARE(picker->property("displayText").toString(),QString("Use global setting · Sharp"));
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
        b.openMagazine("BBD");QTRY_VERIFY_WITH_TIMEOUT(!b.busy(),10000);QVERIFY(b.status().startsWith("Magazine closed"));QVERIFY(f.row("recent").isEmpty());
        QDir configs(f.temp.path()+"/config/sessions/revistas");const auto names=configs.entryList({"*.conf"},QDir::Files);QCOMPARE(names.size(),1);
        QFile config(configs.filePath(names.first()));QVERIFY(config.open(QIODevice::ReadOnly));const auto text=config.readAll();QVERIFY(text.contains("cycles=9000"));QVERIFY(text.contains("/data/magazines/BBD-"));QVERIFY(!text.contains("/source/Revistas"));QVERIFY(text.contains("call run.bat"));
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",&b);engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
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
        LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
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
        Fixture f;LocalizedEngine engine;engine.rootContext()->setContextProperty("vetust",f.backend.get());engine.addImageProvider("art",new ArtProvider(f.temp.path(),{}));
        engine.load(QUrl::fromLocalFile(QStringLiteral(VETUST_SOURCE)+"/Main.qml"));QVERIFY(!engine.rootObjects().isEmpty());
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());QVERIFY(window);window->requestActivate();QTest::qWait(300);
        QCOMPARE(window->property("view").toString(),QString("home"));
        auto field=window->findChild<QQuickItem*>("searchField");QVERIFY(field);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,field->mapToScene({field->width()/2,field->height()/2}).toPoint());
        for(char c:QByteArray("Monkey")){QTest::keyClick(window,c);QTest::qWait(200);QVERIFY(field->hasActiveFocus());}
        QCOMPARE(field->property("text").toString(),QString("Monkey"));QCOMPARE(window->property("view").toString(),QString("all"));
        auto grid=window->findChild<QQuickItem*>("unifiedCatalogGrid");QVERIFY(grid);QTRY_COMPARE(grid->property("count").toInt(),1);
        auto card=qvariant_cast<QQuickItem*>(grid->property("currentItem"));QVERIFY(card);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,card->mapToScene({card->width()/2,card->height()/2}).toPoint());
        auto details=window->findChild<QObject*>("gameDetails");QVERIFY(details);QTRY_VERIFY(details->property("opened").toBool());
        for(const QSize size : {QSize(1480,920),QSize(1050,720)}){
            window->resize(size);
            QTRY_COMPARE(details->property("width").toInt(),window->width());
            QTRY_COMPARE(details->property("height").toInt(),window->height());
            QCOMPARE(details->property("x").toInt(),0);QCOMPARE(details->property("y").toInt(),0);
        }
        auto folder=window->findChild<QQuickItem*>("openDosFolder");QVERIFY(folder);QVERIFY(folder->isVisible());QVERIFY(folder->isEnabled());
        const auto folderPoint=folder->mapToScene({folder->width()/2,folder->height()/2});
        QVERIFY(folderPoint.x()>0&&folderPoint.x()<window->width());QVERIFY(folderPoint.y()>0&&folderPoint.y()<window->height());
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
