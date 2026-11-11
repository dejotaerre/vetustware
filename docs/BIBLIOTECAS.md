# Bibliotecas locales

[English](LIBRARIES.md) · **Español**

## Estructura organizada de VETUSTware

Cada colección contiene sus juegos y una subcarpeta `metadata`. Al elegir la
colección, VETUSTware encuentra sus metadatos automáticamente:

```text
library.json
games/{dosbox,scummvm,atari2600,mame}/
games/{dosbox,scummvm,atari2600,mame}/metadata/
games/dosbox/resources/audio/{roland-mt32,soundfonts}/
games/dosbox/resources/dos/
games/mame/resources/
magazines/
magazines/metadata/
```

DOS utiliza `games/dosbox/archives/` y `versiones-especiales.zip`; su catálogo
personalizado está en `games/dosbox/metadata/perfiles.zip`, las imágenes y manuales en
`recursos.zip`, los perfiles importables en `perfiles-originales.zip` y los extras
en `extras/`. `recursos-compartidos.json` referencia `recursos.zip`.
Las ROMs Roland y el banco SoundCanvas están en games/dosbox/resources/audio. Los nombres
heredados dentro de los perfiles y ZIP originales se interpretan al importar;
no es necesaria ninguna carpeta externa con esos nombres.

ScummVM conserva los nombres relativos de sus carpetas; las carátulas y manuales
están en `games/scummvm/metadata/<carpeta-del-juego>/`. Atari y MAME mantienen las rutas
relativas de sus catálogos: los juegos se resuelven dentro de games/<motor> y
las imágenes dentro de games/<motor>/metadata. Sus indexadores aceptan `--metadata`
para actualizar catálogos sin mezclar imágenes con ROMs. El lanzador MAME recibe
esa misma opción para encontrar el artwork. Los identificadores, favoritos,
ocultos, opciones y partidas no dependen de esta separación.

La instalación personal conserva preferencias, partidas y copias DOS jugables;
la biblioteca conserva las ediciones iniciales que pueden restaurarse en una
instalación nueva. Cada carpeta games/<motor> puede empaquetarse por separado,
incluyendo sus metadatos. Los recursos de cada motor viajan dentro de su carpeta resources. Los ZIP pueden expandir el
espacio ocupado por enlaces duros al extraerse, sin perder contenido.

Esta estructura requiere una compilación con soporte para bibliotecas organizadas;
el paquete público 0.3.4 no la reconoce. También se reconoce la estructura organizada anterior con nombres en español.
La estructura original sigue siendo
importable, como se documenta a continuación.


El motor se instala por separado de los contenidos. Al abrirlo por primera vez,
selecciona las carpetas de las colecciones que quieras usar. Puede estar en otro disco y
puede llamarse como prefieras; no se presupone una carpeta Juegos o Games.

## Colección base

La estructura que reconoce el importador es:

```text
biblioteca/
├── Content/
│   ├── XODOSMetadata.zip
│   └── !DOSmetadata_linux.zip
└── eXo/
    ├── eXoDOS/
    │   └── archivos ZIP de los juegos
    └── mt32/
        └── recursos de sonido, cuando se necesiten
```

`XODOSMetadata.zip` aporta catálogo, imágenes y manuales; el paquete de
metadatos Linux aporta perfiles DOSBox. Los archivos de juego se extraen al
necesitarlos. Los recursos de sonido dependen de los perfiles utilizados.
Esta estructura corresponde a las colecciones eXoDOS compatibles probadas;
no se garantiza compatibilidad con todas las versiones de eXoDOS.

## Biblioteca personalizada

Una biblioteca preparada para VETUSTware 0.3.0 puede añadir:

```text
Personalizaciones/
├── perfiles.zip
├── juegos.zip
└── herramientas.zip  (opcional)
```

El motor reconoce los perfiles y las ediciones desde la biblioteca, y extrae
solo la copia elegida al pulsar Jugar o abrir una consola. Las herramientas DOS,
si se aportan, se preparan localmente al abrir la consola. No están incluidas en
el motor ni pueden obtenerse desde este repositorio.

Los perfiles describen montajes e instrucciones DOSBox: usa bibliotecas y
perfiles de una procedencia que conozcas. El motor no descarga contenidos ni
busca automáticamente archivos faltantes en Internet.

## Qué se conserva

Las preferencias, favoritos e historial pertenecen a cada instalación. Las
copias jugables y partidas se guardan en `data/games` por defecto, con la opción
de elegir otro destino. La biblioteca no se modifica durante las partidas.

Si falta una copia completa, se puede recuperar desde la biblioteca. Las
partidas o modificaciones posteriores que solo existían en esa copia no se
recuperan al borrarla. Una carpeta incompleta existente se conserva y requiere
revisión, para evitar sobrescribir archivos del usuario.

