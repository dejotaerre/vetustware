#pragma once
#include <QCoreApplication>
#include <QJsonObject>
#include <QReadWriteLock>
#include <QTranslator>
#include <QVariantList>

inline QString uiText(const char *text) { return QCoreApplication::translate("Vetust", text); }

class DictionaryTranslator : public QTranslator {
public:
    bool isEmpty() const override { return false; }
    QString translate(const char *context, const char *source, const char *disambiguation = nullptr, int n = -1) const override;
    void setMessages(const QJsonObject &messages);
private:
    mutable QReadWriteLock lock;
    QJsonObject messages;
};

class Languages : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString current READ current NOTIFY changed)
    Q_PROPERTY(QVariantList available READ available CONSTANT)
public:
    explicit Languages(const QString &root, const QString &directory = QString(), QObject *parent = nullptr);
    ~Languages() override;
    QString current() const { return selected; }
    QVariantList available() const { return choices; }
    Q_INVOKABLE QString setLanguage(const QString &code);
signals:
    void changed();
private:
    QString root, directory, selected = "en";
    QVariantList choices;
    DictionaryTranslator dictionary;
    QTranslator qt;
    QJsonObject readLanguage(const QString &code) const;
    void apply(const QString &code, const QJsonObject &document);
};
