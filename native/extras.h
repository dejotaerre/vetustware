#pragma once
#include <QJsonObject>
#include <QVariantList>
#include <QString>

class Extras {
public:
    void load(const QString &library,const QString &metadata=QString(),const QString &dosboxMetadata=QString(),const QString &dosboxGames=QString());
    QVariantList list(const QString &game) const;
    QString prepare(const QString &game, int index, const QString &cache) const;
private:
    QString library,metadataRoot,dosboxRoot,gamesRoot;
    QJsonObject games;
};
