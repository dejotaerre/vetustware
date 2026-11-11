#pragma once
#include <QObject>
#include <QMap>
#include <QProcess>
#include <QTemporaryDir>
#include <QVariantList>
#include <QUrl>
#include <functional>
#include <memory>

using ScummSection = QMap<QString,QString>;
using ScummConfig = QMap<QString,ScummSection>;

class ScummLibrary : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList games READ games NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit ScummLibrary(QString root,QObject *parent=nullptr);
    void setLibrary(const QString &library,const QString &folder=QString(),const QString &metadata=QString());
    std::function<bool()> blocked;
    QVariantList games() const;
    Q_INVOKABLE QVariantMap gameMetadata(const QString &folder,const QString &gameId) const;
    bool busy() const { return running || scanning; }
    bool available() const;
    QString status() const { return message; }
    Q_INVOKABLE void enter();
    Q_INVOKABLE void scan();
    Q_INVOKABLE void play(const QString &target);
    Q_INVOKABLE void manage();
    Q_INVOKABLE void choose(const QString &target);
    Q_INVOKABLE void setHidden(const QString &folder,bool hidden);
    Q_INVOKABLE void openManual(const QString &folder);
    Q_INVOKABLE void openFolder(const QString &folder=QString());
    static ScummConfig readConfig(const QString &path);
    static QByteArray serialize(const ScummConfig &config);
    static QString identity(const ScummSection &section);
    static QString relativePath(const QString &base,const QString &path);
    QString configPath() const { return root+"/config/scummvm.ini"; }
signals:
    void changed();
    void manualReady(QUrl url,QString title);
private:
    QString root,base,metadataBase,message;
    ScummConfig config,detected;
    QProcess process,detector;
    std::unique_ptr<QTemporaryDir> temporary;
    QStringList pending;
    bool running=false,scanning=false;
    int total=0;
    QString executable() const;
    QString localMedia(const QString &folder,const QStringList &names) const;
    void reload();
    void nextFolder();
    void finishScan();
    void fail(const QString &error);
    bool save(const ScummConfig &next);
    bool ready();
    void start(const QString &target);
};
