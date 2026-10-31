#include "emulation.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDateTime>
#include <stdexcept>

static const QStringList engines={"staging","classic","dosbox-x"};
EmulationSettings::EmulationSettings(QString r):root(r){
    settingsPath=root+"/config/emulation.json";
    if(!QFileInfo::exists(settingsPath)&&QFileInfo::exists(root+"/data/emulation.json"))settingsPath=root+"/data/emulation.json";
    QFile f(settingsPath);
    if(f.exists()){
        if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("No se pueden leer los ajustes del emulador.");
        auto doc=QJsonDocument::fromJson(f.readAll());
        if(!doc.isObject())throw std::runtime_error("El archivo de ajustes del emulador está dañado.");
        data=doc.object();
    }
}
QString EmulationSettings::name(const QString &e){return e=="staging"?"DOSBox Staging":e=="classic"?"DOSBox clásico":"DOSBox-X";}
QString EmulationSettings::engine(const QString &id) const{
    QString e=data["games"].toObject()[id].toObject()["engine"].toString();
    if(e.isEmpty())e=data["global"].toObject()["engine"].toString("staging");
    return e.isEmpty()?"staging":e;
}
QString EmulationSettings::executable(const QString &e) const{
    QString path=data["paths"].toObject()[e].toString();
    path.replace("${APP}",root);
    if(path.isEmpty()){
        if(e=="staging"){
            path=root+"/runtime/dosbox-staging-linux-x86_64-0.83.0-7b400/dosbox";
            if(!QFileInfo(path).isExecutable())path=QStandardPaths::findExecutable("dosbox-staging");
        }else path=QStandardPaths::findExecutable(e=="classic"?"dosbox":"dosbox-x");
    }
    return QFileInfo(path).isFile()&&QFileInfo(path).isExecutable()?path:QString{};
}
QStringList EmulationSettings::arguments(const QString &e,const QString &config){
    if(e=="staging")return {"--noprimaryconf","--nolocalconf","--conf",config};
    return {"-conf",config};
}
QVariantList EmulationSettings::fields(const QString &e){
    QVariantList list;
    auto add=[&](QString key,QString label,QStringList choices){list<<QVariantMap{{"key",key},{"label",label},{"choices",choices}};};
    add("cpu.core","Núcleo de CPU",{"auto","normal","simple","dynamic"});
    add("cpu.cycles","Ciclos de CPU",{});
    add("dosbox.memsize","Memoria RAM (MB)",QStringList{"1","2","4","8","16","32",e=="classic"?"63":"64"});
    add("dosbox.machine","Tarjeta gráfica",{"svga_s3","vgaonly","ega","cga","tandy","pcjr","hercules"});
    add("sblaster.sbtype","Sound Blaster",{"sb1","sb2","sbpro1","sbpro2","sb16","none"});
    add("midi.mididevice","Dispositivo MIDI",e=="classic"?QStringList{"default","none","alsa"}:QStringList{"default","none","mt32","fluidsynth"});
    add("sdl.fullscreen","Pantalla completa",{"true","false"});
    if(e=="staging")add("render.shader","Estilo de imagen",{"crt-auto","sharp"});
    add("render.aspect","Corregir proporción",{"true","false"});
    return list;
}
QJsonObject EmulationSettings::overrides(const QString &id,const QString &e) const{
    auto result=data["global"].toObject()["options"].toObject()[e].toObject();
    auto local=data["games"].toObject()[id].toObject()["options"].toObject()[e].toObject();
    for(auto it=local.begin();it!=local.end();++it)result[it.key()]=it.value();
    return result;
}
QString EmulationSettings::save(bool global,const QString &id,const QJsonObject &scope,const QJsonObject &paths){
    if(scope.contains("monitor")&&(!scope["monitor"].isString()||scope["monitor"].toString().size()>256||scope["monitor"].toString().contains(QChar('\n'))))return "Monitor inválido.";
    const QString selected=scope["engine"].toString();
    if(!engines.contains(selected)&&!(selected.isEmpty()&&!global))return "Motor desconocido.";
    auto options=scope["options"].toObject();
    for(auto it=options.begin();it!=options.end();++it){
        if(!engines.contains(it.key())||!it.value().isObject())return "Opciones de motor inválidas.";
        QMap<QString,QStringList> allowed;
        for(auto field:fields(it.key())){auto f=field.toMap();allowed[f["key"].toString()]=f["choices"].toStringList();}
        auto values=it.value().toObject();
        for(auto v=values.begin();v!=values.end();++v){
            const QString value=v.value().toString();
            if(!allowed.contains(v.key()))return "Opción desconocida: "+v.key();
            if(v.key()=="cpu.cycles"){
                if(!QRegularExpression("^(auto|max|[1-9][0-9]{0,8}|fixed [1-9][0-9]{0,8})$").match(value).hasMatch())return "Ciclos: usa auto, max o un número positivo (por ejemplo 10000).";
                if(it.key()=="staging"&&value!="auto"&&value!="max"){
                    int cycles=value.section(' ',-1).toInt();
                    if(cycles<50||cycles>2000000)return "Staging admite entre 50 y 2000000 ciclos.";
                }
            }else if(!allowed[v.key()].contains(value))return "Valor inválido: "+v.key();
        }
    }
    for(auto it=paths.begin();it!=paths.end();++it){
        QString p=it.value().toString();p.replace("${APP}",root);
        if(!engines.contains(it.key())||(!p.isEmpty()&&(!QDir::isAbsolutePath(p)||!QFileInfo(p).isFile()||!QFileInfo(p).isExecutable())))return "Indica la ruta absoluta a un ejecutable existente, sin argumentos.";
    }
    QJsonObject next=data;
    if(global){next["global"]=scope;next["paths"]=paths;}
    else{auto games=next["games"].toObject();games[id]=scope;next["games"]=games;}
    if(next==data)return {};
    const QString stamp=QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"),base=root+"/";
    const QString path=settingsPath,backup=base+"backups/"+stamp+"_VETUST_AJUSTES";
    if(QFileInfo::exists(path)&&(!QDir().mkpath(backup)||!QFile::copy(path,backup+"/emulation.json")))return "No se pudo respaldar la configuración anterior.";
    QDir().mkpath(base+"logs/changes");
    QSaveFile log(base+"logs/changes/"+stamp+"_VETUST_AJUSTES.md");
    QByteArray record=("Ajustes de emulación VETUST: "+(global?QString("globales"):id)+"\nArchivo: "+path+"\nRespaldo previo (si existía): "+backup+"\n").toUtf8();
    if(!log.open(QIODevice::WriteOnly)||log.write(record)!=record.size()||!log.commit())return "No se pudo registrar el cambio.";
    if(!QDir().mkpath(QFileInfo(path).absolutePath()))return "No se pudo crear la carpeta de ajustes.";
    QSaveFile f(path);auto bytes=QJsonDocument(next).toJson();
    if(!f.open(QIODevice::WriteOnly)||f.write(bytes)!=bytes.size()||!f.commit())return "No se pudieron guardar los ajustes.";
    data=next;return {};
}
QJsonObject EmulationSettings::values(const QString &config){
    QJsonObject result;QString section;
    for(auto line:config.split('\n')){
        line=line.trimmed();if(line.startsWith('[')){section=line.mid(1,line.indexOf(']')-1).toLower();continue;}
        if(section=="autoexec"||line.startsWith('#')||line.startsWith(';')||!line.contains('='))continue;
        result[section+'.'+line.section('=',0,0).trimmed().toLower()]=line.section('=',1).trimmed();
    }return result;
}
QString EmulationSettings::apply(QString config,const QJsonObject &values){
    // Las sustituciones conservan los comandos de inicio, incluidos los signos igual.
    auto lines=config.split('\n');QString section;QSet<QString> used;
    for(auto &line:lines){
        QString trimmed=line.trimmed();if(trimmed.startsWith('[')){section=trimmed.mid(1,trimmed.indexOf(']')-1).toLower();continue;}
        if(section=="autoexec"||trimmed.startsWith('#')||trimmed.startsWith(';')||!trimmed.contains('='))continue;
        QString key=section+'.'+trimmed.section('=',0,0).trimmed().toLower();
        if(values.contains(key)){line=key.section('.',1)+'='+values[key].toString();used.insert(key);}
    }
    // Se insertan las claves nuevas dentro de su sección, nunca en AUTOEXEC.
    for(auto it=values.begin();it!=values.end();++it){
        if(used.contains(it.key()))continue;
        const QString header='['+it.key().section('.',0,0)+']',line=it.key().section('.',1)+'='+it.value().toString();
        int index=-1;for(int i=0;i<lines.size();++i)if(lines[i].trimmed().compare(header,Qt::CaseInsensitive)==0){index=i;break;}
        if(index>=0)lines.insert(index+1,line);else{lines.prepend(line);lines.prepend(header);}
    }return lines.join('\n');
}

