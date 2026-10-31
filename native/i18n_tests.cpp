#include "i18n.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QQmlEngine>
#include <QQmlComponent>

static void writeJson(const QString &path,const QJsonObject &doc){
    QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(doc).toJson());
}
class LanguageTests : public QObject {
    Q_OBJECT
private slots:
    void defaultAndPersistence(){
        QTemporaryDir root;
        const QString directory=QStringLiteral(VETUST_TRANSLATIONS);
        {
            QLocale::setDefault(QLocale("es_UY"));
            Languages language(root.path(),directory);
            QCOMPARE(language.current(),QString("en"));QCOMPARE(uiText("Save"),QString("Save"));
            QCOMPARE(QLocale().language(),QLocale::English);
            QDir().mkpath(root.path()+"/config");
            writeJson(root.path()+"/config/interface.json",{{"unrelated",42}});
            QCOMPARE(language.setLanguage("es"),QString());
            QCOMPARE(uiText("Save"),QString("Guardar"));
            QCOMPARE(uiText("Issues · %1 · PDF").arg(3),QString("Números · 3 · PDF"));
            QCOMPARE(language.setLanguage("../../bad"),uiText("This interface language is not available."));
            QCOMPARE(language.current(),QString("es"));
            QFile f(root.path()+"/config/interface.json");QVERIFY(f.open(QIODevice::ReadOnly));
            const auto saved=QJsonDocument::fromJson(f.readAll());QCOMPARE(saved["unrelated"].toInt(),42);
        }
        {
            Languages restored(root.path(),directory);QCOMPARE(restored.current(),QString("es"));
            QCOMPARE(uiText("Sharp"),QString("Nítido"));
            QCOMPARE(restored.setLanguage("en"),QString());QCOMPARE(uiText("Sharp"),QString("Sharp"));
        }
    }
    void communityAndFallback(){
        QTemporaryDir root,directory;
        writeJson(directory.path()+"/fr.json",{{"language","fr"},{"name","Français"},{"messages",QJsonObject{{"Save","Enregistrer"},{"Issues · %1 · PDF","Numéros"},{"Cancel",""}}}});
        writeJson(directory.path()+"/broken.json",{{"language","es"}});
        Languages language(root.path(),directory.path());QCOMPARE(language.available().size(),2);
        QCOMPARE(language.setLanguage("fr"),QString());
        QCOMPARE(uiText("Save"),QString("Enregistrer"));
        QCOMPARE(uiText("Close"),QString("Close"));
        QCOMPARE(uiText("Cancel"),QString("Cancel"));
        QCOMPARE(uiText("Issues · %1 · PDF").arg(2),QString("Issues · 2 · PDF"));
        writeJson(directory.path()+"/fr.json",{{"messages",QJsonObject{}}});
        QCOMPARE(language.setLanguage("en"),QString());
        QVERIFY(!language.setLanguage("fr").isEmpty());QCOMPARE(language.current(),QString("en"));
    }
    void missingLanguageAndInvalidSettings(){
        QTemporaryDir root;
        QDir().mkpath(root.path()+"/config");
        writeJson(root.path()+"/config/interface.json",{{"language","missing"}});
        Languages language(root.path(),QStringLiteral(VETUST_TRANSLATIONS));QCOMPARE(language.current(),QString("en"));
        QFile file(root.path()+"/config/interface.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write("invalid");file.close();
        QVERIFY(!language.setLanguage("es").isEmpty());QCOMPARE(language.current(),QString("en"));
        QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("invalid"));
    }
    void liveQmlBindings(){
        QTemporaryDir root;Languages language(root.path(),QStringLiteral(VETUST_TRANSLATIONS));
        QQmlEngine engine;connect(&language,&Languages::changed,&engine,&QQmlEngine::retranslate);
        QQmlComponent component(&engine);
        component.setData("import QtQml; QtObject { property string label: qsTranslate(\"Vetust\", \"Save\"); property string typed: \"Monkey Island\" }",QUrl());
        std::unique_ptr<QObject> item(component.create());QVERIFY2(item,qPrintable(component.errorString()));
        QCOMPARE(item->property("label").toString(),QString("Save"));
        QCOMPARE(language.setLanguage("es"),QString());QCOMPARE(item->property("label").toString(),QString("Guardar"));
        QCOMPARE(item->property("typed").toString(),QString("Monkey Island"));
        QCOMPARE(language.setLanguage("en"),QString());QCOMPARE(item->property("label").toString(),QString("Save"));
    }
};
QTEST_GUILESS_MAIN(LanguageTests)
#include "i18n_tests.moc"
