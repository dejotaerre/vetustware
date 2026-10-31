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
