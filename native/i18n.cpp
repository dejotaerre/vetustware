#include "i18n.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <algorithm>
#include <cstring>

static QStringList placeholders(const QString &text) {
    QStringList result;
    auto it = QRegularExpression("%L?[1-9][0-9]?").globalMatch(text);
    while (it.hasNext()) result << it.next().captured();
    result.sort();
    return result;
}

QString DictionaryTranslator::translate(const char *context, const char *source, const char *, int) const {
    if (!context || std::strcmp(context, "Vetust") != 0) return {};
    QReadLocker guard(&lock);
    return messages.value(QString::fromUtf8(source)).toString();
}
void DictionaryTranslator::setMessages(const QJsonObject &value) {
    QWriteLocker guard(&lock);
    messages = value;
}
QJsonObject Languages::readLanguage(const QString &code) const {
    if (!QRegularExpression("^[a-z]{2,3}(?:_[A-Za-z0-9]{2,8})*$").match(code).hasMatch()) return {};
    QFile file(directory + '/' + code + ".json");
    if (file.size() > 2 * 1024 * 1024 || !file.open(QIODevice::ReadOnly)) return {};
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject() || doc["language"].toString() != code || doc["name"].toString().trimmed().isEmpty() || !doc["messages"].isObject()) return {};
    return doc.object();
}
Languages::Languages(const QString &r, const QString &dir, QObject *parent) : QObject(parent), root(r), directory(dir.isEmpty() ? r + "/translations" : dir) {
    choices << QVariantMap{{"code", "en"}, {"name", "English"}};
    for (const QString &file : QDir(directory).entryList({"*.json"}, QDir::Files, QDir::Name)) {
        const auto code = file.chopped(5);
        const auto doc = readLanguage(code);
        if (code != "en" && !doc.isEmpty()) choices << QVariantMap{{"code", code}, {"name", doc["name"].toString()}};
    }
    QFile settings(root + "/config/interface.json");
    QString requested = "en";
    if (settings.open(QIODevice::ReadOnly)) requested = QJsonDocument::fromJson(settings.readAll())["language"].toString("en");
    const auto doc = readLanguage(requested);
    apply(requested == "en" || !doc.isEmpty() ? requested : "en", doc);
}
Languages::~Languages() {
    QCoreApplication::removeTranslator(&dictionary);
    QCoreApplication::removeTranslator(&qt);
}
void Languages::apply(const QString &code, const QJsonObject &document) {
    QCoreApplication::removeTranslator(&dictionary);
    QCoreApplication::removeTranslator(&qt);
    QJsonObject valid;
    if (code != "en") {
        const auto messages = document["messages"].toObject();
        for (auto i = messages.begin(); i != messages.end(); ++i) {
            // Una traducción incompleta o con marcadores incorrectos vuelve al inglés.
            if (i.value().isString() && !i.value().toString().trimmed().isEmpty() && placeholders(i.key()) == placeholders(i.value().toString())) valid[i.key()] = i.value();
        }
    }
    dictionary.setMessages(valid);
    selected = code;
    const QLocale locale(code);
    QLocale::setDefault(locale);
    if (code != "en" && qt.load(locale, "qt", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath))) QCoreApplication::installTranslator(&qt);
    QCoreApplication::installTranslator(&dictionary);
}
QString Languages::setLanguage(const QString &code) {
    if (code == selected) return {};
    if (std::none_of(choices.begin(), choices.end(), [&](const QVariant &v) { return v.toMap()["code"] == code; })) return uiText("This interface language is not available.");
    const auto doc = readLanguage(code);
    if (code != "en" && doc.isEmpty()) return uiText("The translation file could not be read.");
    const QString path = root + "/config/interface.json";
    QJsonObject settings;
    QFile previous(path);
    if (previous.exists()) {
        if (!previous.open(QIODevice::ReadOnly)) return uiText("The interface settings could not be read.");
        const auto saved = QJsonDocument::fromJson(previous.readAll());
        if (!saved.isObject()) return uiText("The interface settings file is invalid.");
        settings = saved.object();
    }
    settings["language"] = code;
    const auto bytes = QJsonDocument(settings).toJson();
    QSaveFile file(path);
    if (!QDir().mkpath(root + "/config") || !file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) return uiText("The interface language could not be saved.");
    apply(code, doc);
    emit changed();
    return {};
}