QString EmulationSettings::configured(QString config,const QJsonObject &values,const QString &engine){
    auto options=values;
    if(engine=="staging"){
        const QString shader=options.value("render.shader").toString("sharp");
        // Ambos estilos conservan las proporciones; CRT necesita líneas regulares.
        options["render.shader"]=shader;
        options["render.aspect"]="auto";
        options["render.viewport"]="fit";
        options["render.integer_scaling"]=shader=="sharp"?"off":"auto";
        QStringList filtered;QString section;
        for(const auto &line:config.split('\n')){
            const QString text=line.trimmed().toLower();
            if(text.startsWith('['))section=text;
            // El nombre antiguo no debe prevalecer sobre el estilo elegido.
            if(section=="[render]"&&text.section('=',0,0).trimmed()=="glshader")continue;
            filtered<<line;
        }
        config=filtered.join('\n');
    }
    if(engine=="staging"&&options.contains("cpu.cycles")){
        // Staging 0.82+ utiliza ciclos separados para modo real y protegido.
        const QString cycles=options.take("cpu.cycles").toString();
        QStringList filtered;QString section;
        for(auto line:config.split('\n')){
            QString text=line.trimmed().toLower();
            if(text.startsWith('['))section=text;
            if(section=="[cpu]"&&text.section('=',0,0).trimmed()=="cycles")continue;
            filtered<<line;
        }
        config=filtered.join('\n');
        options["cpu.cpu_cycles"]=cycles=="auto"?"3000":cycles.section(' ',-1);
        options["cpu.cpu_cycles_protected"]=cycles=="auto"?"max":"auto";
        options["cpu.cpu_throttle"]="false";
    }
    return apply(config,options);
}


