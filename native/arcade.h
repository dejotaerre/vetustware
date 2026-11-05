#pragma once
#include <QObject>
#include <QJsonObject>
#include <QProcess>
#include <QVariantList>
#include <QUrl>
#include <QHash>
#include <functional>

class ArcadeLibrary : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList games READ games NOTIFY changed)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit ArcadeLibrary(QString root,QObject *parent=nullptr);
    void setLibrary(const QString &library,const QString &metadata=QString());
    std::function<bool()> blocked;
    std::function<QVariantMap()> monitor;
    QVariantList games() const;
    QVariantMap settings() const;
    bool busy() const { return running || scanning; }
    bool available() const { return !executable().isEmpty(); }
    QString status() const { return message; }
    Q_INVOKABLE void enter();
    Q_INVOKABLE void scan();
    Q_INVOKABLE void play(const QString &id,const QString &variant);
    Q_INVOKABLE void manage();
    Q_INVOKABLE void choose(const QString &id,const QString &variant);
    Q_INVOKABLE void setHidden(const QString &id,bool value);
    Q_INVOKABLE void setFavorite(const QString &id,bool value);
    Q_INVOKABLE QString configure(const QUrl &folder,bool fullscreen,bool artwork);
    Q_INVOKABLE void openFolder(bool personal=false);
    static QString safeFile(const QString &base,const QString &relative);
    QStringList launchArguments(const QString &game,const QVariantMap &screen={}) const;
signals:
    void changed();
private:
    QString root,library,base,metadataBase,metadataOverride,message;
    QJsonObject prefs,catalog;
    mutable QVariantList viewCache;
    mutable QHash<QString,QString> imageCache;
    mutable bool viewDirty=true;
    QProcess process,scanner;
    bool running=false,scanning=false;
    QString executable() const;
    QString cachePath() const;
    void reload();
    bool save(const QJsonObject &next);
    bool ready();
    void start(const QString &rom);
    void notify(const QString &text);
};
