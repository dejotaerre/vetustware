# Cambios de VETUSTware

[English](CHANGELOG.md) · **Español**

## Pendiente de publicar

- Los motores privados de Stella y ScummVM permiten buscar manualmente actualizaciones en GitHub, comprobar compatibilidad e integridad, instalar y recuperar la versión anterior. El importador inicial admite Arch y CachyOS x86_64. Las preferencias y partidas se guardan aparte.
- Al pulsar la carátula en las fichas de DOS, ScummVM y Atari se abre la edición elegida con las mismas comprobaciones que al pulsar Jugar.
- Las cabeceras de ScummVM y Atari 2600 muestran imágenes que se adaptan al tamaño de la ventana.
- Cuando se identifica el monitor elegido, Stella usa Wayland nativo para evitar problemas de posición y tamaño en pantalla completa con XWayland y las capas de compatibilidad SDL.
- Las tarjetas de Atari muestran las cajas y conservan las capturas de juego en la ficha. Las asociaciones explícitas entre ROM y carátula se mantienen al actualizar el catálogo.
- La biblioteca de Atari 2600 con Stella incluye un catálogo transportable de ROMs e imágenes, variantes agrupadas, favoritos, juegos ocultos y ajustes de carpeta, pantalla completa y estilo de imagen.
- Stella guarda los estados nuevos y ajustes de los mandos en un directorio personal, separado de los estados históricos.
- Se añadió portugués de Brasil (`pt_BR`), con los 407 mensajes de interfaz traducidos en el momento de su incorporación.
- Inicio muestra el total del catálogo DOS cargado con el formato numérico del idioma elegido.
- Las fichas de DOS y ScummVM se cierran con una X en la esquina superior derecha, como el visor PDF.
- Se puede elegir una carpeta independiente para ScummVM o usar la subcarpeta automática de la biblioteca.
- ScummVM permite ocultar juegos por carpeta, mostrar los ocultos y restaurar su visibilidad. La elección se conserva entre sesiones.
- Las carpetas de ScummVM admiten una carátula local y un manual en PDF que se abre en el visor interno.
- Se añadió la sección ScummVM con detección por carpeta, elección de edición, imágenes del catálogo cuando están disponibles, búsqueda, ejecución directa y acceso al gestor nativo.
- Los ajustes y partidas nuevas de ScummVM se guardan dentro de VETUSTware, separados de DOSBox y de una instalación independiente de ScummVM.

### Arcade

- Se corrigió la apertura de juegos en el monitor equivocado al identificar la salida nativa de Wayland correspondiente a la pantalla elegida.
- La biblioteca Arcade usa MAME 0.241, con verificación de ROMs, ediciones regionales agrupadas, categorías, favoritos y juegos ocultos.
- Las fichas admiten folletos, fotos de máquinas, marquesinas, imágenes de paneles de control, pantallas de título e historia del juego.
- Se puede elegir la ubicación de la biblioteca, el monitor global, el modo de pantalla completa y las imágenes del mueble. La importación de ajustes y partidas iniciales respeta el progreso personal.
- Se añadieron textos en inglés, español y portugués de Brasil, pruebas del catálogo y del lanzamiento y documentación de Arcade.

## 0.3.4 — 2026-10-31

- Los cursores izquierdo y derecho recorren las páginas o los pares de páginas del visor PDF.
- El visor admite doble página, con portada sola y después las páginas 2–3, 4–5, etc. Los controles de navegación y ajuste actúan sobre el par completo.

## 0.3.3 — 2026-10-31

- La interfaz abre en inglés y se puede cambiar al español desde Ajustes.
- El idioma elegido se conserva entre sesiones y es independiente de las ediciones, los ajustes de emulación y las partidas.
- Se incorporaron traducciones comunitarias en JSON, un validador y una guía para traductores en inglés.
- Se añadió el README en inglés con capturas nuevas y se conservaron el README y las capturas en español.
- La descarga incluye instrucciones de instalación en inglés.
- Al actualizar, el instalador conserva el idioma elegido y los archivos de traducción añadidos por el usuario.

## 0.3.2 — 2026-10-31

- Nueva sección Revistas con colecciones, portadas locales, búsqueda y navegación
  por teclado. Los contenidos se aportan con la biblioteca, separados del motor.
- Lectura de revistas PDF e índices originales en el visor integrado.
- Big Blue Disk y Game Bytes con sus menús originales en DOSBox y copias locales
  que conservan intacta la biblioteca compartida. Respetan ajustes globales,
  monitor y estilo de imagen; sus sesiones no se mezclan con los juegos recientes.
- Nítido como estilo predeterminado en DOSBox Staging y Monitor clásico (CRT)
  como alternativa global o por juego, sin deformar las proporciones.
- Visor PDF con cierre mediante X o Escape, páginas verticales centradas y
  apertura de cada documento desde la primera página.
- Paquetes de fuentes y ejecutable actualizados, con documentación del formato
  de revistas y pruebas de portabilidad y preservación de datos.

## 0.3.1 — 2026-10-31

- Extras de biblioteca: documentos y medios complementarios desde las fichas.
- Visor PDF integrado y extracción de extras con verificación SHA256.

## 0.3.0

- Primera publicación del motor nativo y sus paquetes limpios para Linux.
