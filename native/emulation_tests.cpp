#include "emulation.h"
#include "displays.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTextStream>
#include <stdexcept>
static void check(bool yes,const char *text){if(!yes)throw std::runtime_error(text);}
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);
    try{
        QTemporaryDir dir;QDir().mkpath(dir.path()+"/data");
        EmulationSettings settings(dir.path());
        check(settings.engine("a")=="staging","motor predeterminado");
        settings.data={{"global",QJsonObject{{"engine","classic"},{"options",QJsonObject{{"classic",QJsonObject{{"cpu.cycles","5000"},{"dosbox.memsize","16"}}},{"staging",QJsonObject{{"cpu.cycles","max"}}}}}}},{"games",QJsonObject{{"a",QJsonObject{{"options",QJsonObject{{"classic",QJsonObject{{"cpu.cycles","9000"}}}}}}},{"b",QJsonObject{{"engine","staging"}}}}}};
        check(settings.engine("a")=="classic"&&settings.engine("b")=="staging","herencia del motor");
        check(settings.overrides("a","classic")["cpu.cycles"]=="9000","juego prevalece");
        check(settings.overrides("a","classic")["dosbox.memsize"]=="16","global heredado");
        check(settings.overrides("b","classic")["cpu.cycles"]=="5000","aislamiento entre juegos");
        check(settings.overrides("a","staging")["cpu.cycles"]=="max","aislamiento entre motores");
        QString config="[cpu]\ncycles=3000\n[sdl]\nfullscreen=false\n[autoexec]\nset cycles=123\necho a=b\nexit\n";
        auto merged=EmulationSettings::apply(config,{{"cpu.cycles","9000"},{"dosbox.memsize","32"},{"sdl.fullscreen","true"}});
        check(merged.contains("[autoexec]\nset cycles=123\necho a=b\nexit\n"),"preservar arranque");
        check(EmulationSettings::values(merged)["cpu.cycles"]=="9000","aplicar ciclos");
        check(EmulationSettings::values(merged)["dosbox.memsize"]=="32","insertar sección");
        auto modern=EmulationSettings::values(EmulationSettings::configured(config,{{"cpu.cycles","4321"}},"staging"));
        check(!modern.contains("cpu.cycles")&&modern["cpu.cpu_cycles"]=="4321"&&modern["cpu.cpu_cycles_protected"]=="auto","ciclos modernos Staging");
        auto automatic=EmulationSettings::values(EmulationSettings::configured(config,{{"cpu.cycles","auto"}},"staging"));
        check(automatic["cpu.cpu_cycles"]=="3000"&&automatic["cpu.cpu_cycles_protected"]=="max","ciclos automáticos Staging");
        const QString oldVideo="[render]\nglshader=crt/vga-1080p\nshader=bilinear\naspect=false\nviewport=800x600\ninteger_scaling=vertical\n"+config;
        const auto crtConfig=EmulationSettings::configured(oldVideo,{{"render.shader","crt-auto"}},"staging");
        const auto crt=EmulationSettings::values(crtConfig);
        check(crt["render.shader"]=="crt-auto"&&crt["render.aspect"]=="auto"&&crt["render.integer_scaling"]=="auto"&&crt["render.viewport"]=="fit","CRT opcional y proporciones conservadas");
        check(!crt.contains("render.glshader")&&crtConfig.contains("set cycles=123\necho a=b\nexit"),"eliminar alias antiguo sin tocar AUTOEXEC");
        const auto sharp=EmulationSettings::values(EmulationSettings::configured(oldVideo,{},"staging"));
        check(sharp["render.shader"]=="sharp"&&sharp["render.integer_scaling"]=="off"&&sharp["render.aspect"]=="auto"&&sharp["render.viewport"]=="fit","nítido predeterminado permite ampliar sin deformar");
        check(EmulationSettings::configured(oldVideo,{},"classic")==oldVideo&&EmulationSettings::configured(oldVideo,{},"dosbox-x")==oldVideo,"no aplicar shaders de Staging a otros motores");
        EmulationSettings videoSettings(dir.path()+"/video");
        const QJsonObject videoGlobal{{"engine","staging"},{"options",QJsonObject{{"staging",QJsonObject{{"render.shader","sharp"}}}}}};
        const QJsonObject videoGame{{"engine",""},{"options",QJsonObject{{"staging",QJsonObject{{"render.shader","crt-auto"}}}}}};
        check(videoSettings.save(true,"",videoGlobal,{}).isEmpty()&&videoSettings.save(false,"a",videoGame,{}).isEmpty(),"guardar estilos global e individual");
        EmulationSettings savedVideo(dir.path()+"/video");
        check(savedVideo.overrides("a","staging")["render.shader"]=="crt-auto"&&savedVideo.overrides("b","staging")["render.shader"]=="sharp","persistencia y prioridad del estilo individual");
        auto invalidVideo=videoGlobal;invalidVideo["options"]=QJsonObject{{"staging",QJsonObject{{"render.shader","desconocido"}}}};
        check(!savedVideo.save(true,"",invalidVideo,{}).isEmpty(),"rechazar estilos desconocidos");
        check(EmulationSettings::apply(merged,{})==merged,"heredar no cambia perfil");
        check(EmulationSettings::arguments("classic","a b.conf")==QStringList{"-conf","a b.conf"},"argumentos separados");
        check(EmulationSettings::arguments("staging","x").contains("--noprimaryconf"),"aislar configuración Staging");
        const QString boot="[sdl]\nfullscreen=true\n[autoexec]\n@mount C \"/juegos/uno\"\nimgmount d \"/juegos/CD uno.iso\" -t iso\nd:\ncd \\DIG\ncall START.BAT\nGAME.EXE\nexit\n[mt32]\nromdir=/roms\n";
        const auto shell=EmulationSettings::consoleConfig(boot,"/fallback");
        check(shell.contains("@mount C \"/juegos/uno\"")&&shell.contains("imgmount d \"/juegos/CD uno.iso\" -t iso"),"conservar disco y CD");
        check(shell.contains("c:\ncd \\\n")&&!shell.contains("fallback"),"entrar en la raíz C existente");
        check(!shell.contains("START.BAT")&&!shell.contains("GAME.EXE")&&!shell.contains("\\DIG")&&!shell.contains("\nexit\n"),"no ejecutar juego ni cerrar consola");
        check(shell.contains("[mt32]\nromdir=/roms")&&EmulationSettings::values(shell)["sdl.fullscreen"]=="true","mantener hardware y secciones posteriores");
        check(shell.contains("\nmount\n")&&shell.contains("ejemplo D:"),"mostrar unidades reales y explicar el cambio de unidad");
        const auto tools=EmulationSettings::consoleConfig(boot,"/fallback","/tools","xtree");
        check(tools.contains("mount T \"/tools\"")&&tools.contains("set PATH=T:\\;%PATH%")&&tools.endsWith("T:\\XTGOLD.COM C: /R\n"),"XTree con unidad propia y arranque en C");
        const auto occupied=EmulationSettings::consoleConfig(boot+"\n[autoexec]\nmount t \"/otro disco\"\n","/fallback","/tools");
        check(occupied.contains("mount U \"/tools\"")&&occupied.contains("mount t \"/otro disco\"")&&!occupied.contains("XTGOLD.COM C:"),"conservar unidad T del juego y consola sin autoarranque");
        check(!shell.contains("XTREE"),"consola sin herramientas disponibles");
        const auto norton=EmulationSettings::consoleConfig(boot,"/fallback","/xtree","norton","/norton");
        check(norton.contains("mount T \"/xtree\"")&&norton.contains("mount U \"/norton\"")&&norton.endsWith("U:\\NC.EXE\n"),"convivencia de XTree y Norton sin solapar unidades");
        check(norton.contains("set NC=U:\\")&&norton.contains("set NORTON=U:\\")&&norton.contains("set PATH=U:\\;%PATH%"),"recursos y ajustes de Norton en su carpeta");
        const auto ncOnly=EmulationSettings::consoleConfig(boot,"/fallback",{},"norton","/norton");
        check(ncOnly.endsWith("T:\\NC.EXE\n")&&!ncOnly.contains("XTREE"),"Norton independiente de XTree");
        const auto bothConsole=EmulationSettings::consoleConfig(boot,"/fallback","/xtree",{},"/norton");
        check(!bothConsole.contains("\\NC.EXE\n")&&!bothConsole.contains("\\XTGOLD.COM C:"),"herramientas en consola sin ejecución automática");
        const auto diskette=EmulationSettings::consoleConfig("[autoexec]\nboot floppy.img\nexit\n","/juego disquete");
        check(diskette.contains("mount c \"/juego disquete\"")&&!diskette.contains("boot floppy.img"),"montar carpeta para juegos de arranque directo");
        settings.data["paths"]=QJsonObject{{"dosbox-x",dir.path()+"/ausente"}};
        check(settings.executable("dosbox-x").isEmpty(),"motor ausente en entorno de prueba");
        auto bad=settings.save(false,"a",{{"engine","classic"},{"options",QJsonObject{{"classic",QJsonObject{{"cpu.cycles","1\n[autoexec]\nexit"}}}}}},{});
        check(!bad.isEmpty()&&!QFile::exists(dir.path()+"/data/emulation.json"),"rechazar valores inválidos sin guardar");
        QFile f(dir.path()+"/data/emulation.json");check(f.open(QIODevice::WriteOnly),"archivo de prueba");f.write(QJsonDocument(settings.data).toJson());f.close();
        EmulationSettings reopened(dir.path());check(reopened.engine("b")=="staging"&&reopened.overrides("a","classic")["cpu.cycles"]=="9000","lectura persistente");
        const QVariantMap primary{{"id","DP-3"},{"index",0},{"x",0},{"y",0}},secondary{{"id","DP-2"},{"index",1},{"x",2560},{"y",0}};
        check(Displays::resolve("DP-2",{primary,secondary})==secondary,"seleccionar por identidad");
        auto moved=secondary;moved["index"]=0;
        check(Displays::resolve("DP-2",{moved})==moved,"identidad conservada al reordenar pantallas");
        check(Displays::resolve("DP-2",{primary})==primary,"pantalla desconectada usa una disponible");
        check(Displays::resolve("DP-2",{}).isEmpty()&&Displays::resolve("",{primary,secondary}).isEmpty(),"sin pantallas y modo automático");
        check(Displays::options("staging",secondary)["sdl.display"]=="1"&&Displays::options("dosbox-x",secondary)["sdl.display"]=="2","numeración diferente de motores");
        check(Displays::options("classic",secondary).isEmpty()&&Displays::options("staging",{}).isEmpty(),"no introducir opciones incompatibles");
        QProcessEnvironment env;env.insert("SDL_VIDEO_CENTERED","1");Displays::environment(env,secondary);
        check(env.value("SDL_VIDEODRIVER")=="x11"&&env.value("SDL_VIDEO_FULLSCREEN_DISPLAY")=="1"&&env.value("SDL_VIDEO_WINDOW_POS")=="2600,40"&&!env.contains("SDL_VIDEO_CENTERED"),"entorno clásico en pantalla secundaria");
        QProcessEnvironment unchanged;unchanged.insert("prueba","valor");auto original=unchanged;Displays::environment(unchanged,{});check(unchanged==original,"automático conserva entorno");
        auto scope=reopened.data["global"].toObject();scope["monitor"]="DP-2";
        check(reopened.save(true,"",scope,{}).isEmpty(),"guardar monitor global");
        EmulationSettings monitorSaved(dir.path());check(monitorSaved.data["global"].toObject()["monitor"]=="DP-2","persistencia del monitor");
        check(monitorSaved.overrides("a","classic")["cpu.cycles"]=="9000","monitor preserva ajustes por juego");
        scope["monitor"]=QJsonObject{};check(!monitorSaved.save(true,"",scope,{}).isEmpty(),"validar tipo de monitor");
        QTextStream(stdout)<<"Herencia, aislamiento, validación, configuración y persistencia: correctos\n";return 0;
    }catch(const std::exception &e){QTextStream(stderr)<<e.what()<<'\n';return 1;}
}