QString EmulationSettings::consoleConfig(const QString &config,const QString &folder,const QString &toolsFolder,const QString &fileManager,const QString &nortonFolder){
    QString settings,mounts;bool autoexec=false,hasC=false;
    const QRegularExpression mount("^@?(?:img)?mount\\s+",QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression driveC("^@?(?:img)?mount\\s+c\\s+",QRegularExpression::CaseInsensitiveOption);
    // Se conservan los montajes, nunca los ejecutables, menús ni órdenes de arranque.
    for(const QString &line:config.split('\n')){
        const QString trimmed=line.trimmed();
        if(trimmed.startsWith('[')){
            autoexec=trimmed.compare("[autoexec]",Qt::CaseInsensitive)==0;
            if(!autoexec)settings+=line+'\n';
        }else if(!autoexec)settings+=line+'\n';
        else if(mount.match(trimmed).hasMatch()){
            mounts+=trimmed+'\n';hasC=hasC||driveC.match(trimmed).hasMatch();
        }
    }
    // Los títulos que arrancan desde disquete o imagen cruda exponen su carpeta local.
    if(!hasC)mounts+="mount c \""+folder+"\"\n";
    QString setup,hint,launch;
    const QRegularExpression drive("^@?(?:img)?mount\\s+([a-z])\\s+",QRegularExpression::CaseInsensitiveOption|QRegularExpression::MultilineOption);
    QString used="ABCYZ";auto matches=drive.globalMatch(mounts);
    while(matches.hasNext())used+=matches.next().captured(1).toUpper();
    auto addTool=[&](const QString &folder){
        QString letter;
        for(const QChar candidate:QString("TUVWXSRQPONMLKJIHGFED"))if(!used.contains(candidate)){letter=candidate;used+=candidate;break;}
        if(letter.isEmpty())throw std::runtime_error("No hay una unidad libre para las herramientas DOS.");
        mounts+="mount "+letter+" \""+folder+"\"\n";
        setup+="set PATH="+letter+":\\;%PATH%\n";
        return letter+":\\";
    };
    if(!toolsFolder.isEmpty()){
        const QString path=addTool(toolsFolder);
        hint+="echo XTREE: explorar archivos. Dentro de XTree: L cambia disco, Q sale.\n";
        if(fileManager=="xtree")launch=path+"XTGOLD.COM C: /R\n";
    }
    if(!nortonFolder.isEmpty()){
        const QString path=addTool(nortonFolder);
        setup+="set NC="+path+"\nset NORTON="+path+"\n";
        hint+="echo NC: Norton Commander. Alt-F1/F2: discos. F10: volver a DOS.\n";
        if(fileManager=="norton")launch=path+"NC.EXE\n";
    }
    return settings+"\n[autoexec]\n@echo off\n"+mounts+setup+
        "c:\ncd \\\ncls\necho VETUSTware - Consola del juego\nver\necho.\nmount\necho.\necho Cambiar de unidad: escribe su letra y dos puntos (ejemplo D:).\necho DIR: ver archivos. CD: cambiar carpeta. EXIT: volver.\n"+hint+"echo.\n"+launch;
}
