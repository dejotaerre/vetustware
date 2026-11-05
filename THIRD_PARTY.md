# Componentes externos

La licencia MIT incluida corresponde al código, documentación e icono propios
de VETUSTware. Cada dependencia conserva su propia licencia.

Este paquete no redistribuye Qt, SDL, libzip, DOSBox, juegos, ROM, fuentes MIDI,
carátulas, manuales, XTree ni Norton Commander. Utiliza las bibliotecas y
emuladores que instale el destinatario en su sistema.

Referencias oficiales:

- Qt: https://www.qt.io/licensing/ (se usan Core, Gui, Qml, Quick,
  QuickControls2, Sql, Concurrent, Pdf y PdfQuick).
- SDL: https://www.libsdl.org/license.php
- libzip: https://libzip.org/license/
- DOSBox Staging: https://www.dosbox-staging.org/
- DOSBox clásico: https://www.dosbox.com/
- DOSBox-X: https://dosbox-x.com/

La colección eXoDOS y sus metadatos se aportan localmente por cada usuario y
no forman parte del paquete. No hay descargas automáticas de juegos.

Las capturas de docs/images muestran la interfaz con contenidos aportados por
una biblioteca local. Las marcas, imágenes y demás contenidos de juegos que
aparecen en ellas pertenecen a sus titulares y no se incluyen en la licencia
MIT del código. No se distribuyen los archivos originales de esos contenidos.

Stella es una dependencia opcional para Atari 2600 y se instala por separado.
Sitio y documentación: https://stella-emu.github.io/ . Su licencia es GPL;
VETUSTware invoca su ejecutable y consulta la base local mediante `-listrominfo`.
El paquete de VETUSTware no contiene Stella, su base de ROMs, ROMs Atari. Los catálogos generados pertenecen a la biblioteca local
y conservan metadatos procedentes de Stella.

La fotografía de cabecera `native/assets/atari2600-header.png` fue aportada
por el usuario para la interfaz. La imagen y las marcas no están cubiertas
por la licencia MIT del código. No se ha identificado su licencia de redistribución.

La ilustración `native/assets/scummvm-header.png` se generó con IA para esta
interfaz a partir de una referencia temática aportada por el usuario. No es
material oficial de ScummVM; el nombre y sus marcas pertenecen a sus titulares.

Los runtimes privados de Stella y ScummVM pueden importarse desde los paquetes
nativos del equipo o instalarse mediante paquetes compatibles publicados para
VETUSTware. Se conservan sus licencias, recursos y procedencia en cada runtime.
No forman parte de la licencia MIT ni se incluyen automáticamente en el paquete
de la aplicación. Véase docs/RUNTIMES.md para preparación y redistribución.

## MAME 0.241 (optional private runtime)

Arcade requires the matching MAME 0.241 runtime. MAME is licensed under GPL-2.0-or-later; components have additional notices. The locally prepared runtime records its signed Arch package origin and private dependency versions in `PROVENANCE.json`, and retains license files. Before distributing this runtime, provide the corresponding sources and notices for every bundled component. Library ROMs, CHDs and artwork are separate third-party material and are not covered by VETUSTware's MIT license. See `docs/ARCADE.md`.

La cabecera `native/assets/arcade-header.png` fue compuesta y recortada con IA
sobre imágenes de Pac-Man, Donkey Kong y Street Fighter II de `mame/cabinets/`
de la biblioteca aportada por el usuario. Tiene fondo transparente. Las imágenes
y marcas de referencia pertenecen a sus titulares y no están cubiertas por la
licencia MIT del código. No se ha identificado su licencia de redistribución.
