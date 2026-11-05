#pragma once
#include "i18n.h"
#include <QJsonObject>
#include <QVariantList>
#include <QString>

class Magazines {
public:
    void load(const QString &library,const QString &folder=QString());
    QVariantList series() const;
    QVariantList issues(const QString &series) const;
    QJsonObject entry(const QString &id) const;
    QString pdf(const QString &id) const;
    QString prepareDos(const QString &id,const QString &workingRoot) const;
    QString error() const{return loadError.isEmpty()?QString():uiText(loadError.toUtf8().constData());}
private:
    QString base,loadError;
    QJsonObject catalog;
    QString resolve(const QString &relative) const;
};
