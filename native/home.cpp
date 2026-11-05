#include "i18n.h"
#include "backend.h"
#include <QDate>
#include <QJsonArray>
#include <QFileInfo>
#include <QCryptographicHash>
#include <algorithm>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QSaveFile>

namespace {
constexpr int SpotlightHistoryLimit = 12;
}

void Backend::refreshHome(bool visit){
    if(homeDay!=QDate::currentDate().toString(Qt::ISODate)){rebuildHome();refresh();}
    if(!visit)return;
    if(!homeVisited){
        QFile history(root+"/data/spotlight-history.json");
        if(history.open(QIODevice::ReadOnly)){
            const auto ids=QJsonDocument::fromJson(history.readAll()).array();
            for(const auto &id:ids){
                if(id.isString())spotlightHistory.append(id.toString());
                if(spotlightHistory.size()==SpotlightHistoryLimit)break;
            }
        }
        homeVisited=true;
    }
    updateSpotlight(true);
}

void Backend::updateSpotlight(bool rotate){
    if(spotlightCandidates.isEmpty()){
        spotlightData.clear();
        emit spotlightChanged();
        return;
    }
    const auto &related=collections.value("recommendations");
    int selected=indexes.value(spotlightData.value("id").toString(),-1);
    if(rotate || selected<0){
        // Primero se consideran los títulos ausentes del historial. Si todos se
        // mostraron, se elige el que lleva más tiempo sin aparecer.
        QList<int> suggestions, discoveries;
        int oldest=-1;
        for(int index:spotlightCandidates){
            const int position=spotlightHistory.indexOf(games[index]["id"].toString());
            const int age=position<0 ? SpotlightHistoryLimit : position;
            if(age<oldest)continue;
            if(age>oldest){
                oldest=age;
                suggestions.clear();
                discoveries.clear();
            }
            if(related.contains(index))suggestions.append(index);
            else discoveries.append(index);
        }
        auto *random=QRandomGenerator::global();
        const bool useSuggestions=discoveries.isEmpty() || (!suggestions.isEmpty() && random->bounded(3)!=0);
        const auto &pool=useSuggestions ? suggestions : discoveries;
        QList<int> illustrated;
        for(int index:pool){
            const auto &game=games[index];
            if(!game["cover"].toString().isEmpty() || !game["shots"].toArray().isEmpty())
                illustrated.append(index);
        }
        const auto &choices=illustrated.isEmpty() ? pool : illustrated;
        selected=choices[random->bounded(int(choices.size()))];
        const auto id=games[selected]["id"].toString();
        spotlightHistory.removeAll(id);
        spotlightHistory.prepend(id);
        if(spotlightHistory.size()>SpotlightHistoryLimit)spotlightHistory.removeLast();
        QSaveFile history(root+"/data/spotlight-history.json");
        const auto bytes=QJsonDocument(QJsonArray::fromStringList(spotlightHistory)).toJson(QJsonDocument::Compact);
        if(!history.open(QIODevice::WriteOnly) || history.write(bytes)!=bytes.size() || !history.commit())
            qWarning()<<"Could not save spotlight history:"<<history.errorString();
    }
    spotlightData=publicGame(games[selected]);
    spotlightData["reason"]=related.contains(selected) ? spotlightRecommendationReason : uiText("A fresh look at your collection");
    emit spotlightChanged();
}

