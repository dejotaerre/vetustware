#include "atari.h"
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

static void write(const QString &path,const QByteArray &bytes){QDir().mkpath(QFileInfo(path).absolutePath());QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write(bytes),bytes.size());}
class AtariTests : public QObject {
    Q_OBJECT
private slots:
    void portableCatalogAndPreferences(){
        QTemporaryDir temp;const auto root=temp.path()+"/app",library=temp.path()+"/library",base=library+"/atari2600";
        write(base+"/roms/Game.bin","ROM");write(base+"/snapshots/Game.png","image");write(base+"/boxes/Front.png","cover");
        QJsonArray variants{QJsonObject{{"id","md5-a"},{"title","Game"},{"files",QJsonArray{"roms/Game.bin"}},{"image","snapshots/Game.png"},{"cover","boxes/Front.png"}}};
        QJsonObject catalog{{"format",1},{"romFiles",1},{"uniqueRoms",1},{"games",QJsonArray{QJsonObject{{"id","game"},{"title","Game"},{"variants",variants}}}}};
        write(base+"/catalog.json",QJsonDocument(catalog).toJson());AtariLibrary atari(root);atari.setLibrary(library);QCOMPARE(atari.games().size(),1);
        atari.setFavorite("game",true);QTest::qWait(2);atari.setHidden("game",true);QTest::qWait(2);atari.choose("game","md5-a");
        auto g=atari.games().first().toMap();QVERIFY(g["selected"].toMap()["cover"].toString().endsWith("/boxes/Front.png"));QVERIFY(g["selected"].toMap()["image"].toString().endsWith("/snapshots/Game.png"));QVERIFY(g["favorite"].toBool());QVERIFY(g["hidden"].toBool());
        QVERIFY(atari.configure({},false,true).isEmpty());AtariLibrary restored(root);restored.setLibrary(library);
        QVERIFY(restored.games().first().toMap()["favorite"].toBool());QVERIFY(!restored.settings()["fullscreen"].toBool());
        const auto args=restored.launchArguments(base+"/roms/Game.bin",{{"index",2}});
        QCOMPARE(args.value(args.indexOf("-basedir")+1),root+"/data/stella");QCOMPARE(args.value(args.indexOf("-display")+1),"2");
        QCOMPARE(args.value(args.indexOf("-tv.filter")+1),"3");QCOMPARE(args.last(),base+"/roms/Game.bin");
        QVERIFY(QDir().rename(library,temp.path()+"/moved"));restored.setLibrary(temp.path()+"/moved");QCOMPARE(restored.games().size(),1);
        QVERIFY(restored.games().first().toMap()["hidden"].toBool());QVERIFY(AtariLibrary::safeFile(base,"/etc/passwd").isEmpty());
        write(temp.path()+"/outside.bin","outside");QVERIFY(QFile::link(temp.path()+"/outside.bin",temp.path()+"/moved/atari2600/escape.bin"));
        QVERIFY(AtariLibrary::safeFile(temp.path()+"/moved/atari2600","escape.bin").isEmpty());
        QVERIFY(!restored.configure(QUrl("https://example.org/roms"),true,false).isEmpty());
    }
    void processLifecycleAndBlocking(){
        QTemporaryDir temp;const auto root=temp.path()+"/app",base=temp.path()+"/library/atari2600";
        QDir().mkpath(base);const auto executable=root+"/runtime/stella/stella";
        write(executable,"#!/bin/sh\nprintf '%s' \"$SDL_VIDEODRIVER\" > driver.txt\nsleep 0.15\nexit 0\n");QFile::setPermissions(executable,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
        AtariLibrary atari(root);atari.setLibrary(temp.path()+"/library");atari.blocked=[] {return true;};atari.manage();QVERIFY(!atari.busy());
        atari.blocked=[] {return false;};atari.monitor=[] {return QVariantMap{{"index",1},{"driver","wayland"}};};atari.manage();QVERIFY(atari.busy());QTRY_VERIFY(!atari.busy());QVERIFY(QFileInfo(root+"/logs/stella.log").isFile());
        QVERIFY(atari.status().contains("kept"));QFile driver(root+"/data/stella/driver.txt");QVERIFY(driver.open(QIODevice::ReadOnly));QCOMPARE(driver.readAll(),QByteArray("wayland"));
        write(executable,"#!/nonexistent/interpreter\n");atari.manage();QTRY_VERIFY(!atari.busy());QVERIFY(atari.status().startsWith("Could not start Stella"));
    }
};
QTEST_MAIN(AtariTests)
#include "atari_tests.moc"
