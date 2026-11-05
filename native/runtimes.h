#pragma once
#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <functional>

class Runtimes : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList engines READ engines NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString detail READ detail NOTIFY changed)
    Q_PROPERTY(bool failed READ failed NOTIFY changed)
public:
    explicit Runtimes(QString root,QObject *parent=nullptr);
    std::function<bool()> blocked;
    QVariantList engines() const{return rows;}
    bool busy() const{return working;}
    QString message() const;
    QString detail() const{return errorDetail;}
    bool failed() const{return error;}
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void check();
    Q_INVOKABLE void install(const QString &engine);
    Q_INVOKABLE void rollback(const QString &engine);
    Q_INVOKABLE void importSystem(const QString &engine);
signals:
    void changed();
    void installed();
private:
    QString root,action,messageKey,errorDetail;
    QVariantList rows;
    QProcess process;
    bool working=false,error=false;
    void start(const QString &operation,const QString &engine=QString());
};
