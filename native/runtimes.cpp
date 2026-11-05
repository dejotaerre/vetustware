#include "runtimes.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

Runtimes::Runtimes(QString r,QObject *parent):QObject(parent),root(r){
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){
        working=false;error=true;messageKey="Could not manage emulator runtimes.";errorDetail=process.errorString();emit changed();
    });
    connect(&process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus state){
        working=false;
        const auto data=QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        error=code!=0 || state!=QProcess::NormalExit || !data["ok"].toBool();
        if(error){messageKey="Could not manage emulator runtimes.";errorDetail=data["error"].toString();if(errorDetail.isEmpty())errorDetail=QString::fromUtf8(process.readAllStandardError()).right(2000);}
        else{
            rows=data["engines"].toVariant().toList();errorDetail.clear();
            messageKey=action=="check"?"Update check completed.":action=="status"?QString():"Runtime ready. Your settings and saves have been kept.";
            if(action!="status" && action!="check")emit installed();
        }
        emit changed();
    });
}
QString Runtimes::message() const{return messageKey;}
void Runtimes::start(const QString &operation,const QString &engine){
    if(working)return;
    if(blocked && blocked()){error=true;messageKey="Close the game before managing runtimes.";errorDetail.clear();emit changed();return;}
    if(!engine.isEmpty() && engine!="scummvm" && engine!="stella")return;
    action=operation;error=false;errorDetail.clear();working=true;
    messageKey=operation=="check"?"Checking for compatible updates…":operation=="status"?"Reading installed runtimes…":"Preparing and verifying runtime…";
    emit changed();
    QStringList args={root+"/helpers/runtime_manager.py","--root",root,operation};
    if(!engine.isEmpty())args<<"--engine"<<engine;
    process.setWorkingDirectory(root);
    process.start(QStandardPaths::findExecutable("python3"),args);
}
void Runtimes::refresh(){start("status");}
void Runtimes::check(){start("check");}
void Runtimes::install(const QString &engine){start("install",engine);}
void Runtimes::rollback(const QString &engine){start("rollback",engine);}
void Runtimes::importSystem(const QString &engine){start("import-system",engine);}
