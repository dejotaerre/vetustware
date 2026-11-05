#pragma once
#include <QFileInfo>
#include <QString>
#include <QMap>

// Se mantienen las bibliotecas anteriores sin exigir que se renombren.
namespace LibraryLayout {
inline bool international(const QString &root) { return !root.isEmpty() && QFileInfo::exists(root+"/library.json"); }
inline bool organized(const QString &root) { return international(root)||(!root.isEmpty() && QFileInfo::exists(root+"/biblioteca.json")); }
inline QString path(const QString &root,const QString &current,const QString &legacy) {
    if(current.startsWith("metadata/")){
        const auto nested=root+"/games/"+current.section('/',1,1)+"/metadata";
        if(QFileInfo(nested).isDir())return nested+(current.count('/')>1?'/'+current.section('/',2):QString());
    }
    if(current.startsWith("resources/")){
        const auto part=current.section('/',1,1);
        const auto nested=root+"/games/"+(part=="mame"?"mame":"dosbox")+"/resources";
        if(QFileInfo(nested).isDir())return nested+'/'+current.section('/',part=="mame"?2:1);
    }
    if(international(root))return root+'/'+current;
    if(!organized(root))return root+'/'+legacy;
    const QMap<QString,QString> folders{{"games","juegos"},{"metadata","metadatos"},{"resources","recursos"},{"magazines","revistas"},{"archives","archivos"},{"extras","complementos"},{"editions","ediciones"}};
    auto parts=current.split('/');
    for(auto &part:parts)part=folders.value(part,part);
    return root+'/'+parts.join('/');
}
inline QString games(const QString &root,const QString &engine) { return path(root,"games/"+engine,engine); }
inline QString metadata(const QString &root,const QString &engine,const QString &gameRoot) {
    if(!gameRoot.isEmpty()&&QFileInfo(gameRoot+"/metadata").isDir())return gameRoot+"/metadata";
    return organized(root)?path(root,"metadata/"+engine,QString()):gameRoot;
}
}
