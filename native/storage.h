#pragma once
#include <QString>
#include <QJsonObject>

class Storage {
public:
    explicit Storage(const QString &root);
    QString root,library,games;
    QJsonObject data;
    QString expand(QString text) const;
    static QString validate(const QString &library,const QString &games);
    QString save(const QString &library,const QString &games) const;
};
