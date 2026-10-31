#include "displays.h"
#include <SDL.h>

QVariantList Displays::available(){
    QVariantList result;
    // La consulta y el emulador usan X11 para compartir numeración y coordenadas.
    if(SDL_VideoInit("x11")!=0)return result;
    for(int i=0;i<SDL_GetNumVideoDisplays();++i){
        SDL_Rect bounds;
        if(SDL_GetDisplayBounds(i,&bounds)!=0)continue;
        const QString name=QString::fromUtf8(SDL_GetDisplayName(i));
        result<<QVariantMap{{"id",name},{"label",QString("%1 · %2 × %3").arg(name).arg(bounds.w).arg(bounds.h)},
            {"index",i},{"x",bounds.x},{"y",bounds.y},{"width",bounds.w},{"height",bounds.h}};
    }
    SDL_VideoQuit();
    return result;
}
QVariantMap Displays::resolve(const QString &name,const QVariantList &screens){
    if(name.isEmpty())return {};
    for(const auto &value:screens)if(value.toMap()["id"].toString()==name)return value.toMap();
    // Si se desconecta la pantalla elegida, se usa la primera disponible sin borrar la preferencia.
    return screens.isEmpty()?QVariantMap{}:screens.first().toMap();
}
QJsonObject Displays::options(const QString &engine,const QVariantMap &screen){
    if(screen.isEmpty())return {};
    const int index=screen["index"].toInt();
    if(engine=="staging")return {{"sdl.display",QString::number(index)},{"sdl.window_position","auto"}};
    if(engine=="dosbox-x")return {{"sdl.display",QString::number(index+1)}};
    return {};
}
void Displays::environment(QProcessEnvironment &env,const QVariantMap &screen){
    if(screen.isEmpty())return;
    env.insert("SDL_VIDEODRIVER","x11");
    // SDL 1.2 y sdl12-compat, utilizados por DOSBox clásico.
    const QString index=QString::number(screen["index"].toInt());
    env.insert("SDL_VIDEO_FULLSCREEN_DISPLAY",index);
    env.insert("SDL_VIDEO_FULLSCREEN_HEAD",index);
    env.insert("SDL_VIDEO_WINDOW_POS",QString("%1,%2").arg(screen["x"].toInt()+40).arg(screen["y"].toInt()+40));
    env.remove("SDL_VIDEO_CENTERED");
}
