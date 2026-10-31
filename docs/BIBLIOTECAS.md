# Bibliotecas locales

El motor se instala por separado de los contenidos. Al abrirlo por primera vez,
selecciona la raíz de una biblioteca compatible. Puede estar en otro disco y
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
