#include "arcade.h"
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>
static void write(const QString &path,const QByteArray &bytes){QDir().mkpath(QFileInfo(path).absolutePath());QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(bytes),bytes.size());}
class ArcadeTests : public QObject {
    Q_OBJECT
private slots:
    void portableCatalogPreferencesAndMonitor(){
        QTemporaryDir temp;const auto root=temp.path()+"/app",library=temp.path()+"/library",base=library+"/mame";
        QDir().mkpath(base+"/roms");write(base+"/flyers/game.png","image");
        QJsonArray variants{QJsonObject{{"id","game"},{"title","Game"},{"playable",true},{"cover","flyers/game.png"},{"image","../../outside.png"}}};
        QJsonObject catalog{{"format",1},{"engine","0.241"},{"games",QJsonArray{QJsonObject{{"id","game"},{"title","Game"},{"variants",variants}}}}};
        write(base+"/catalog.json",QJsonDocument(catalog).toJson());ArcadeLibrary arcade(root);arcade.setLibrary(library);QCOMPARE(arcade.games().size(),1);
        arcade.setFavorite("game",true);arcade.setHidden("game",true);arcade.choose("game","game");
        QVERIFY(arcade.configure({},false,false).isEmpty());ArcadeLibrary restored(root);restored.setLibrary(library);
        auto g=restored.games().first().toMap();QVERIFY(g["favorite"].toBool());QVERIFY(g["hidden"].toBool());
        QVERIFY(g["selected"].toMap()["image"].toString().isEmpty());QVERIFY(g["cover"].toString().endsWith("/flyers/game.png"));
        auto args=restored.launchArguments("game",{{"index",1},{"id","Display name"}});
        QCOMPARE(args.value(args.indexOf("--screen")+1),"screen1");QVERIFY(args.contains("--window"));QVERIFY(args.contains("--crop"));
        QCOMPARE(args.value(args.indexOf("--personal")+1),root+"/data/mame/0.241");
        QVERIFY(QDir().rename(library,temp.path()+"/moved"));restored.setLibrary(temp.path()+"/moved");QCOMPARE(restored.games().size(),1);
        QVERIFY(!restored.configure(QUrl("https://example.org/roms"),true,true).isEmpty());
        QVERIFY(!restored.configure(QUrl::fromLocalFile(temp.path()),true,true).isEmpty());
    }
    void rejectsWrongCatalogVersionAndBlocksLaunching(){
        QTemporaryDir t;const auto base=t.path()+"/library/mame";write(base+"/catalog.json","{\"format\":1,\"engine\":\"0.242\",\"games\":[]}");
        ArcadeLibrary lib(t.path()+"/app");lib.setLibrary(t.path()+"/library");QVERIFY(lib.games().isEmpty());QVERIFY(!lib.available());
        lib.blocked=[] {return true;};lib.manage();QVERIFY(!lib.busy());QVERIFY(lib.status().contains("Wait"));
    }
};
QTEST_MAIN(ArcadeTests)
#include "arcade_tests.moc"