void Backend::rebuildHome(){
    homeDay=QDate::currentDate().toString(Qt::ISODate);
    homeRowData.clear();collections.clear();
    QList<int> daily;
    QHash<int,QByteArray> order;
    for(int i=0;i<games.size();++i){
        daily<<i;
        order[i]=QCryptographicHash::hash((homeDay+games[i]["id"].toString()).toUtf8(),QCryptographicHash::Sha256);
    }
    // El orden se mantiene durante el día y no salta al escribir ni al volver a Inicio.
    std::sort(daily.begin(),daily.end(),[&](int a,int b){
        const bool ca=!games[a]["cover"].toString().isEmpty(),cb=!games[b]["cover"].toString().isEmpty();
        if(ca!=cb)return ca;
        return order[a]<order[b];
    });
    auto pick=[&](auto predicate){QList<int> result;for(int i:daily)if(predicate(games[i]))result<<i;return result;};
    auto add=[&](const QString &key,const QString &title,const QString &subtitle,const QList<int> &ids){
        collections[key]=ids;
        if(ids.isEmpty())return;
        QVariantList cards;for(int i:ids.mid(0,24))cards<<publicGame(games[i]);
        homeRowData<<QVariantMap{{"key",key},{"title",title},{"subtitle",subtitle},{"count",ids.size()},{"games",cards}};
    };
    const auto favoriteIds=pick([&](const QJsonObject &g){return favorites.value(g["id"].toString());});
    auto recentIds=pick([&](const QJsonObject &g){return recent.value(g["id"].toString())>0;});
    std::sort(recentIds.begin(),recentIds.end(),[&](int a,int b){
        return recent.value(games[a]["id"].toString())>recent.value(games[b]["id"].toString());
    });
    add("recent",uiText("Recently played"),uiText("Return to games you have already opened"),recentIds);

    int seed=-1;
    if(!favoriteIds.isEmpty()){
        auto seeds=favoriteIds;
        std::sort(seeds.begin(),seeds.end(),[&](int a,int b){
            const double ra=recent.value(games[a]["id"].toString()),rb=recent.value(games[b]["id"].toString());
            if(ra!=rb)return ra>rb;
            return games[a]["title"].toString()<games[b]["title"].toString();
        });
        seed=seeds.first();
    }else if(!recentIds.isEmpty())seed=recentIds.first();
    QList<int> recommended;
    QString reason;
    if(seed>=0){
        const auto reference=games[seed];const QString studio=reference["developer"].toString();
        const auto genres=reference["genre"].toString().split(';',Qt::SkipEmptyParts);
        QHash<int,int> scores;
        for(int i:daily){
            const auto &g=games[i];const QString id=g["id"].toString();
            if(i==seed||favorites.value(id)||recent.value(id)>0)continue;
            int score=(!studio.isEmpty()&&g["developer"].toString()==studio)?6:0;
            const auto candidateGenres=g["genre"].toString().split(';',Qt::SkipEmptyParts);
            for(const auto &genre:genres)if(candidateGenres.contains(genre))score+=2;
            if(score>0){scores[i]=score;recommended<<i;}
        }
        std::stable_sort(recommended.begin(),recommended.end(),[&](int a,int b){return scores[a]>scores[b];});
        recommended=recommended.mid(0,60);
        reason=(favorites.value(reference["id"].toString())?uiText("Because you like "):uiText("More like "))+reference["title"].toString();
        add("recommendations",reason,uiText("Related genres and studios · Based on your local activity"),recommended);
    }else{
        recommended=daily.mid(0,36);reason=uiText("Start exploring");
        add("recommendations",reason,uiText("Daily selection · Mark favorites for more relevant suggestions"),recommended);
    }
    spotlightRecommendationReason=reason;
    add("spanish",uiText("In Spanish"),uiText("Editions and subtitles available in your collection"),pick([&](const QJsonObject &g){
        const QString id=g["id"].toString();
        const auto profile=spanishProfiles.contains(id)?spanishProfiles[id].toObject():profiles[id].toObject();
        return (spanishProfiles.contains(id)||profile["language"].toString()=="es")&&QFileInfo::exists(storage.expand(profile["editionDirectory"].toString())+"/.ready");
    }));
    add("favorites",uiText("Your favorites"),uiText("Your personal selection"),favoriteIds);
    auto genreRow=[&](const QString &key,const QString &title,const QString &subtitle,const QStringList &genres,bool graphical=false){
        add(key,title,subtitle,pick([&](const QJsonObject &g){
            const auto tags=g["genre"].toString().split(';');
            if(graphical&&(tags.contains("Interactive Fiction")||tags.contains("Text-Based")))return false;
            for(const auto &tag:genres)if(tags.contains(tag))return true;
            return false;
        }));
    };
    genreRow("adventure",uiText("Graphic adventures"),uiText("Stories, exploration and puzzles"),{"Adventure"},true);
    genreRow("strategy",uiText("Strategy and building"),uiText("Cities, empires and decisions"),{"Strategy","Construction and Management Simulation"});
    genreRow("action",uiText("Action and shooters"),uiText("Reflexes, challenges and worlds to explore"),{"Action","Shooter","First Person Shooter"});
    genreRow("racing",uiText("Racing"),uiText("Speed in all its forms"),{"Racing / Driving"});
    genreRow("rpg",uiText("Role-playing"),uiText("Characters, dungeons and great adventures"),{"Role-Playing"});
    genreRow("puzzle",uiText("Puzzles"),uiText("A good idea beats quick reflexes"),{"Puzzle"});
    genreRow("platform",uiText("Platformers"),uiText("Jumps you still remember"),{"Platform"});
    genreRow("sports",uiText("Sports"),uiText("From the stadium to your keyboard"),{"Sports"});
    auto studioRow=[&](const QString &key,const QString &title,const QStringList &names){
        add(key,title,uiText("Explore by studio"),pick([&](const QJsonObject &g){
            for(const auto &name:names)if(g["developer"].toString().contains(name,Qt::CaseInsensitive))return true;
            return false;
        }));
    };
    studioRow("lucasarts",uiText("The LucasArts universe"),{"LucasArts","Lucasfilm"});
    studioRow("sierra",uiText("Sierra stories"),{"Sierra"});
    studioRow("idsoftware","id Software",{"id Software"});
    auto discovery=pick([&](const QJsonObject &g){const auto id=g["id"].toString();return !favorites.value(id)&&recent.value(id)<=0;});
    spotlightCandidates=discovery.isEmpty() ? daily : discovery;
    for(int i:recommended)discovery.removeAll(i);
    add("discover",uiText("Discover something different"),uiText("A different selection every day · Beyond the familiar titles"),discovery.mid(0,36));
    if(homeVisited)updateSpotlight(false);
    emit homeChanged();
}
