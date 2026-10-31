#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <functional>
#include <zip.h>

class Archive {
public:
    explicit Archive(const QString &path);
    ~Archive();
    Archive(const Archive &) = delete;
    QByteArray read(const QString &name);
    QStringList names() const;
    void extractFile(const QString &name,const QString &destination,qint64 size,const QByteArray &sha256);
    QString extract(const QString &destination, const QString &expectedRoot, const std::function<void(int)> &progress, bool selectedRootOnly=false);
private:
    zip_t *handle = nullptr;
};
QString resolveCase(const QString &path);
