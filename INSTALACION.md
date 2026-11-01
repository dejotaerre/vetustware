# VETUSTware — instalación de la versión de prueba 0.3.4

[English installation guide](INSTALLATION.md)

Interfaz nativa para una colección local de juegos DOS con estructura eXoDOS.
Licencia MIT: ver LICENSE. No requiere navegador ni servicios en línea.

## Qué contiene el paquete

Programa, icono, instalador, importador de biblioteca y código fuente completo.
No contiene juegos, ISOs, revistas, carátulas, manuales, partidas, favoritos, catálogo
personal, configuración del autor, emuladores, XTree ni Norton Commander.
El motor y la biblioteca se distribuyen por separado.

El archivo linux-x86_64 incluye un ejecutable compilado en CachyOS con Qt 6.12.
No es un binario universal para todas las distribuciones: necesita bibliotecas
compatibles. BUILD_INFO.json identifica las versiones de compilación.
El paquete de fuentes permite compilar en el equipo destinatario.

## Instalación del paquete con ejecutable

1. Instalar desde los repositorios de la distribución las dependencias:
   Qt 6 (Quick, QuickControls2, Sql con controlador SQLite, Concurrent,
   Pdf/PdfQuick y diálogos QML), libzip, SDL2, Python 3. Se necesita un escritorio
   Linux con X11 o XWayland y OpenGL. El emulador se instala aparte.
2. Extraer el archivo tar.gz en una carpeta temporal y abrir una terminal allí.
3. Comprobar las bibliotecas e instalar:

```sh
ldd ./vetustware
python3 helpers/install.py --desktop
```

Si ldd muestra «not found», instalar la biblioteca indicada. Si aparecen errores
sobre versiones de Qt, GLIBC o GLIBCXX, compilar desde las fuentes como se explica
abajo. No se necesita sudo para ejecutar el instalador de VETUSTware.

El destino predeterminado es `~/vetustware`, calculado desde la home del usuario
que instala. Para otro destino:

```sh
python3 helpers/install.py --dest "$HOME/Aplicaciones/vetustware" --desktop
```

No usar como destino la misma carpeta desde la que se ejecuta el instalador.
Abrir VETUSTware desde el menú de aplicaciones o con:

```sh
"$HOME/vetustware/vetustware"
```

## Emulador

Instalar DOSBox Staging, DOSBox clásico o DOSBox-X desde una fuente de confianza.
Al instalar VETUSTware se busca `dosbox-staging`, `dosbox` o `dosbox-x` en PATH y
se selecciona uno disponible, priorizando Staging. Revisar **Ajustes → Motor**.
Si el ejecutable del emulador tiene otro nombre, indicar su ruta en
**Opciones avanzadas → Ejecutable personalizado**.
Si todavía no hay ningún emulador, VETUSTware permite organizar la biblioteca y
avisa al intentar jugar. El programa no instala paquetes del sistema.

## Primer inicio: elegir las carpetas

El asistente pide la carpeta de la biblioteca, que puede estar en otro disco.
Admite la colección base eXoDOS o una biblioteca personalizada de VETUSTware.
La raíz debe conservar `Content/XODOSMetadata.zip`,
`Content/!DOSmetadata_linux.zip` y `eXo/eXoDOS`.

Una biblioteca personalizada incorpora `Personalizaciones/perfiles.zip`,
`Personalizaciones/juegos.zip` y, opcionalmente, `herramientas.zip`.
El motor reconoce su catálogo, ediciones e instrucciones automáticamente.
Al pulsar **Jugar** o abrir la consola, extrae únicamente la edición elegida.
XTree y Norton Commander se instalan desde los recursos de la biblioteca al
abrir una consola, si están incluidos. No forman parte del motor MIT.

Las copias jugables y sus partidas van por defecto a `data/games` dentro de la
instalación del motor. En el asistente se puede activar la opción para elegir
otra carpeta. No se presupone que exista `Juegos`, `Games` ni otro nombre.
La carpeta de la biblioteca permanece intacta durante el uso, por lo que puede
mantenerse compartida por torrent sin que las partidas alteren sus archivos.

Borrar una copia jugable permite recuperarla desde la biblioteca al abrirla de
nuevo, pero también borra las partidas o cambios posteriores que solo existan
en esa copia. Una carpeta presente pero incompleta nunca se sobrescribe
automáticamente: se conserva para que el usuario pueda revisar sus archivos.

Los perfiles importados no garantizan que todos los títulos funcionen con cada
motor DOSBox; se pueden ajustar individualmente. La interfaz inicia en inglés.
Para usar español, selecciona **Settings → Interface language → Español**.
La elección se aplica al instante y se conserva al volver a abrir la aplicación.
No cambia el idioma de los juegos, documentos ni revistas.

## Compilar desde las fuentes

