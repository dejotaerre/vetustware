#pragma once
#include <QJsonObject>
#include <QVariantList>
#include <QString>

class Extras {
public:
    void load(const QString &library);
    QVariantList list(const QString &game) const;
    QString prepare(const QString &game, int index, const QString &cache) const;
private:
    QString library;
    QJsonObject games;
};
