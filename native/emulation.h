#pragma once
#include <QJsonObject>
#include <QVariantList>
#include <QStringList>

class EmulationSettings {
public:
    explicit EmulationSettings(QString root);
    QJsonObject data;
    QString engine(const QString &id) const;
    QString executable(const QString &engine) const;
    static QString name(const QString &engine);
    static QStringList arguments(const QString &engine,const QString &config);
    static QVariantList fields(const QString &engine);
    QJsonObject overrides(const QString &id,const QString &engine) const;
    QString save(bool global,const QString &id,const QJsonObject &scope,const QJsonObject &paths);
    static QString consoleConfig(const QString &config,const QString &folder,const QString &toolsFolder=QString(),const QString &fileManager=QString(),const QString &nortonFolder=QString());
    static QString configured(QString config,const QJsonObject &values,const QString &engine);
    static QString apply(QString config,const QJsonObject &values);
    static QJsonObject values(const QString &config);
private:
    QString root,settingsPath;
};
