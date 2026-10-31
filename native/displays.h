#pragma once
#include <QVariantList>
#include <QJsonObject>
#include <QProcessEnvironment>

namespace Displays {
QVariantList available();
QVariantMap resolve(const QString &name,const QVariantList &screens);
QJsonObject options(const QString &engine,const QVariantMap &screen);
void environment(QProcessEnvironment &env,const QVariantMap &screen);
}
