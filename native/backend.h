#pragma once
#include <QObject>
#include <QVariantList>
#include <QJsonObject>
#include <QProcess>
#include <QSqlDatabase>
#include <QQuickImageProvider>
#include <QMutex>
#include <QUrl>
#include <memory>
#include "archive.h"
#include "emulation.h"
#include "storage.h"
#include "extras.h"
#include "magazines.h"
#include "scummvm.h"
#include "atari.h"
#include "arcade.h"
#include "runtimes.h"

struct LaunchPlan { QString directory, config, error; };
class ArtProvider : public QQuickImageProvider {
public:
    ArtProvider(QString root,QJsonObject assets);
    void setAssets(QJsonObject assets);
    QImage requestImage(const QString &id,QSize *size,const QSize &requested) override;
private:
    QString root;QJsonObject assets;QString zipPath;QMutex mutex;std::unique_ptr<Archive> zip;
};
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(ScummLibrary* scumm READ scumm CONSTANT)
    Q_PROPERTY(AtariLibrary* atari READ atari CONSTANT)
    Q_PROPERTY(ArcadeLibrary* arcade READ arcade CONSTANT)
    Q_PROPERTY(Runtimes* runtimes READ runtimes CONSTANT)
    Q_PROPERTY(QVariantMap storageSettings READ storageSettings NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList monitors READ monitors NOTIFY monitorsChanged)
    Q_PROPERTY(QVariantList pageGames READ pageGames NOTIFY changed)
    Q_PROPERTY(QVariantList homeRows READ homeRows NOTIFY homeChanged)
    Q_PROPERTY(QVariantMap sectionBackgrounds READ sectionBackgrounds NOTIFY sectionBackgroundsChanged)
    Q_PROPERTY(QVariantList homeSpotlights READ homeSpotlights NOTIFY spotlightChanged)
    Q_PROPERTY(QVariantMap homeSpotlight READ homeSpotlight NOTIFY spotlightChanged)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList gameExtras READ gameExtras NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList magazineSeries READ magazineSeries NOTIFY storageChanged)
    Q_PROPERTY(QString magazineError READ magazineError NOTIFY settingsChanged)
    Q_PROPERTY(int catalogCount READ catalogCount NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(int page READ page NOTIFY changed)
    Q_PROPERTY(int pages READ pages NOTIFY changed)
    Q_PROPERTY(QStringList genres READ genres CONSTANT)
    Q_PROPERTY(bool nortonAvailable READ nortonAvailable NOTIFY storageChanged)
    Q_PROPERTY(bool xTreeAvailable READ xTreeAvailable NOTIFY storageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int progress READ progress NOTIFY statusChanged)
public:
    explicit Backend(QString root,QObject *parent=nullptr);
    QJsonObject assets() const{return assetMap;}
    QVariantMap storageSettings() const;
    Q_INVOKABLE QString saveStorage(const QUrl &library,const QUrl &games,const QUrl &scummvm=QUrl(),const QVariantMap &locations={});
    Q_INVOKABLE QVariantList inspectLibraries(const QString &library,const QVariantMap &locations) const;
    QVariantList monitors() const{return monitorData;}
    Q_INVOKABLE void refreshMonitors();
    QVariantList pageGames() const;
    Q_INVOKABLE QVariantMap unifiedCatalog(const QString &query,const QString &engine,int page,const QString &genre=QString(),bool verified=false) const;
    QVariantList homeRows() const{return homeRowData;}
    QVariantMap sectionBackgrounds() const{return backgrounds;}
    Q_INVOKABLE QString setSectionBackground(const QString &section,const QUrl &image);
    QVariantMap homeSpotlight() const{return spotlightData.value(0).toMap();}
    QVariantList homeSpotlights() const{return spotlightData;}
    Q_INVOKABLE void refreshHome(bool visit=false,int count=0);
    void retranslate();
    static constexpr int PageSize=28;
    QVariantMap selected() const;
    int catalogCount() const{return games.size();}
    int count() const{return filtered.size();}
    int page() const{return currentPage;}
    int pages() const{return qMax(1,(int(filtered.size())+PageSize-1)/PageSize);}
    QStringList genres() const{return genreList;}
    bool busy() const{return active || scummLibrary->busy() || atariLibrary->busy() || runtimeManager->busy() || arcadeLibrary->busy();}
    Runtimes* runtimes() const{return runtimeManager;}
    ScummLibrary *scumm() const{return scummLibrary;}
    AtariLibrary *atari() const{return atariLibrary;}
    ArcadeLibrary *arcade() const{return arcadeLibrary;}
    Q_INVOKABLE QVariantMap scummArtwork(const QString &gameId) const;
    QString status() const{return message;}
    int progress() const{return percent;}
    Q_INVOKABLE void filter(const QString &query,const QString &genre,const QString &view,bool testedOnly,const QString &collection=QString());
    Q_INVOKABLE void selectGame(const QString &id);
    Q_INVOKABLE void pageBy(int direction);
    Q_INVOKABLE void moveSelection(int direction);
    Q_INVOKABLE void toggleFavorite();
    Q_INVOKABLE void play();
    Q_INVOKABLE void openGameFolder();
    Q_INVOKABLE void openConsole(const QString &fileManager=QString());
    bool xTreeAvailable() const;
    bool nortonAvailable() const;
    Q_INVOKABLE void setSpanish(bool enabled);
    Q_INVOKABLE void openManual();
    QVariantList gameExtras() const{return extras.list(selectedId);}
    Q_INVOKABLE void openExtra(int index);
    QVariantList magazineSeries() const{return magazines.series();}
    QString magazineError() const{return magazines.error();}
    Q_INVOKABLE QVariantList magazineIssues(const QString &series) const{return magazines.issues(series);}
    Q_INVOKABLE void openMagazine(const QString &id);
    Q_INVOKABLE void openLog();
    Q_INVOKABLE void dismissStatus();
    Q_INVOKABLE QVariantMap emulatorSettings(bool global) const;
    Q_INVOKABLE QVariantList emulatorFields(const QString &engine) const;
    Q_INVOKABLE QString saveEmulatorSettings(bool global,const QString &id,const QVariantMap &scope,const QVariantMap &paths);
    Q_INVOKABLE QVariantMap inheritedSettings(bool global,const QString &engine) const;
    LaunchPlan prepare(const QJsonObject &game,const QJsonObject &profile,const QString &engine,const QJsonObject &options,const std::function<void(int)> &callback,bool console=false,const QString &fileManager=QString());
    Q_INVOKABLE QString diagnostics() const;
signals:
    void sectionBackgroundsChanged();
    void storageChanged();
    void settingsChanged();
    void monitorsChanged();
    void changed();void homeChanged();void spotlightChanged();void selectionChanged();void statusChanged();void manualReady(QUrl url);
    void extraReady(QUrl url,QString name,bool pdf);
private:
    ScummLibrary *scummLibrary;
    AtariLibrary *atariLibrary;
    ArcadeLibrary *arcadeLibrary;
    Runtimes *runtimeManager;
    QVariantList monitorData;
    Extras extras;
    Magazines magazines;
    QVariantList homeRowData,spotlightData;
    int spotlightCount=1;
    QHash<QString,QList<int>> collections;QString collectionKey,homeDay;
    QList<int> spotlightCandidates;
    bool spotlightClassics=false;
    QVariantMap backgrounds;
    quint64 backgroundRevision=0;
    QString spotlightRecommendationReason;
    QStringList spotlightHistory;
    bool homeVisited=false;
    void updateSpotlight(bool rotate);
    void rebuildHome();
    void launch(bool console,const QString &fileManager=QString());
    void loadCatalog();
    void restorePrepared(const QJsonObject &profile,const QString &readyPath,const std::function<void(int)> &callback);
    void restoreTools();
    QString root;Storage storage;QString library,gamesRoot;
    QString query,genre,view="all",selectedId,message;
    bool active=false,testedOnly=false;int currentPage=0,percent=0;
    QList<QJsonObject> games;QHash<QString,int> indexes;QHash<QString,QString> searches;
    QJsonObject assetMap,profiles,spanishProfiles;QHash<QString,bool> spanishPreference;QHash<QString,bool> favorites;QHash<QString,double> recent;
    QList<int> filtered;QStringList genreList;
    EmulationSettings emulation;
    QSqlDatabase database;QProcess process;
    QVariantMap publicGame(const QJsonObject &g) const;
    void refresh();void notify(const QString &text,int progress=-1);
    QString makeConfig(const QJsonObject &game,const QJsonObject &profile,const QString &folder,const QString &environment,const QString &engine,const QJsonObject &options,bool console,const QString &fileManager);
};
