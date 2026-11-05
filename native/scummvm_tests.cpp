#include "scummvm.h"
#include <QtTest>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QGuiApplication>

static void writeFile(const QString &path,const QByteArray &bytes){
    QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size())qFatal("No se pudo crear el archivo de prueba");
}
static int fakeScumm(const QStringList &args){
    QString config,path;
    for(const auto &arg:args){if(arg.startsWith("--config="))config=arg.mid(9);if(arg.startsWith("--path="))path=arg.mid(7);}
    if(args.contains("--add")){
        if(QFileInfo::exists(path+"/fail"))return 1;
        ScummConfig found;
        if(!QFileInfo::exists(path+"/unknown")){
            found["monkey"]={{"gameid","monkey"},{"engineid","scumm"},{"path",path},{"description","Monkey Island (Spanish)"},{"language","es"}};
            if(QFileInfo::exists(path+"/ambiguous")){found["other"]=found["monkey"];found["other"]["platform"]="amiga";}
        }
        writeFile(config,ScummLibrary::serialize(found));return 0;
    }
    writeFile(config+".args",QJsonDocument(QJsonArray::fromStringList(args)).toJson());return 0;
}
class ScummTests:public QObject {
    Q_OBJECT
private slots:
    void iniAndIdentity(){
        QTemporaryDir tmp;
        ScummConfig ini{{"scummvm",{{"fullscreen","true"}}},{"test",{{"description","Español = edición"},{"path",tmp.path()+"/Con espacios"}}}};
        writeFile(tmp.path()+"/file.ini",ScummLibrary::serialize(ini));QCOMPARE(ScummLibrary::readConfig(tmp.path()+"/file.ini"),ini);
        ScummSection a{{"gameid","monkey"},{"vetust_relative_path","Monkey/CD"},{"path","/old/Monkey/CD"}};auto b=a;b["path"]="D:/New/Monkey/CD";QCOMPARE(ScummLibrary::identity(a),ScummLibrary::identity(b));
        b["language"]="es";QVERIFY(ScummLibrary::identity(a)!=ScummLibrary::identity(b));
    }
    void containment(){
        QTemporaryDir tmp;QDir().mkpath(tmp.path()+"/library/scummvm/A");QDir().mkpath(tmp.path()+"/elsewhere");
        const auto base=tmp.path()+"/library/scummvm";
        QCOMPARE(ScummLibrary::relativePath(base,base+"/A"),QString("A"));
        QVERIFY(ScummLibrary::relativePath(base,base).isEmpty());QVERIFY(ScummLibrary::relativePath(base,tmp.path()+"/elsewhere").isEmpty());
#ifndef Q_OS_WIN
        QVERIFY(QFile::link(tmp.path()+"/elsewhere",base+"/link"));QVERIFY(ScummLibrary::relativePath(base,base+"/link").isEmpty());
#endif
    }
    void localCoverAndManual(){
        QTemporaryDir tmp;const QString base=tmp.path()+"/library/scummvm",folder=base+"/Juego";
        writeFile(folder+"/Portada.JPG","imagen de prueba");writeFile(folder+"/Manual.PDF","documento de prueba");
        ScummLibrary scumm(tmp.path()+"/app");scumm.setLibrary(tmp.path()+"/library");
        const auto row=scumm.games().first().toMap();
        QCOMPARE(QUrl(row["cover"].toString()).toLocalFile(),folder+"/Portada.JPG");
        QCOMPARE(QUrl(row["manual"].toString()).toLocalFile(),folder+"/Manual.PDF");
        QSignalSpy opened(&scumm,&ScummLibrary::manualReady);scumm.openManual("Juego");QCOMPARE(opened.size(),1);
        QCOMPARE(opened.first().first().toUrl().toLocalFile(),folder+"/Manual.PDF");
        scumm.openManual("../Juego");QCOMPARE(opened.size(),1);
        scumm.blocked=[] {return true;};scumm.openManual("Juego");QCOMPARE(opened.size(),1);
#ifndef Q_OS_WIN
        QFile::remove(folder+"/Manual.PDF");writeFile(tmp.path()+"/outside.pdf","fuera");QVERIFY(QFile::link(tmp.path()+"/outside.pdf",folder+"/Manual.PDF"));
        QVERIFY(scumm.games().first().toMap()["manual"].toString().isEmpty());
#endif
    }
    void hiddenFoldersPersist(){
        QTemporaryDir tmp;const QString root=tmp.path()+"/app",lib=tmp.path()+"/library";
        QDir().mkpath(lib+"/scummvm/Sin detectar español");
        ScummLibrary scumm(root);scumm.setLibrary(lib);
        QVERIFY(!scumm.games().first().toMap()["hidden"].toBool());
        scumm.setHidden("../fuera",true);QVERIFY(!QFileInfo::exists(scumm.configPath()));
        scumm.setHidden("Sin detectar español",true);QVERIFY(scumm.games().first().toMap()["hidden"].toBool());
        ScummLibrary reopened(root);reopened.setLibrary(lib);QVERIFY(reopened.games().first().toMap()["hidden"].toBool());
        reopened.blocked=[] {return true;};reopened.setHidden("Sin detectar español",false);QVERIFY(reopened.games().first().toMap()["hidden"].toBool());
        reopened.blocked={};reopened.setHidden("Sin detectar español",false);QVERIFY(!reopened.games().first().toMap()["hidden"].toBool());
        scumm.enter();scumm.setLibrary(lib);QVERIFY(!scumm.games().first().toMap()["hidden"].toBool());
    }
    void seriesOrdering(){
        QTemporaryDir tmp;const QString root=tmp.path()+"/app",lib=tmp.path()+"/library";
        const QList<QPair<QString,QString>> titles={
            {"comi","La maldición de Monkey Island"},{"monkey2","Monkey Island 2: LeChuck's Revenge"},
            {"monkey","The Secret of Monkey Island"},{"atlantis","Indiana Jones and the Fate of Atlantis"},
            {"indy3","Indiana Jones and the Last Crusade"},{"lsl7","Leisure Suit Larry 7: Love for Sail!"},
            {"lsl6hires","Leisure Suit Larry 6: Shape Up or Slip Out!"},{"lsl5","Leisure Suit Larry 5"},
            {"lsl1sci","Leisure Suit Larry in the Land of the Lounge Lizards"},
            {"tentacle","Day of the Tentacle"},{"maniac","Maniac Mansion"},{"loom","Loom"}
        };
        ScummConfig config;
        for(const auto &[id,title]:titles){
            const QString path=lib+"/scummvm/"+id;QDir().mkpath(path);
            config[id]={{"gameid",id},{"description",title},{"path",path}};
        }
        // Las carpetas todavía sin detectar conservan un orden natural por nombre.
        QDir().mkpath(lib+"/scummvm/Unknown 10");QDir().mkpath(lib+"/scummvm/Unknown 2");
        config["scummvm"]["vetust_hidden_folders"]="[\"monkey2\"]";
        const auto original=ScummLibrary::serialize(config);writeFile(root+"/config/scummvm.ini",original);
        ScummLibrary scumm(root);scumm.setLibrary(lib);QStringList folders;
        for(const auto &v:scumm.games()){
            const auto row=v.toMap();folders<<row["folder"].toString();
            if(config.contains(row["folder"].toString())){
                QCOMPARE(row["title"].toString(),config[row["folder"].toString()]["description"]);
                QCOMPARE(row["target"].toString(),row["folder"].toString());
            }
            QCOMPARE(row["hidden"].toBool(),row["folder"].toString()=="monkey2");
        }
        QCOMPARE(folders,QStringList({"indy3","atlantis","lsl1sci","lsl5","lsl6hires","lsl7","loom","maniac","tentacle","monkey","monkey2","comi","Unknown 2","Unknown 10"}));
        QCOMPARE(ScummLibrary::serialize(ScummLibrary::readConfig(scumm.configPath())),original);
    }
    void scanPreservesAndLaunches(){
        QTemporaryDir tmp;const QString root=tmp.path()+"/app",lib=tmp.path()+"/collection",base=lib+"/scummvm";
        for(const auto &folder:{"A español","B copia","C ambigua","D desconocido"})QDir().mkpath(base+'/'+folder);
        writeFile(base+"/C ambigua/ambiguous",{});writeFile(base+"/D desconocido/unknown",{});
        QDir().mkpath(root+"/runtime/scummvm");
#ifdef Q_OS_WIN
        const QString executable=root+"/runtime/scummvm/scummvm.exe";
#else
        const QString executable=root+"/runtime/scummvm/scummvm";
#endif
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(),executable));
        ScummLibrary scumm(root);scumm.setLibrary(lib);QVERIFY(scumm.available());scumm.scan();QVERIFY(scumm.busy());
        QTRY_VERIFY_WITH_TIMEOUT(!scumm.busy(),20000);QCOMPARE(scumm.games().size(),4);
        auto config=ScummLibrary::readConfig(scumm.configPath());QCOMPARE(config.size(),5);
        int ready=0,ambiguous=0,unknown=0;QString target;
        for(const auto &v:scumm.games()){
            auto row=v.toMap();auto variants=row["variants"].toList();
            if(variants.isEmpty())++unknown;
            else if(variants.size()>1){++ambiguous;QVERIFY(row["target"].toString().isEmpty());target=variants.first().toMap()["target"].toString();}
            else ++ready;
        }
        QCOMPARE(ready,2);QCOMPARE(ambiguous,1);QCOMPARE(unknown,1);
        scumm.setHidden("A español",true);scumm.setHidden("D desconocido",true);
        auto hiddenCount=[&]{int count=0;for(const auto &v:scumm.games())if(v.toMap()["hidden"].toBool())++count;return count;};
        QCOMPARE(hiddenCount(),2);
        scumm.choose(target);config=ScummLibrary::readConfig(scumm.configPath());QCOMPARE(config[target]["vetust_selected"],QString("true"));
        config[target]["music_volume"]="111";writeFile(scumm.configPath(),ScummLibrary::serialize(config));scumm.enter();scumm.scan();QTRY_VERIFY_WITH_TIMEOUT(!scumm.busy(),20000);
        config=ScummLibrary::readConfig(scumm.configPath());QCOMPARE(config.size(),5);QCOMPARE(config[target]["music_volume"],QString("111"));QCOMPARE(hiddenCount(),2);
        scumm.play(target);QTRY_VERIFY_WITH_TIMEOUT(!scumm.busy(),5000);QVERIFY(QFileInfo::exists(scumm.configPath()+".args"));QVERIFY(QFileInfo(config[target]["savepath"]).isDir());
        QFile args(scumm.configPath()+".args");QVERIFY(args.open(QIODevice::ReadOnly));QVERIFY(QJsonDocument::fromJson(args.readAll()).array().contains(target));args.close();QFile::remove(args.fileName());
        scumm.blocked=[] {return true;};scumm.manage();QVERIFY(!scumm.busy());QVERIFY(!QFileInfo::exists(args.fileName()));scumm.blocked={};
        // Cambiar la ruta de la biblioteca conserva identidades, opciones y partidas.
        const auto moved=tmp.path()+"/moved";QVERIFY(QDir().rename(lib,moved));scumm.setLibrary(moved);QCOMPARE(scumm.games().size(),4);QCOMPARE(hiddenCount(),2);scumm.play(target);QTRY_VERIFY_WITH_TIMEOUT(!scumm.busy(),5000);
        config=ScummLibrary::readConfig(scumm.configPath());QVERIFY(config[target]["path"].startsWith(moved));QCOMPARE(config[target]["music_volume"],QString("111"));
        const auto before=ScummLibrary::serialize(config);writeFile(moved+"/scummvm/A español/fail",{});scumm.scan();QTRY_VERIFY_WITH_TIMEOUT(!scumm.busy(),5000);QCOMPARE(ScummLibrary::serialize(ScummLibrary::readConfig(scumm.configPath())),before);
        scumm.play("nonexistent");QVERIFY(!scumm.busy());
        const QString custom=tmp.path()+"/Aventuras aparte";
        QVERIFY(QDir().rename(moved+"/scummvm",custom));QFile::remove(custom+"/A español/fail");
        scumm.setLibrary(moved,custom);QCOMPARE(scumm.games().size(),4);QCOMPARE(hiddenCount(),2);
        scumm.play(target);QTRY_VERIFY_WITH_TIMEOUT(!scumm.busy(),5000);
        config=ScummLibrary::readConfig(scumm.configPath());QVERIFY(config[target]["path"].startsWith(custom+'/'));QCOMPARE(config[target]["music_volume"],QString("111"));
        QVERIFY(QFileInfo(config[target]["savepath"]).isDir());
        ScummLibrary reopened(root);reopened.setLibrary(moved,custom);QCOMPARE(reopened.games().size(),4);
        QDir().mkpath(moved+"/scummvm/Fallback");scumm.setLibrary(moved,custom+"-desconectado");QVERIFY(scumm.games().isEmpty());
        scumm.setLibrary(moved);QCOMPARE(scumm.games().size(),1);

    }
};
int main(int argc,char **argv){
    QGuiApplication app(argc,argv);const auto args=app.arguments();
    if(args.size()>1&&args[1].startsWith("--config="))return fakeScumm(args.mid(1));
    ScummTests test;return QTest::qExec(&test,argc,argv);
}
#include "scummvm_tests.moc"
