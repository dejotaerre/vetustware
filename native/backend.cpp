#include <QTemporaryDir>
#include "backend.h"
#include "displays.h"
#include <QGuiApplication>
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QProcessEnvironment>
#include <algorithm>
#include <QSet>
#include <QCryptographicHash>
#include <stdexcept>

static void fail(const QString &s){throw std::runtime_error(s.toStdString());}
static QJsonObject readJson(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))fail("No se encuentra "+path);auto d=QJsonDocument::fromJson(f.readAll());if(!d.isObject())fail("Datos inválidos: "+path);return d.object();}
static void writeNew(const QString &path,const QByteArray &data){if(QFileInfo::exists(path))return;QSaveFile f(path);if(!f.open(QIODevice::WriteOnly)||f.write(data)!=data.size()||!f.commit())fail("No se puede escribir "+path);}
static QString fold(QString s){s=s.normalized(QString::NormalizationForm_D).toCaseFolded();s.remove(QRegularExpression("[\\p{Mn}]"));return s;}
ArtProvider::ArtProvider(QString r,QJsonObject a):QQuickImageProvider(QQuickImageProvider::Image,ForceAsynchronousImageLoading),root(r),assets(a){}
void ArtProvider::setAssets(QJsonObject a){QMutexLocker lock(&mutex);assets=a;zip.reset();}
QImage ArtProvider::requestImage(const QString &identifier,QSize *size,const QSize &requested){
    try{
        const QString id=identifier.section('/',0,0);QString asset;
        {QMutexLocker lock(&mutex);asset=assets[id].toString();}if(asset.isEmpty())return {};
        int width=qBound(120,requested.width()>0?requested.width():800,1600);
        const QString path=root+"/data/thumbnails/"+id+"-"+QString::number(width)+".webp";
        QImage img(path);
        if(img.isNull()){
            QByteArray data;
            {QMutexLocker lock(&mutex);const QString path=Storage(root).library+"/Content/XODOSMetadata.zip";if(!zip||zipPath!=path){zip=std::make_unique<Archive>(path);zipPath=path;}data=zip->read(asset);}
            img.loadFromData(data);if(img.isNull())return {};
            img=img.scaled(width,width,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            QDir().mkpath(QFileInfo(path).absolutePath());QSaveFile output(path);
            if(output.open(QIODevice::WriteOnly)){img.save(&output,"WEBP",87);output.commit();}
        }
        if(size)*size=img.size();return img;
    }catch(const std::exception &e){qWarning()<<e.what();return {};}
}
Backend::Backend(QString r,QObject *parent):QObject(parent),root(r),storage(r),library(storage.library),gamesRoot(storage.games),emulation(r){
    QDir().mkpath(root+"/data");
    loadCatalog();
    database=QSqlDatabase::addDatabase("QSQLITE","vetust-native");database.setDatabaseName(root+"/data/personal.sqlite");if(!database.open())fail(database.lastError().text());
    QSqlQuery sql(database);sql.exec("CREATE TABLE IF NOT EXISTS games(id TEXT PRIMARY KEY,favorite INTEGER DEFAULT 0,last_played REAL)");sql.exec("SELECT id,favorite,last_played FROM games");
    while(sql.next()){favorites[sql.value(0).toString()]=sql.value(1).toBool();recent[sql.value(0).toString()]=sql.value(2).toDouble();}
    sql.exec("CREATE TABLE IF NOT EXISTS editions(id TEXT PRIMARY KEY,spanish INTEGER NOT NULL)");
    sql.exec("SELECT id,spanish FROM editions");while(sql.next())spanishPreference[sql.value(0).toString()]=sql.value(1).toBool();
    connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){active=false;notify("No se pudo iniciar DOSBox: "+process.errorString());});
    connect(&process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus exitStatus){
        active=false;
        const QString id=process.property("gameId").toString();
        if(code==0&&exitStatus==QProcess::NormalExit){
            if(process.property("console").toBool()){notify("Consola cerrada · Los cambios en los archivos del juego se conservan");return;}
            double now=QDateTime::currentSecsSinceEpoch();recent[id]=now;QSqlQuery q(database);q.prepare("INSERT INTO games(id,last_played) VALUES(?,?) ON CONFLICT(id) DO UPDATE SET last_played=excluded.last_played");q.addBindValue(id);q.addBindValue(now);q.exec();
            notify("Sesión finalizada · Las partidas permanecen en VETUSTware");
        }else notify("El emulador terminó con un error. Puedes consultar el registro.");
        rebuildHome();refresh();emit selectionChanged();
    });
    selectedId="67bdc1a5-86ff-4cc8-a8e1-293a97de6e95";rebuildHome();refresh();
}
void Backend::loadCatalog(){
    extras.load(library);
    games.clear();indexes.clear();searches.clear();genreList.clear();spanishProfiles={};
    auto catalog=QFileInfo::exists(root+"/data/catalog.json")?readJson(root+"/data/catalog.json"):QJsonObject{};assetMap=catalog["assets"].toObject();profiles=QFileInfo::exists(root+"/data/original-profiles.json")?readJson(root+"/data/original-profiles.json"):QJsonObject{};
    if(QFileInfo::exists(root+"/data/spanish-profiles.json"))spanishProfiles=readJson(root+"/data/spanish-profiles.json");
    // La biblioteca es la fuente de sus perfiles; los datos locales permiten usar juegos ya instalados sin el disco.
    if(QFileInfo::exists(library+"/Personalizaciones/perfiles.zip")){
        Archive custom(library+"/Personalizaciones/perfiles.zip");
        auto document=[&](const QString &name){auto doc=QJsonDocument::fromJson(custom.read(name));if(!doc.isObject())fail("Metadatos personalizados inválidos: "+name);return doc.object();};
        catalog=document("catalog.json");assetMap=catalog["assets"].toObject();
        profiles=document("original-profiles.json");spanishProfiles=document("spanish-profiles.json");
    }
    for(const auto &v:catalog["games"].toArray()){
        auto g=v.toObject();indexes[g["id"].toString()]=games.size();games<<g;
        QString haystack=g["title"].toString()+" "+g["developer"].toString()+" "+g["year"].toString();
        for(const auto &alias:g["aliases"].toArray())haystack+=" "+alias.toString();searches[g["id"].toString()]=fold(haystack);
        for(const auto &genre:g["genre"].toString().split(';',Qt::SkipEmptyParts))if(!genreList.contains(genre))genreList<<genre;
    }
    genreList.sort(Qt::CaseInsensitive);genreList.prepend("Todos los géneros");
}
void Backend::refreshMonitors(){
    const auto current=QGuiApplication::platformName()=="offscreen"?QVariantList{}:Displays::available();
    if(current!=monitorData){monitorData=current;emit monitorsChanged();}
}
QVariantMap Backend::publicGame(const QJsonObject &g) const{
    QVariantMap m=g.toVariantMap();const QString id=g["id"].toString();auto p=profiles[id].toObject();
    m["favorite"]=favorites.value(id);m["tested"]=p["tested"].toBool();m["playable"]=!p["config"].toString().isEmpty()&&!p["archive"].toString().isEmpty();
    const QString engine=emulation.engine(id);
    const bool customized=engine!="staging"||!emulation.overrides(id,engine).isEmpty()||!emulation.data["paths"].toObject()[engine].toString().isEmpty();
    m["tested"]=p["tested"].toBool()&&!customized;
    m["profileLabel"]=EmulationSettings::name(engine)+" · "+(customized?"Ajustes personalizados · Por verificar":p["tested"].toBool()?"Arranque verificado":p["special"].toBool()?"Compatibilidad especial por verificar":"Perfil original importado");
    const bool hasSpanish=spanishProfiles.contains(id);
    m["spanishAvailable"]=hasSpanish||p["language"].toString()=="es";m["hasSpanish"]=hasSpanish;m["spanish"]=hasSpanish&&spanishPreference.value(id,true);
    if(m["spanish"].toBool()){m["tested"]=false;m["profileLabel"]=EmulationSettings::name(engine)+" · Español · Partidas independientes";}
    const QString folder=g["folder"].toString();
    if(folder=="mi1")m["summary"]="Conviértete en pirata junto a Guybrush Threepwood. Explora el Caribe, resuelve enigmas y enfréntate al temible LeChuck en una aventura llena de humor.";
    else if(folder=="Ppersia")m["summary"]="Una hora para escapar de las mazmorras y rescatar a la princesa. Sortea trampas, domina cada salto y enfréntate a los guardias en este clásico de la animación rotoscópica.";
    else if(folder=="DOOM")m["summary"]="Las lunas de Marte han abierto las puertas del infierno. Recorre una base invadida y sobrevive a los demonios en el clásico de id Software.";
    else m["summary"]=g["description"].toString();
    return m;
}
QVariantMap Backend::selected() const{return indexes.contains(selectedId)?publicGame(games[indexes[selectedId]]):QVariantMap{};}
QVariantList Backend::pageGames() const{QVariantList list;for(int i=currentPage*PageSize;i<qMin((currentPage+1)*PageSize,int(filtered.size()));i++)list<<publicGame(games[filtered[i]]);return list;}
void Backend::refresh(){
    filtered.clear();const QString match=fold(query);
    QHash<int,int> collectionRank;const auto members=collections.value(collectionKey);
    for(int i=0;i<members.size();++i)collectionRank[members[i]]=i;
    for(int i=0;i<games.size();i++){
        const auto &g=games[i];const QString id=g["id"].toString();
        if(!collectionKey.isEmpty()&&!collectionRank.contains(i))continue;
        if(!match.isEmpty()&&!searches[id].contains(match))continue;
        if(!genre.isEmpty()&&genre!="Todos los géneros"&&!g["genre"].toString().split(';').contains(genre))continue;
        if(view=="favorites"&&!favorites[id])continue;if(view=="recent"&&recent[id]<=0)continue;
        if(testedOnly&&!publicGame(g).value("tested").toBool())continue;
        filtered<<i;
    }
    std::sort(filtered.begin(),filtered.end(),[&](int a,int b){
        if(view=="recent"){
            const auto ra=recent.value(games[a]["id"].toString()),rb=recent.value(games[b]["id"].toString());
            if(ra!=rb)return ra>rb;
        }
        if(!collectionKey.isEmpty())return collectionRank.value(a)<collectionRank.value(b);
        return QString::localeAwareCompare(games[a]["title"].toString(),games[b]["title"].toString())<0;
    });currentPage=qBound(0,currentPage,pages()-1);emit changed();
}
void Backend::filter(const QString &q,const QString &g,const QString &v,bool only,const QString &collection){query=q;genre=g;view=v;testedOnly=only;collectionKey=collection;currentPage=0;refresh();}
void Backend::selectGame(const QString &id){if(!indexes.contains(id))return;selectedId=id;emit selectionChanged();}
void Backend::pageBy(int d){currentPage=qBound(0,currentPage+d,pages()-1);emit changed();}
void Backend::moveSelection(int d){if(filtered.isEmpty())return;int index=filtered.indexOf(indexes.value(selectedId,-1));index=qBound(0,index+d,int(filtered.size())-1);int previous=currentPage;currentPage=index/PageSize;selectGame(games[filtered[index]]["id"].toString());if(previous!=currentPage)emit changed();}
void Backend::toggleFavorite(){if(!indexes.contains(selectedId))return;bool yes=!favorites[selectedId];QSqlQuery q(database);q.prepare("INSERT INTO games(id,favorite) VALUES(?,?) ON CONFLICT(id) DO UPDATE SET favorite=excluded.favorite");q.addBindValue(selectedId);q.addBindValue(yes);if(!q.exec()){notify("No se pudo guardar el favorito.");return;}favorites[selectedId]=yes;rebuildHome();refresh();emit selectionChanged();}
void Backend::notify(const QString &text,int value){message=text;percent=value;emit statusChanged();}
void Backend::dismissStatus(){if(!active)notify("");}
void Backend::openLog(){QString id=process.property("logId").toString();if(id.isEmpty())id=selectedId+(spanishProfiles.contains(selectedId)&&spanishPreference.value(selectedId,true)?"-es":"");const QString path=root+"/logs/"+id+"-native.log";if(QFileInfo::exists(path))QDesktopServices::openUrl(QUrl::fromLocalFile(path));else notify("Este juego aún no tiene un registro de ejecución nativa.");}
void Backend::openExtra(int index){
    if(active)return;
    active=true;notify("Abriendo extra…");
    const auto source=extras;const QString id=selectedId,cache=root+"/cache/extras";
    auto watcher=new QFutureWatcher<QPair<QString,QString>>(this);
    connect(watcher,&QFutureWatcher<QPair<QString,QString>>::finished,this,[this,watcher]{
        const auto result=watcher->result();watcher->deleteLater();active=false;
        if(!result.second.isEmpty()){notify(result.second);return;}
        const QFileInfo file(result.first);notify("Extra preparado: "+file.fileName());
        emit extraReady(QUrl::fromLocalFile(result.first),file.fileName(),file.suffix().compare("pdf",Qt::CaseInsensitive)==0);
    });
    watcher->setFuture(QtConcurrent::run([source,id,index,cache]()->QPair<QString,QString>{
        try{return {source.prepare(id,index,cache),{}};}
        catch(const std::exception &e){return {{},QString::fromUtf8(e.what())};}
    }));
}
void Backend::openManual(){
    try{const QString name=games[indexes[selectedId]]["manual"].toString();if(name.isEmpty())return;QString path=root+"/data/manuals/"+selectedId+".pdf";QDir().mkpath(QFileInfo(path).absolutePath());if(!QFileInfo::exists(path)){Archive zip(library+"/Content/XODOSMetadata.zip");writeNew(path,zip.read(name));}emit manualReady(QUrl::fromLocalFile(path));}
    catch(const std::exception &e){notify(QString::fromUtf8(e.what()));}
}
QString Backend::makeConfig(const QJsonObject &game,const QJsonObject &profile,const QString &folder,const QString &environment,const QString &engine,const QJsonObject &options,bool console,const QString &fileManager){
    const QString id=game["id"].toString();const QString legacy=root+"/config/legacy-profiles/"+id+".conf";
    const QString old=QFileInfo::exists(legacy)?legacy:gamesRoot+"/"+id+"/vetust.conf";
    // Se respetan los tres perfiles ya probados y sus partidas existentes.
    const bool preserved=profile["tested"].toBool()&&QFileInfo::exists(old);
    QString source=profile["config"].toString();
    if(preserved){QFile f(old);if(!f.open(QIODevice::ReadOnly))fail("No se puede leer el perfil probado.");source=QString::fromUtf8(f.readAll());}
    source=storage.expand(source);
    QString result,section;bool autoexec=false;
    for(QString line:source.split('\n')){
        line=line.trimmed();if(line.startsWith('[')){section=line.toLower();autoexec=section=="[autoexec]";result+=line+'\n';continue;}
        if(line.isEmpty()||line.startsWith('#')){result+=line+'\n';continue;}
        if(!autoexec){
            QString key=line.section('=',0,0).trimmed().toLower();
            if(!preserved&&key=="fullscreen")line="fullscreen=false";
            if(key=="output")line="output=opengl";
            if(engine=="staging"&&key=="windowresolution")line="window_size=1280x960";
            if(engine=="staging"&&key=="fullresolution")line="fullscreen_mode=desktop";
            if(key=="mapperfile"||key=="midiconfig"||key=="fluid.driver"||key=="fluid.gain"||key=="fluid.soundfont"||key=="mt32.romdir")continue;
            if(engine=="staging"&&key=="machine"&&line.section('=',1).trimmed()=="vgaonly")line="machine=svga_s3";
            result+=line+'\n';continue;
        }
        // Solo se traducen rutas del anfitrión: las rutas internas de DOS se conservan.
        QRegularExpression tokens("\"[^\"]*\"|[^\\s]+");auto iterator=tokens.globalMatch(line);QList<QRegularExpressionMatch> matches;
        while(iterator.hasNext())matches<<iterator.next();
        for(auto it=matches.crbegin();it!=matches.crend();++it){
            QString token=it->captured(),clean=token;clean.remove('"');clean.replace('\\','/');
            if(clean.contains("eXoDOS/",Qt::CaseInsensitive)||clean.compare("./eXoDOS",Qt::CaseInsensitive)==0){
                int pos=clean.indexOf("eXoDOS",0,Qt::CaseInsensitive);QString relative=clean.mid(pos);
                if(relative.split('/').contains(".."))fail("Ruta de montaje inesperada.");
                QString translated=resolveCase(environment+'/'+relative);
                if(!QFileInfo::exists(translated))fail("Falta un archivo requerido por el perfil: "+relative);
                line.replace(it->capturedStart(),it->capturedLength(),'"'+translated+'"');
            }else if(clean.startsWith('/')&&QRegularExpression("^@?(img)?mount\\s",QRegularExpression::CaseInsensitiveOption).match(line).hasMatch()){
                const QString translated=resolveCase(clean);
                if(!QFileInfo::exists(translated))fail("No existe el archivo o carpeta de montaje: "+clean);
                line.replace(it->capturedStart(),it->capturedLength(),'"'+translated+'"');
            }
        }
        result+=line+'\n';
    }
    // Staging necesita sus secciones de sintetizadores para los perfiles MT-32 y MIDI.
    if(engine=="staging")result+="\n[mt32]\nromdir="+library+"/eXo/mt32\n[fluidsynth]\nsoundfont="+library+"/eXo/mt32/SoundCanvas.sf2\n";
    if(engine=="dosbox-x")result=EmulationSettings::apply(result,{{"midi.mt32.romdir",library+"/eXo/mt32"},{"midi.fluid.soundfont",library+"/eXo/mt32/SoundCanvas.sf2"}});
    result=EmulationSettings::configured(result,options,engine);
    if(console)result=EmulationSettings::consoleConfig(result,folder,xTreeAvailable()?root+"/runtime/dos-tools/xtgold":QString(),fileManager,nortonAvailable()?root+"/runtime/dos-tools/norton":QString());
    const QString digest=QString::fromLatin1(QCryptographicHash::hash(result.toUtf8(),QCryptographicHash::Sha256).toHex().left(16));
    const QString configDir=root+"/config/sessions/"+id;QDir().mkpath(configDir);
    const QString path=configDir+"/vetust-"+(console?QString("console-"):QString())+engine+'-'+digest+".conf";writeNew(path,result.toUtf8());return path;
}
void Backend::restorePrepared(const QJsonObject &profile,const QString &readyPath,const std::function<void(int)> &callback){
    const QString name=profile["preparedRoot"].toString();
    if(name.isEmpty()||QFileInfo::exists(readyPath))return;
    if(!QRegularExpression("^[a-zA-Z0-9_-]+$").match(name).hasMatch())fail("Identificador de edición inválido.");
    const QString target=gamesRoot+'/'+name;
    if(!readyPath.startsWith(target+'/'))fail("Ruta de edición fuera de su carpeta.");
    if(QFileInfo::exists(target))fail("Hay archivos de esta edición sin una instalación completa. Consérvalos y revisa la carpeta antes de recuperarla.");
    QTemporaryDir temporary(gamesRoot+"/.vetust-"+name+"-XXXXXX");
    if(!temporary.isValid())fail("No se puede preparar la copia jugable.");
    Archive archive(library+"/Personalizaciones/juegos.zip");
    const QString extracted=archive.extract(temporary.path(),name,callback,true);
    const QString extractedPath=temporary.path()+'/'+extracted;
    if(!QFileInfo::exists(extractedPath+readyPath.mid(target.size())))fail("La edición de la biblioteca está incompleta.");
    if(!QDir().rename(extractedPath,target))fail("No se pudo terminar la recuperación del juego.");
}
void Backend::restoreTools(){
    const QString archive=library+"/Personalizaciones/herramientas.zip";
    if(!QFileInfo::exists(archive))return;
    const QString destination=root+"/runtime/dos-tools";
    if(!QDir().mkpath(destination))fail("No se puede preparar la carpeta de herramientas DOS.");
    for(const QString &name:QStringList{"xtgold","norton"}){
        if(QFileInfo::exists(destination+'/'+name))continue;
        QTemporaryDir temporary(destination+"/.tools-XXXXXX");if(!temporary.isValid())fail("No se pueden instalar las herramientas DOS.");
        Archive zip(archive);const QString extracted=zip.extract(temporary.path(),name,[](int){},true);
        if(!QDir().rename(temporary.path()+'/'+extracted,destination+'/'+name))fail("No se pudo instalar la herramienta DOS.");
    }
}
LaunchPlan Backend::prepare(const QJsonObject &game,const QJsonObject &profile,const QString &engine,const QJsonObject &options,const std::function<void(int)> &callback,bool console,const QString &fileManager){
    LaunchPlan plan;
    try{
        if(gamesRoot==root+"/data/games"&&!QDir().mkpath(gamesRoot))fail("No se puede crear la carpeta de copias jugables.");
        if(gamesRoot.isEmpty()||!QFileInfo(gamesRoot).isDir())fail("La carpeta de juegos no está disponible. Conecta el disco o revisa Biblioteca y carpetas.");
        if(console)restoreTools();
        if(profile.contains("editionDirectory")){
            const QString directory=storage.expand(profile["editionDirectory"].toString());
            restorePrepared(profile,directory+"/.ready",callback);
            if(!QFileInfo::exists(directory+"/.ready"))fail("La edición personalizada no está instalada completamente ni tiene una copia recuperable en la biblioteca.");

            plan.directory=directory;plan.config=makeConfig(game,profile,directory,directory,engine,options,console,fileManager);return plan;
        }
        QString id=game["id"].toString(),folderName=game["folder"].toString(),target=gamesRoot+"/"+id;
        restorePrepared(profile,target+"/.ready",callback);
        if(!QFileInfo::exists(target+"/.ready")){
            if(library.isEmpty()||!QFileInfo(library+"/eXo/eXoDOS").isDir())fail("La colección original no está disponible. Conecta el disco o revisa Biblioteca y carpetas.");
            const QString partial=target+".native-partial";
            if(QFileInfo::exists(partial))QDir(partial).removeRecursively();
            try{
                Archive zip(library+"/eXo/eXoDOS/"+profile["archive"].toString());QString extracted=zip.extract(partial,folderName,callback);
                if(extracted!=folderName&&!QDir().rename(partial+'/'+extracted,partial+'/'+folderName))fail("No se pudo normalizar la carpeta del juego.");
                if(QFileInfo::exists(target))fail("Hay una instalación incompleta que debe revisarse antes de continuar.");
                writeNew(partial+"/.ready",{});if(!QDir().rename(partial,target))fail("No se pudo terminar la instalación.");
            }catch(...){QDir(partial).removeRecursively();throw;}
        }
        const QString folder=target+'/'+folderName,environment=root+"/cache/environments/"+id;QDir().mkpath(environment+"/eXoDOS");
        const QString link=environment+"/eXoDOS/"+folderName;
        if(QFileInfo(link).isSymLink()&&QFileInfo(link).symLinkTarget()!=folder)QFile::remove(link);
        if(!QFileInfo::exists(link)&&!QFile::link(folder,link))fail("No se pudo preparar el entorno del juego.");
        // Las ROM y fuentes se consultan en lectura; solo el juego se monta con escritura.
        for(const QString &resource:QStringList{"mt32"}){
            const QString link=environment+'/'+resource,target=library+"/eXo/"+resource;
            if(QFileInfo(link).isSymLink()&&QFileInfo(link).symLinkTarget()!=target)QFile::remove(link);
            if(!library.isEmpty()&&QFileInfo::exists(target)&&!QFileInfo::exists(link))QFile::link(target,link);
        }
        plan.config=makeConfig(game,profile,folder,environment,engine,options,console,fileManager);plan.directory=environment;
    }catch(const std::exception &e){plan.error=QString::fromUtf8(e.what());}
    return plan;
}
void Backend::play(){launch(false);}
bool Backend::xTreeAvailable() const{return QFileInfo::exists(library+"/Personalizaciones/herramientas.zip")||QFileInfo::exists(root+"/runtime/dos-tools/xtgold/xtgold.com")&&QFileInfo::exists(root+"/runtime/dos-tools/xtgold/xtg.exe")&&QFileInfo::exists(root+"/runtime/dos-tools/xtgold/XTREE.BAT");}
bool Backend::nortonAvailable() const{return QFileInfo::exists(library+"/Personalizaciones/herramientas.zip")||QFileInfo::exists(root+"/runtime/dos-tools/norton/NC.EXE")&&QFileInfo::exists(root+"/runtime/dos-tools/norton/NCMAIN.EXE");}
void Backend::openConsole(const QString &fileManager){launch(true,fileManager);}
void Backend::launch(bool console,const QString &fileManager){
    if(active||!indexes.contains(selectedId))return;
    if(fileManager=="xtree"&&!xTreeAvailable()){notify("XTree Gold no está instalado en VETUSTware.");return;}
    if(fileManager=="norton"&&!nortonAvailable()){notify("Norton Commander no está instalado en VETUSTware.");return;}
    if(!fileManager.isEmpty()&&fileManager!="xtree"&&fileManager!="norton"){notify("Explorador DOS desconocido.");return;}
    auto game=games[indexes[selectedId]],profile=profiles[selectedId].toObject();
    const QString catalogId=selectedId;
    if(spanishProfiles.contains(selectedId)&&spanishPreference.value(selectedId,true)){
        profile=spanishProfiles[selectedId].toObject();game["id"]=selectedId+"-es";
    }
    if(profile["config"].toString().isEmpty()||(profile["archive"].toString().isEmpty()&&!profile.contains("editionDirectory"))){notify("No hay configuración o ZIP disponible para este juego.");return;}
    const QString engine=emulation.engine(selectedId),executable=emulation.executable(engine);
    const QJsonObject options=emulation.overrides(selectedId,engine);
    if(executable.isEmpty()){notify(EmulationSettings::name(engine)+" no está instalado o su ruta no es válida. Revisa Ajustes globales.");return;}
    active=true;notify("Preparando "+game["title"].toString(),0);
    auto watcher=new QFutureWatcher<LaunchPlan>(this);
    connect(watcher,&QFutureWatcher<LaunchPlan>::finished,this,[this,watcher,game,engine,executable,catalogId,console]{
        const auto plan=watcher->result();watcher->deleteLater();
        if(!plan.error.isEmpty()){active=false;notify(plan.error);return;}
        const QString id=game["id"].toString()+(console?"-console":"");QDir().mkpath(root+"/logs");process.setProperty("console",console);process.setProperty("gameId",catalogId);process.setProperty("logId",id);
        process.setWorkingDirectory(plan.directory);auto env=QProcessEnvironment::systemEnvironment();env.insert("XDG_CONFIG_HOME",root+"/config/emulators");env.insert("XDG_DATA_HOME",root+"/data/emulators");
        refreshMonitors();
        const QString preferred=emulation.data["global"].toObject()["monitor"].toString();
        const auto screen=Displays::resolve(preferred,monitorData);
        Displays::environment(env,screen);process.setProcessEnvironment(env);
        QString config=plan.config;
        const auto displayOptions=Displays::options(engine,screen);
        if(!displayOptions.isEmpty()){
            try{
                QFile source(config);if(!source.open(QIODevice::ReadOnly))fail("No se puede leer la configuración del juego.");
                const auto contents=EmulationSettings::apply(QString::fromUtf8(source.readAll()),displayOptions).toUtf8();
                const QString digest=QString::fromLatin1(QCryptographicHash::hash(contents,QCryptographicHash::Sha256).toHex().left(16));
                config=QFileInfo(config).absolutePath()+"/vetust-monitor-"+engine+"-"+digest+".conf";
                writeNew(config,contents);
            }catch(const std::exception &e){active=false;notify(QString::fromUtf8(e.what()));return;}
        }
        process.setStandardOutputFile(root+"/logs/"+id+"-native.log",QIODevice::Append);process.setStandardErrorFile(root+"/logs/"+id+"-native.log",QIODevice::Append);
        process.start(executable,EmulationSettings::arguments(engine,config));
        notify((console?"Consola DOS: ":"En juego: ")+game["title"].toString()+" · "+EmulationSettings::name(engine)+" · Ctrl+F9 para salir",100);
    });
    watcher->setFuture(QtConcurrent::run([this,game,profile,engine,options,console,fileManager]{return prepare(game,profile,engine,options,[this](int n){QMetaObject::invokeMethod(this,[this,n]{percent=n;emit statusChanged();},Qt::QueuedConnection);},console,fileManager);}));
}
QString Backend::diagnostics() const{
    int available=0,tested=0,special=0;for(auto p:profiles){auto o=p.toObject();available+=!o["config"].toString().isEmpty()&&!o["archive"].toString().isEmpty();tested+=o["tested"].toBool();special+=o["special"].toBool();}
    return QString("Juegos: %1\nPerfiles disponibles: %2\nArranque verificado: %3\nCompatibilidad especial: %4\nFavoritos: %5\n").arg(games.size()).arg(available).arg(tested).arg(special).arg(std::count(favorites.begin(),favorites.end(),true));
}

