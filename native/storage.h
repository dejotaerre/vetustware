#pragma once
#include <QString>
#include <QJsonObject>

class Storage {
public:
    explicit Storage(const QString &root);
    QString root,library,games;
    QJsonObject data;
    QString expand(QString text) const;
    QString dosboxPath(const QString &relative) const;
    QString sharedPath(const QString &current,const QString &legacy) const;
    QString metadataRoot() const;
    QString metadataDirectory(const QString &engine) const;
    bool organized() const;
    static QString validate(const QString &library,const QString &games,const QString &dosbox=QString());
    QString save(const QString &library,const QString &games,const QString &scummvm=QString(),const QJsonObject &locations={}) const;
};
