#include "backend.h"
#include <QDate>
#include <QJsonArray>
#include <QFileInfo>
#include <QCryptographicHash>
#include <algorithm>

void Backend::refreshHome(){
    if(homeDay!=QDate::currentDate().toString(Qt::ISODate)){rebuildHome();refresh();}
}

void Backend::rebuildHome(){
    homeDay=QDate::currentDate().toString(Qt::ISODate);
    homeRowData.clear();collections.clear();spotlightData.clear();
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
    add("recent","Jugados recientemente","Vuelve a los juegos que ya abriste",recentIds);

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
        reason=(favorites.value(reference["id"].toString())?"Porque te gusta ":"Más como ")+reference["title"].toString();
        add("recommendations",reason,"Afinidad por géneros y estudio · A partir de tu actividad local",recommended);
    }else{
        recommended=daily.mid(0,36);reason="Para empezar a explorar";
        add("recommendations",reason,"Selección diaria · Marca favoritos para recibir sugerencias afines",recommended);
    }
    if(!recommended.isEmpty()){
        spotlightData=publicGame(games[recommended.first()]);spotlightData["reason"]=reason;
    }else if(!daily.isEmpty()){
        spotlightData=publicGame(games[daily.first()]);spotlightData["reason"]="Una nueva mirada a tu colección";
    }
    add("spanish","En español","Ediciones y subtítulos disponibles en tu colección",pick([&](const QJsonObject &g){
        const QString id=g["id"].toString();
        const auto profile=spanishProfiles.contains(id)?spanishProfiles[id].toObject():profiles[id].toObject();
        return (spanishProfiles.contains(id)||profile["language"].toString()=="es")&&QFileInfo::exists(storage.expand(profile["editionDirectory"].toString())+"/.ready");
    }));
    add("favorites","Tus favoritos","Tu selección personal",favoriteIds);
    auto genreRow=[&](const QString &key,const QString &title,const QString &subtitle,const QStringList &genres,bool graphical=false){
        add(key,title,subtitle,pick([&](const QJsonObject &g){
            const auto tags=g["genre"].toString().split(';');
            if(graphical&&(tags.contains("Interactive Fiction")||tags.contains("Text-Based")))return false;
            for(const auto &tag:genres)if(tags.contains(tag))return true;
            return false;
        }));
    };
    genreRow("adventure","Aventuras gráficas","Historias, exploración y enigmas",{"Adventure"},true);
    genreRow("strategy","Estrategia y construcción","Ciudades, imperios y decisiones",{"Strategy","Construction and Management Simulation"});
    genreRow("action","Acción y disparos","Reflejos, desafíos y mundos por recorrer",{"Action","Shooter","First Person Shooter"});
    genreRow("racing","Carreras","Velocidad en todas sus formas",{"Racing / Driving"});
    genreRow("rpg","Rol","Personajes, mazmorras y grandes aventuras",{"Role-Playing"});
    genreRow("puzzle","Puzles","Una buena idea vale más que mil reflejos",{"Puzzle"});
    genreRow("platform","Plataformas","Saltos que siguen en la memoria",{"Platform"});
    genreRow("sports","Deportes","Del estadio a tu teclado",{"Sports"});
    auto studioRow=[&](const QString &key,const QString &title,const QStringList &names){
        add(key,title,"Explora por estudio",pick([&](const QJsonObject &g){
            for(const auto &name:names)if(g["developer"].toString().contains(name,Qt::CaseInsensitive))return true;
            return false;
        }));
    };
    studioRow("lucasarts","El universo de LucasArts",{"LucasArts","Lucasfilm"});
    studioRow("sierra","Historias de Sierra",{"Sierra"});
    studioRow("idsoftware","id Software",{"id Software"});
    auto discovery=pick([&](const QJsonObject &g){const auto id=g["id"].toString();return !favorites.value(id)&&recent.value(id)<=0;});
    for(int i:recommended)discovery.removeAll(i);
    add("discover","Descubre algo distinto","Otra selección cada día · Más allá de los títulos habituales",discovery.mid(0,36));
    emit homeChanged();
}