QVariantMap Backend::storageSettings() const{
    const bool sourceAvailable=!library.isEmpty()&&QFileInfo(library+"/Content/XODOSMetadata.zip").isFile();
    const bool gamesAvailable=!gamesRoot.isEmpty()&&QFileInfo(gamesRoot).isDir();
    return {{"library",library},{"games",gamesRoot},{"needsSetup",library.isEmpty()||gamesRoot.isEmpty()},
        {"status",sourceAvailable&&gamesAvailable?"Biblioteca disponible":!gamesAvailable?"Juegos no disponibles: conecta el disco o revisa las carpetas.":"Colección original no disponible. Los juegos instalados y las carátulas guardadas se conservan."}};
}
QString Backend::saveStorage(const QUrl &source,const QUrl &installed){
    if(active)return "Espera a que termine la sesión actual.";
    const QString lib=source.toLocalFile(),target=installed.toLocalFile();
    if(target==root+"/data/games"&&!QDir().mkpath(target))return "No se puede crear la carpeta de juegos.";
    auto error=Storage::validate(lib,target);if(!error.isEmpty())return error;
    // Cambiar de carpeta conserva los datos. El traslado físico se hace por separado.
    if(!gamesRoot.isEmpty()&&target!=gamesRoot){
        for(const auto &name:QDir(gamesRoot).entryList(QDir::Dirs|QDir::NoDotAndDotDot)){
            if(QFileInfo(gamesRoot+'/'+name+"/.ready").exists()&&!QFileInfo(target+'/'+name+"/.ready").exists())
                return "La nueva carpeta no contiene todos tus juegos instalados. Copia primero su contenido para conservar las partidas.";
        }
        for(const auto &collection:{profiles,spanishProfiles})for(const auto &value:collection){
            const QString current=storage.expand(value.toObject()["editionDirectory"].toString());
            if(current.startsWith(gamesRoot+'/')&&QFileInfo::exists(current+"/.ready")&&!QFileInfo::exists(target+current.mid(gamesRoot.size())+"/.ready"))
                return "Falta una edición personal en la nueva carpeta. Copia también las ediciones españolas y sus partidas.";
        }
    }
    if(!QDir().mkpath(root+"/cache"))return "No se puede preparar la importación.";
    active=true;notify("Preparando biblioteca…",0);
    auto watcher=new QFutureWatcher<QString>(this);
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher]{
        const QString error=watcher->result();watcher->deleteLater();active=false;
        if(!error.isEmpty()){notify(error);return;}
        storage=Storage(root);library=storage.library;gamesRoot=storage.games;
        try{loadCatalog();if(!indexes.contains(selectedId))selectedId=games.isEmpty()?QString():games.first()["id"].toString();rebuildHome();refresh();emit selectionChanged();emit storageChanged();notify("Biblioteca y carpetas guardadas");}
        catch(const std::exception &e){notify(QString::fromUtf8(e.what()));}
    });
    watcher->setFuture(QtConcurrent::run([this,lib,target]{
        // Los catálogos personales existentes se mantienen al localizar de nuevo la colección.
        if(games.isEmpty()){
            QProcess importer;importer.start("python3",{root+"/helpers/import_library.py","--library",lib,"--data",root+"/data"});
            if(!importer.waitForStarted()||!importer.waitForFinished(-1)||importer.exitCode()!=0)return QString("No se pudo importar la biblioteca: ")+QString::fromUtf8(importer.readAllStandardError());
        }
        return storage.save(lib,target);
    }));
    return {};
}
QVariantMap Backend::emulatorSettings(bool global) const{
    QVariantList engines;
    for(const QString &id:QStringList{"staging","classic","dosbox-x"}){
        const QString path=emulation.executable(id);
        engines<<QVariantMap{{"id",id},{"name",EmulationSettings::name(id)},{"available",!path.isEmpty()},{"path",path}};
    }
    QJsonObject scope=global?emulation.data["global"].toObject():emulation.data["games"].toObject()[selectedId].toObject();
    if(global&&scope["engine"].toString().isEmpty())scope["engine"]="staging";
    return {{"scope",scope.toVariantMap()},{"paths",emulation.data["paths"].toObject().toVariantMap()},{"engines",engines},{"effectiveEngine",emulation.engine(selectedId)},{"globalEngine",emulation.engine("")},{"gameId",selectedId}};
}
QVariantList Backend::emulatorFields(const QString &engine) const{return EmulationSettings::fields(engine);}
QVariantMap Backend::inheritedSettings(bool global,const QString &engine) const{
    QJsonObject values;
    if(!global){
        const auto game=games[indexes.value(selectedId)];const auto profile=profiles[selectedId].toObject();
        QString source=profile["config"].toString();QFile f(gamesRoot+"/"+selectedId+"/vetust.conf");
        if(profile["tested"].toBool()&&f.open(QIODevice::ReadOnly))source=QString::fromUtf8(f.readAll());
        if(spanishProfiles.contains(selectedId)&&spanishPreference.value(selectedId,true))source=spanishProfiles[selectedId].toObject()["config"].toString();
        values=EmulationSettings::values(source);
        values["sdl.fullscreen"]="false";
        if(engine=="staging"&&values.value("dosbox.machine").toString()=="vgaonly")values["dosbox.machine"]="svga_s3";
    }
    QVariantMap result;
    auto defaults=emulation.data["global"].toObject()["options"].toObject()[engine].toObject();
    for(auto field:EmulationSettings::fields(engine)){
        QString key=field.toMap()["key"].toString();
        result[key]=global?QString("Perfil de cada juego"):defaults.contains(key)?defaults[key].toString()+" · Global":values.contains(key)?values[key].toString()+" · Perfil":QString("Predeterminado del motor");
    }
    return result;
}
QString Backend::saveEmulatorSettings(bool global,const QString &id,const QVariantMap &scope,const QVariantMap &paths){
    if(active)return "Espera a que termine el juego antes de cambiar la configuración.";
    if(!global&&!indexes.contains(id))return "Juego desconocido.";
    QString error=emulation.save(global,id,QJsonObject::fromVariantMap(scope),QJsonObject::fromVariantMap(paths));
    if(error.isEmpty()){rebuildHome();refresh();emit selectionChanged();notify(global?"Ajustes globales guardados":"Ajustes de este juego guardados");}
    return error;
}

void Backend::setSpanish(bool enabled){
    if(active||!spanishProfiles.contains(selectedId))return;
    QSqlQuery q(database);q.prepare("INSERT INTO editions(id,spanish) VALUES(?,?) ON CONFLICT(id) DO UPDATE SET spanish=excluded.spanish");
    q.addBindValue(selectedId);q.addBindValue(enabled);
    if(!q.exec()){notify("No se pudo guardar la edición elegida.");return;}
    spanishPreference[selectedId]=enabled;emit selectionChanged();rebuildHome();refresh();
}