Se requiere un compilador C++20, CMake >= 3.21, pkg-config y los paquetes de
desarrollo de Qt >= 6.5 (incluido Qt Test), libzip y SDL2.

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build -j 4
ctest --test-dir native/build --output-on-failure
python3 helpers/install.py --desktop
```

El instalador utiliza el binario recién compilado si existe en native/build.
No se necesitan juegos para ejecutar las pruebas incluidas.

## Datos, actualizaciones y cambio de disco

La aplicación mantiene en su propia carpeta:

- config/: ajustes, ubicaciones y configuraciones DOSBox.
- data/: catálogo importado, favoritos, historial, carátulas, manuales y las
  copias jugables con sus partidas (data/games, salvo destino personalizado).
- cache/, logs/ y backups/: archivos regenerables, registros y respaldos.
- helpers/, runtime/: herramientas propias y posibles emuladores añadidos localmente.
- translations/: traducciones de la interfaz en JSON; `config/interface.json` guarda el idioma elegido.

Los juegos y las partidas permanecen en data/games o en la carpeta alternativa
elegida por el usuario.
Para actualizar, cerrar VETUSTware y DOSBox y ejecutar el instalador del nuevo
paquete hacia el mismo destino. Se respaldan los archivos de programa anteriores;
los ajustes, catálogo y partidas se conservan. Los runtimes existentes no se
sobrescriben automáticamente. Las traducciones propias con otros nombres de
archivo y el idioma seleccionado también se conservan. Los catálogos incluidos
se actualizan, respaldando la copia anterior.

En **Ajustes → Biblioteca y carpetas** se pueden cambiar las ubicaciones.
Cambiar una ruta no mueve los archivos: copiar antes la carpeta de juegos completa,
incluidos los archivos ocultos y las partidas. Si falta un disco, reconectarlo o
localizarlo de nuevo. No se borra el catálogo por una desconexión.

Para mover la aplicación, cerrar y copiar su carpeta completa; actualizar el
acceso del escritorio para que apunte a la nueva ubicación. Las bibliotecas
compartidas necesarias deben seguir disponibles en el sistema.

## Compartir

Compartir el archivo de distribución limpio y su SHA256, no comprimir una
instalación ya utilizada: después de importar y jugar, config/ y data/ contienen
información del usuario. La distribución de las dependencias y de los contenidos
ajenos se rige por sus propias licencias; ver THIRD_PARTY.md.

## Diagnóstico

```sh
"$HOME/vetustware/vetustware" --diagnostics
```

El diagnóstico muestra rutas del usuario. Revisarlo antes de publicarlo.
Esta versión se ha probado en el sistema indicado en BUILD_INFO.json; las pruebas
en otras distribuciones siguen siendo necesarias antes de anunciar compatibilidad.

## Extras de la biblioteca

Desde 0.3.1, las fichas con material complementario muestran **Extras**. Puedes
buscar por nombre y abrir mapas, guías, imágenes, música y vídeos. Los PDF se
abren en VETUSTware; los demás formatos utilizan el visor predeterminado del
sistema. El material puede corresponder a la edición original y estar en otro
idioma. Los scripts y ejecutables históricos no aparecen como documentos.

El archivo seleccionado se extrae y verifica mediante SHA-256 en
`cache/extras` dentro de la instalación. Esa caché puede borrarse con la aplicación
cerrada: los documentos se recuperan de la biblioteca al volver a abrirlos.
No se modifican los juegos, sus perfiles ni las partidas.

Una biblioteca sin complementos sigue funcionando. La versión 0.3.0 puede usar
los juegos de una biblioteca ampliada, pero requiere actualizarse a 0.3.1 para
mostrar el botón Extras.


## Revistas (desde 0.3.2)

La sección **Revistas** reconoce `Revistas/revistas.json` dentro de la biblioteca.
Muestra las colecciones y sus portadas locales, con búsqueda y lectura PDF en el
visor integrado. El índice original de cada colección también está disponible.
No se incluyen revistas ni portadas de terceros en este paquete.

Big Blue Disk y Game Bytes se abren con sus menús originales mediante DOSBox.
La primera apertura prepara una copia local en `data/magazines` dentro de la
instalación; no escribe en la biblioteca compartida. Estas sesiones usan los
ajustes globales de emulador y monitor y no se agregan al historial de juegos.
Las copias locales pueden contener cambios propios: conservarlas si se desea
mantener esos cambios. Una copia incompleta no se sobrescribe automáticamente.

Para preparar o trasladar una biblioteca, consulta `docs/BIBLIOTECAS.md`.

## Estilo de imagen (desde 0.3.2)

Con DOSBox Staging, **Ajustes → Estilo de imagen** ofrece **Nítido** por defecto
y **Monitor clásico (CRT)** como alternativa. Ambos conservan las proporciones:
Nítido permite escalado fraccionario para aprovechar la pantalla y CRT usa el
escalado apropiado para sus líneas de barrido. Se ha probado con Staging 0.83.
La configuración de cada juego puede elegir otro estilo o volver al global.

El visor PDF usa una X para cerrar, centra las páginas verticales y abre cada
documento desde su primera página. También puede cerrarse con Escape.

## Visor PDF (mejoras de 0.3.4)

Manuales, extras PDF y revistas usan el mismo visor. El selector **Una página /
Doble página** permite mostrar la portada sola y luego los pares 2–3, 4–5, etc.
Una última página sin pareja aparece sola. **Página completa** y **Ajustar al ancho**
actúan sobre la vista completa, conservando las proporciones.

Los cursores **izquierdo/derecho** y los botones Anterior/Siguiente cambian de
página o de par. Los atajos funcionan con el visor abierto y se suspenden mientras
está desplegado el selector de vista. Cambiar el modo conserva la página elegida;
cada documento empieza en su portada y el modo se mantiene durante la sesión.
**X** o **Escape** cierran el visor.