Para trasladar una carpeta de juegos, copia todo su contenido —incluidos los
archivos ocultos— y actualiza el destino desde Biblioteca y carpetas. Cambiar la
ruta en los ajustes no mueve físicamente los archivos.

La instantánea personalizada de una biblioteca tampoco se actualiza al cambiar
los juegos instalados; las nuevas personalizaciones deben exportarse de nuevo.
Esta versión del repositorio no ofrece un exportador general de bibliotecas.

## Recursos compartidos opcionales

`Content/recursos-compartidos.json` conserva los nombres históricos de imágenes y
manuales idénticos que comparten un miembro de `XODOSMetadata.zip`. Tiene `format: 1`
y `archives`, cuyo objeto `XODOSMetadata.zip` relaciona cada nombre anterior con
`entry` (miembro conservado), `bytes` y `sha256`. Los destinos deben existir en ese
mismo ZIP; no se admiten rutas externas ni cadenas de referencias.

El catálogo de `Personalizaciones/perfiles.zip` apunta directamente a los miembros
conservados, por lo que el visor y las fichas no necesitan resolver alias. El
importador también consulta este mapa al reconstruir un catálogo desde el XML:
mantiene los identificadores de imagen y la asociación de cada nombre histórico
con su juego. Debe conservarse el mapa junto al ZIP al trasladar la biblioteca.

Los paquetes de artwork de MAME también pueden presentarse como carpetas con el
nombre del sistema, conservando los layouts y sus imágenes. Las copias idénticas
pueden compartir almacenamiento mediante enlaces duros. Al editar una imagen por
separado debe romperse antes su enlace; copiar sin preservar enlaces conserva el
contenido, pero puede aumentar el espacio ocupado.

## Complementos opcionales (desde 0.3.1)

```text
Complementos/
├── metadata-original.zip
├── variantes-locales.zip
├── conservacion.json
├── extras.json
├── SHA256SUMS
└── LEEME.txt
```

Los dos ZIP conservan documentación original y variantes técnicas históricas.
No sustituyen los perfiles de `Personalizaciones/perfiles.zip`. El inventario
`conservacion.json` documenta la cobertura de los archivos originales mediante
rutas relativas y SHA-256; el motor no necesita leerlo durante el uso normal.

`extras.json` tiene `format: 1` y un objeto `games`, cuyas claves son los ID del
catálogo. Cada valor es una lista de documentos con `name`, `kind`, `bytes`,
`sha256`, `archive` (ruta relativa a la biblioteca) y `entry` (miembro del ZIP).
Un documento puede referirse a un ZIP existente para evitar duplicar contenido.
El motor permite documentos y medios, rechaza scripts/ejecutables y rutas que
salgan de la biblioteca, y verifica el archivo antes de abrirlo.

Se extrae solamente el documento elegido. Un HTML con referencias relativas a
otros archivos puede necesitar sus recursos adicionales; el visor no reconstruye
un sitio web completo. Conservar un archivo no garantiza que el equipo receptor
tenga un visor instalado para su formato.


## Revistas opcionales

Desde 0.3.2 se admite `Revistas/revistas.json` (formato 1).
Las versiones anteriores siguen usando los juegos, pero no muestran Revistas.

El catálogo contiene dos listas:

- `series`: `id`, `title`, `description`, `kind` (`pdf` o `dos`), `count`,
  `cover` opcional, `indexId` para el índice PDF o `launchId` para la revista DOS.
- `entries`: `id`, `series`, `title`, `kind` y `path`. Los PDF pueden incluir
  `cover`, `bytes` e `isIndex`; los índices no duplican números en la cuadrícula.
  Las entradas DOS incluyen `revision`, un SHA256 del inventario de su contenido.

`path` y `cover` son relativos a `Revistas/`. No se admiten rutas externas a esa
carpeta. Las portadas son imágenes locales: no hay descarga ni servicio externo.
Los PDF se leen directamente. Los índices originales pueden contener enlaces
externos o referencias históricas; la navegación por números usa el catálogo.

Los perfiles DOS admitidos son `BBD` y `GameBytes`, ambos con `run.bat` en su raíz.
Al abrirlos, el motor prepara una copia completa en
`data/magazines/<carpeta>-<revision>` dentro de su instalación. Solo DOSBox ejecuta
el menú de esa copia; los lanzadores `.sh` de la biblioteca no se ejecutan.
La preparación es atómica y una copia incompleta existente no se sobrescribe.
Se usan los ajustes globales de emulación, monitor y estilo de imagen. Estas
sesiones no se añaden al historial de juegos.

Se puede compartir y mover `Revistas/` junto al resto de la biblioteca. Los
archivos `conservacion.json` y `SHA256SUMS`, si se aportan, documentan la integridad
del material original y de los archivos añadidos. Las copias de trabajo locales
pueden contener cambios del lector y no forman parte de la biblioteca compartida.

