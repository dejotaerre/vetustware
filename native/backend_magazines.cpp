#include "i18n.h"
#include "backend.h"
#include "displays.h"
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QDir>
#include <QSaveFile>
#include <QCryptographicHash>

void Backend::openMagazine(const QString &id){
    if(active)return;
    try{
        const auto entry=magazines.entry(id);const QString title=entry["title"].toString();
        if(entry["kind"]=="pdf"){
            emit extraReady(QUrl::fromLocalFile(magazines.pdf(id)),uiText("Magazine · ")+title,true);return;
        }
        const QString engine=emulation.engine(""),executable=emulation.executable(engine);
        if(executable.isEmpty()){notify(uiText("Check the DOSBox engine in Global settings."));return;}
        active=true;notify(uiText("Preparing ")+title+"…",0);
        const auto source=magazines;const QString workingRoot=root+"/data/magazines";
        auto watcher=new QFutureWatcher<QPair<QString,QString>>(this);
        connect(watcher,&QFutureWatcher<QPair<QString,QString>>::finished,this,[this,watcher,engine,executable,title,id]{
            const auto result=watcher->result();watcher->deleteLater();
            if(!result.second.isEmpty()){active=false;notify(result.second);return;}
            const QString directory=result.first;
            if(directory.contains('"')||directory.contains('\n')){active=false;notify(uiText("The magazine path contains characters that DOSBox cannot use."));return;}
            QString config="[sdl]\nfullscreen=true\noutput=opengl\n";
            config+=engine=="staging"?"window_size=1280x960\nfullscreen_mode=desktop\n":"windowresolution=1280x960\nfullresolution=desktop\n";
            config+="[dosbox]\nmachine=svga_s3\nmemsize=32\n[cpu]\ncore=auto\ncycles=auto\n[render]\naspect=true\n[autoexec]\nmount c \""+directory+"\"\nc:\ncls\ncall run.bat\nexit\n";
            config=EmulationSettings::configured(config,emulation.overrides("",engine),engine);
            refreshMonitors();const auto screen=Displays::resolve(emulation.data["global"].toObject()["monitor"].toString(),monitorData);
            config=EmulationSettings::apply(config,Displays::options(engine,screen));
            const QByteArray bytes=config.toUtf8();const QString digest=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex().left(16));
            const QString sessions=root+"/config/sessions/revistas";QDir().mkpath(sessions);const QString path=sessions+'/'+digest+".conf";
            QSaveFile f(path);
            if(!f.open(QIODevice::WriteOnly)||f.write(bytes)!=bytes.size()||!f.commit()){active=false;notify(uiText("Could not save the magazine configuration."));return;}
            auto env=QProcessEnvironment::systemEnvironment();env.insert("XDG_CONFIG_HOME",root+"/config/emulators");env.insert("XDG_DATA_HOME",root+"/data/emulators");Displays::environment(env,screen);
            process.setProcessEnvironment(env);process.setWorkingDirectory(directory);
            process.setProperty("magazine",true);process.setProperty("console",false);process.setProperty("gameId",QString());process.setProperty("logId","revista-"+id);
            QDir().mkpath(root+"/logs");const QString log=root+"/logs/revista-"+id+"-native.log";
            process.setStandardOutputFile(log,QIODevice::Append);process.setStandardErrorFile(log,QIODevice::Append);
            process.start(executable,EmulationSettings::arguments(engine,path));notify(uiText("DOS magazine · ")+title+uiText(" · Ctrl+F9 to exit"),100);
        });
        watcher->setFuture(QtConcurrent::run([source,id,workingRoot]()->QPair<QString,QString>{
            try{return {source.prepareDos(id,workingRoot),{}};}
            catch(const std::exception &e){return {{},QString::fromUtf8(e.what())};}
        }));
    }catch(const std::exception &e){notify(QString::fromUtf8(e.what()));}
}
