#include "backend.h"
#include "i18n.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QImageReader>
#include <QBuffer>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <stdexcept>

namespace {
const QStringList textFields={"title","year","developer","publisher","genre"};
const QStringList imageFields={"cover","box3d","background"};
const QStringList languages={"en","es","pt_BR"};
bool validId(const QString &id){return QRegularExpression("^[a-zA-Z0-9_-]{1,100}$").match(id).hasMatch();}
QJsonObject readObject(const QString &path){
    QFile f(path);if(!f.open(QIODevice::ReadOnly)||f.size()>4*1024*1024)throw std::runtime_error("Cannot read customization data.");
    QJsonParseError error;auto d=QJsonDocument::fromJson(f.readAll(),&error);
    if(error.error!=QJsonParseError::NoError||!d.isObject())throw std::runtime_error("Invalid customization data.");
    return d.object();
}
void atomicWrite(const QString &path,const QByteArray &bytes){
    QSaveFile f(path);if(!f.open(QIODevice::WriteOnly)||f.write(bytes)!=bytes.size()||!f.commit())throw std::runtime_error("Cannot save customization data.");
}
}

QString Backend::originalDescription(const QJsonObject &game,const QString &language) const{
    if(language=="en")return game["description"].toString();
    QString value=game["description"].toString();
    QStringList codes{language.section('_',0,0)};if(!codes.contains(language))codes<<language;
    for(const auto &code:codes){
        const auto path=storage.metadataDirectory("dosbox")+"/descriptions."+code+".json";
        if(!QFileInfo::exists(path))continue;
        const auto text=readObject(path).value(game["id"].toString());
        if(text.isString()&&!text.toString().trimmed().isEmpty())value=text.toString();
    }
    return value;
}

void Backend::applyCustomizations(){
    customGames.clear();
    const auto base=storage.metadataDirectory("dosbox")+"/customizations";
    for(auto it=originalGames.begin();it!=originalGames.end();++it){
        if(!validId(it.key()))continue;
        const auto path=base+"/"+it.key()+".json";if(!QFileInfo::exists(path))continue;
        try{
            auto record=readObject(path);if(record["format"].toInt()!=1||record["id"].toString()!=it.key())continue;
            QJsonObject fields,descriptions;
            const auto stored=record["fields"].toObject();
            for(const auto &key:textFields)if(stored[key].isString())fields[key]=stored[key];
            // Las imágenes propias se resuelven desde la biblioteca, sin rutas personales persistidas.
            auto resolve=[&](const QString &ref){
                if(ref.startsWith("images/")&&QRegularExpression("^images/[a-f0-9]{64}\\.png$").match(ref).hasMatch()){
                    const auto file=QFileInfo(base+"/"+ref);
                    if(!file.isFile()||!file.canonicalFilePath().startsWith(QFileInfo(base).canonicalFilePath()+"/"))return QString();
                    const auto id="custom-"+QCryptographicHash::hash(file.canonicalFilePath().toUtf8()+ref.toUtf8(),QCryptographicHash::Sha256).toHex();
                    assetMap[id]=QUrl::fromLocalFile(file.absoluteFilePath()).toString();return QString(id);
                }
                return assetMap.contains(ref)?ref:QString();
            };
            for(const auto &key:imageFields)if(stored[key].isString())fields[key]=resolve(stored[key].toString());
            if(stored["shots"].isArray()){
                QJsonArray shots;for(const auto &v:stored["shots"].toArray()){const auto id=resolve(v.toString());if(!id.isEmpty())shots.append(id);}fields["shots"]=shots;
            }
            if(fields.contains("cover"))fields["realCover"]=!fields["cover"].toString().isEmpty();
            for(const auto &lang:languages)if(record["descriptions"].toObject()[lang].isString())descriptions[lang]=record["descriptions"].toObject()[lang];
            customGames[it.key()]=QJsonObject{{"fields",fields},{"descriptions",descriptions}};
        }catch(const std::exception &e){qWarning()<<path<<e.what();}
    }
}

QVariantMap Backend::gameCustomization(const QString &id) const{
    if(!originalGames.contains(id)||!validId(id))return {{"error",uiText("Game not found.")}};
    try{
        const auto path=storage.metadataDirectory("dosbox")+"/customizations/"+id+".json";
        if(QFileInfo::exists(path)){
            const auto saved=readObject(path);
            if(saved["format"].toInt()!=1||saved["id"].toString()!=id)throw std::runtime_error("Invalid customization data.");
        }
        const auto original=originalGames.value(id);
        const auto game=games.value(indexes.value(id));QVariantMap fields,descriptions;
        for(const auto &key:textFields)fields[key]=game[key].toString();
        for(const auto &key:imageFields)fields[key]=game[key].toString();
        fields["shots"]=game["shots"].toArray().toVariantList();
        const auto overrides=customGames.value(id)["descriptions"].toObject();
        for(const auto &lang:languages)descriptions[lang]=overrides.contains(lang)?overrides[lang].toString():originalDescription(original,lang);
        return {{"fields",fields},{"descriptions",descriptions},{"customized",QFileInfo::exists(storage.metadataDirectory("dosbox")+"/customizations/"+id+".json")}};
    }catch(const std::exception &e){return {{"error",QString::fromUtf8(e.what())}};}
}