## Subbiblioteca ScummVM

La pestaña **SCUMMVM** utiliza la carpeta elegida para esa colección. Coloca cada juego en su propia subcarpeta, con sus datos ya
extraídos. El primer acceso detecta los juegos; **Actualizar catálogo** vuelve
a buscarlos cuando agregas material. Las subcarpetas anidadas también se revisan.

Cada carpeta se detecta por separado para conservar copias y ediciones
similares. Si ScummVM ofrece varias coincidencias, VETUSTware pide elegir una:
la detección no demuestra que todas esas variantes funcionen. Las carpetas sin
coincidencias permanecen visibles para poder revisarlas. Las carátulas se
reutilizan del catálogo DOS únicamente cuando existe una equivalencia conocida;
los títulos sin arte muestran su nombre.

**Gestionar en ScummVM** abre el gestor nativo con la configuración exclusiva
de VETUSTware. Allí se pueden editar gráficos, sonido y opciones por juego.
Quitar una entrada en ese gestor no borra los archivos; una búsqueda posterior
puede volver a detectarla. El catálogo de VETUSTware muestra únicamente los
juegos ubicados dentro de esta subbiblioteca.

- Configuración: `config/scummvm.ini`, dentro de la instalación de VETUSTware.
- Partidas nuevas: `data/scummvm/saves/<identificador>`, dentro de la instalación.
- Respaldos de configuración: `backups/scummvm`.
- Registros: `logs/scummvm*.log`.

La búsqueda conserva los ajustes de las ediciones existentes y no escribe en
los archivos de los juegos. Las partidas y la configuración habitual de una
instalación independiente de ScummVM no se importan ni se modifican. Si cambias
la ruta de partidas desde el gestor, se respeta esa elección.

Se utiliza ScummVM instalado en el sistema o un ejecutable en
`runtime/scummvm/scummvm` (`scummvm.exe` en Windows). El paquete de VETUSTware no
incluye ScummVM. Esto no implica que el port de VETUSTware a Windows esté terminado.

En cada carpeta de juego ScummVM se puede incluir `manual.pdf` y una imagen
`portada.jpg`, `cover.jpg`, `portada.png` o `cover.png`. La portada local tiene
prioridad sobre el arte del catálogo DOS y el manual se abre en el visor interno,
con doble página y controles de teclado. Estos archivos acompañan al juego al
trasladar la biblioteca; no dependen de rutas del equipo que la preparó.

### Ocultar juegos de ScummVM

En la ficha de cada carpeta, «Ocultar juego» la retira de la vista habitual sin borrar archivos, partidas ni configuración. «Mostrar ocultos» incluye también estas carpetas, marcadas como «Oculto»; desde su ficha, «Volver a mostrar» restaura su visibilidad. La búsqueda respeta este filtro. La elección es personal, se guarda en config/scummvm.ini de la aplicación y se conserva al actualizar el catálogo o mover la biblioteca. Cada entrada representa una carpeta con sus posibles ediciones.

### Carpeta independiente para ScummVM

En Ajustes → Bibliotecas y almacenamiento, ScummVM permite elegir otra ubicación, incluso en otro disco. Selecciona la carpeta que contiene las carpetas de cada juego y metadata. El cambio actualiza el catálogo sin trasladar archivos; conserva las opciones, los ocultos y las partidas de las carpetas que mantienen su nombre. La ruta opcional se guarda en config/storage.json, clave scummvm. Una ruta personalizada no se sustituye por otra al desconectar su disco.


### Fichas independientes

`metadata/<carpeta-del-juego>/game.json` permite conservar una ficha independiente: título, año, estudio, editor, género, descripciones por idioma, caja 3D, capturas y manual. La descripción sigue el idioma de la interfaz y vuelve al inglés si no hay traducción. Las portadas y manuales locales existentes tienen prioridad. Las capturas copiadas de DOS se identifican como imágenes de referencia, pues la edición elegida en ScummVM puede ser distinta.

El JSON utiliza `"format": 1` y un objeto `"games"` cuyas claves son el `gameid` exacto de ScummVM. Cada registro admite `title`, `year`, `developer`, `publisher`, `genre`, `descriptions` (claves `en`, `es`, `pt_BR`), `cover`, `box3d`, `background`, `manual` y una lista `shots`. Las rutas de los recursos son relativas a la carpeta que contiene `game.json`; se rechazan archivos externos. `source.engine` con valor `dosbox` identifica la galería como material de referencia. Varios identificadores en una carpeta conservan sus fichas separadas.