QString Backend::saveGameCustomization(const QString &id,const QVariantMap &draft){
    if(busy())return uiText("Close the running session before editing.");
    if(!originalGames.contains(id)||!validId(id))return uiText("Game not found.");
    const auto metadata=storage.metadataDirectory("dosbox");
    if(metadata.isEmpty()||!QFileInfo(metadata).isDir())return uiText("Connect the DOSBox library before editing.");
    try{
        const auto original=originalGames.value(id);
        const auto values=draft.value("fields").toMap();
        if(values.value("title").toString().trimmed().isEmpty())return uiText("Enter a game title.");
        QJsonObject fields,descriptions;
        for(const auto &key:textFields){const auto text=values.value(key).toString().trimmed();if(text.size()>1000)return uiText("A field is too long.");if(text!=original[key].toString())fields[key]=text;}
        for(const auto &lang:languages){
            const auto input=draft.value("descriptions").toMap();if(!input.contains(lang))continue;
            const auto text=input[lang].toString();if(text.size()>100000)return uiText("A field is too long.");
            if(text!=originalDescription(original,lang))descriptions[lang]=text;
        }
        const auto base=metadata+"/customizations";
        QMap<QString,QByteArray> pending;
        auto imageRef=[&](const QString &ref)->QString{
            if(ref.isEmpty())return {};
            QString local;
            if(QUrl(ref).isLocalFile())local=QUrl(ref).toLocalFile();
            else if(assetMap.contains(ref)){
                const auto asset=assetMap[ref].toString();
                if(!QUrl(asset).isLocalFile())return ref;
                local=QUrl(asset).toLocalFile();
            }else throw std::runtime_error("Unknown image reference.");
            QFileInfo file(local);if(!file.isFile()||file.size()>80*1024*1024)throw std::runtime_error("Image missing or larger than 80 MiB.");
            // Reutilizar las imágenes ya importadas evita recomprimirlas al editar el texto.
            if(file.canonicalPath()==QFileInfo(base+"/images").canonicalFilePath()&&QRegularExpression("^[a-f0-9]{64}\\.png$").match(file.fileName()).hasMatch())return "images/"+file.fileName();
            QImageReader reader(local);reader.setAutoTransform(true);const auto size=reader.size();
            if(!size.isValid()||qint64(size.width())*size.height()>100000000)throw std::runtime_error("Invalid image dimensions.");
            if(size.width()>4096||size.height()>4096)reader.setScaledSize(size.scaled(4096,4096,Qt::KeepAspectRatio));
            auto image=reader.read();if(image.isNull())throw std::runtime_error("Could not open the image.");
            QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);if(!image.save(&buffer,"PNG"))throw std::runtime_error("Could not save the image.");
            const QString name="images/"+QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()+".png";pending[name]=bytes;return name;
        };
        for(const auto &key:imageFields){const auto ref=imageRef(values.value(key).toString());if(ref!=original[key].toString())fields[key]=ref;}
        QJsonArray shots;const auto source=values.value("shots").toList();if(source.size()>100)return uiText("Use no more than 100 gallery images.");
        for(const auto &item:source){const auto ref=imageRef(item.toString());if(!ref.isEmpty())shots.append(ref);}
        if(shots!=original["shots"].toArray())fields["shots"]=shots;
        if(!QDir().mkpath(base+"/images"))return uiText("Cannot write to the DOSBox library.");
        // El JSON se publica al final: una imagen inválida nunca deja una ficha parcialmente guardada.
        for(auto it=pending.begin();it!=pending.end();++it)if(!QFileInfo::exists(base+"/"+it.key()))atomicWrite(base+"/"+it.key(),it.value());
        const auto path=base+"/"+id+".json";
        atomicWrite(path,QJsonDocument(QJsonObject{{"format",1},{"id",id},{"fields",fields},{"descriptions",descriptions}}).toJson());
        refreshCustomizations();return {};
    }catch(const std::exception &e){return uiText("Could not save game details: ")+QString::fromUtf8(e.what());}
}

QString Backend::resetGameCustomization(const QString &id){
    if(busy())return uiText("Close the running session before editing.");
    if(!originalGames.contains(id)||!validId(id))return uiText("Game not found.");
    const auto metadata=storage.metadataDirectory("dosbox");
    if(metadata.isEmpty()||!QFileInfo(metadata).isDir())return uiText("Connect the DOSBox library before editing.");
    const auto path=metadata+"/customizations/"+id+".json";
    if(QFileInfo::exists(path)&&!QFile::remove(path))return uiText("Cannot restore original details.");
    refreshCustomizations();return {};
}

void Backend::refreshCustomizations(){
    loadCatalog();emit storageChanged();rebuildHome();refresh();emit selectionChanged();
}