Al copiar el JSON junto con sus recursos, la colección ScummVM deja de necesitar DOSBox para mostrar esa ficha. Este formato no importa automáticamente un catálogo DOS: también hay que convertir los registros y copiar los archivos referenciados. Si no existe un registro local correspondiente, continúa disponible el respaldo visual del catálogo DOSBox.

## Atari 2600 y Stella

La sección Atari usa la carpeta elegida en **Bibliotecas y almacenamiento**
o **Ajustes de Atari**. Estructura:

```text
atari2600/
├── roms/              # ROMs .bin y .a26, incluidas subcarpetas
└── metadata/
    ├── boxes/
    ├── snapshots/
    ├── covers.json
    └── catalog.json
```

El índice agrupa variantes por nombre y copias idénticas por contenido MD5.
No modifica ROMs ni estados. Los fabricantes, mandos y formatos conocidos
proceden de la base local de Stella; algunos títulos dependen de los nombres
de archivo y pueden requerir revisión. Las imágenes de otra edición del mismo
título se usan como alternativa cuando no existe coincidencia exacta.

Para generar el catálogo portátil en una biblioteca propia:

```sh
python3 helpers/atari_index.py --library /ruta/biblioteca/atari2600 \
  --output /ruta/biblioteca/atari2600/metadata/catalog.json --stella /usr/bin/stella
```

**Actualizar catálogo** solo guarda un índice en la instalación del usuario;
no escribe sobre la biblioteca compartida. La carpeta completa es trasladable.
Los ajustes de VETUSTware quedan en `config/atari.json`; Stella usa
`data/stella` para su configuración, partidas, capturas y memoria persistente.
Los mandos se ajustan en Stella, no se fuerzan a joystick para juegos de paddle.
El monitor respeta la selección global de VETUSTware.

La conservación de estados de Stella 3.2.2 mantiene los archivos originales,
pero no implica compatibilidad con Stella actual. No se mezclan con estados nuevos.

### Cajas y capturas de Atari

Las tarjetas muestran cajas; las capturas se mantienen disponibles mediante
**Captura** dentro de la ficha. Una caja ausente se muestra como pendiente,
sin sustituirla por una captura. Las imágenes mantienen sus proporciones.

`atari2600/metadata/boxes` contiene las cajas y `atari2600/metadata/snapshots` las capturas.
`covers.json` registra asociaciones explícitas por MD5 de ROM:

```json
{"format":1,"variants":{"MD5_DE_LA_ROM":"boxes/nombre-de-la-caja.png"}}
```

Las rutas de imágenes son relativas a `atari2600/metadata`. Una misma caja puede representar varias
variantes del mismo título. La asociación se conserva al trasladar la carpeta
y al usar **Actualizar catálogo**. Tras editar este archivo, actualiza el
catálogo para incorporar los cambios. Las rutas externas a la biblioteca se
rechazan. Un catálogo compartido más nuevo tiene prioridad sobre la caché local.

La colección preparada incluye `box-matching.json`, con las asociaciones,
las equivalencias de título revisadas y los casos pendientes o ambiguos.
La semejanza aproximada de nombres no asigna imágenes automáticamente.

## Arcade / MAME 0.241

La sección Arcade usa la carpeta elegida para MAME; se puede elegir otra ubicación en Ajustes de Arcade. Su catálogo agrupa variantes y verifica ROMs con MAME 0.241. La colección puede llevar folletos, fotos de máquinas, marquesinas, capturas y configuraciones/partidas iniciales. Las sesiones nuevas usan `data/mame/0.241` en la instalación personal y no sobrescriben la biblioteca compartida. Consulta `ARCADE.md` para el diseño completo.


## Ubicaciones independientes

«Bibliotecas y almacenamiento» permite elegir DOSBox, ScummVM, Atari 2600,
Arcade y Revistas por separado. Cada colección puede estar en otro disco y lleva
su subcarpeta `metadata`. No se configura una ruta adicional de metadatos ni se
necesita una biblioteca principal. «Detectar carpetas» facilita seleccionar varias
colecciones de una estructura organizada. Cambiar una ruta no mueve archivos.

DOSBox lleva `resources/audio` (ROMs Roland y bancos de sonido) y
`resources/dos` (herramientas de consola). MAME conserva su material auxiliar
en `resources`, sin activar plugins antiguos: el runtime sigue usando los de
0.241. Estas subcarpetas acompañan a cada colección, sin otro selector de ruta. Las copias DOS jugables se configuran en Avanzado; el resto de las
partidas y preferencias permanece en la instalación de VETUSTware.

En Revistas, `metadata/revistas.json` contiene el catálogo; `path` y `cover` son
relativos a la carpeta de revistas. Las portadas están en `metadata/covers`.
Se siguen leyendo catálogos antiguos en la raíz y bibliotecas con metadatos
compartidos, para permitir migraciones sin invalidar colecciones anteriores.
