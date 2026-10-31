# VETUSTware

**Tu colección DOS, lista para explorar y jugar.**

VETUSTware es una aplicación nativa para Linux, escrita en C++20 y Qt Quick,
que organiza una biblioteca local de juegos MS-DOS y los abre con DOSBox.
Carátulas, fichas, manuales y configuraciones individuales en una interfaz
pensada para disfrutar de la colección. No necesita un navegador ni una cuenta.

*Native Linux frontend for local DOS game collections, with DOSBox profiles,
artwork, manuals and portable libraries. The current interface is in Spanish.*

![Inicio de VETUSTware](docs/images/inicio.png)

[Descargar 0.3.1](https://github.com/dejotaerre/vetustware/releases/tag/v0.3.1)
· [Instalación](INSTALACION.md)
· [Bibliotecas](docs/BIBLIOTECAS.md)
· [Reportar un problema](https://github.com/dejotaerre/vetustware/issues)

## Qué puedes hacer

- Explorar categorías y selecciones, buscar juegos, guardar favoritos y volver a los recientes.
- Consultar carátulas, capturas y manuales desde la ficha de cada juego.
- Elegir DOSBox Staging, DOSBox clásico o DOSBox-X, con ajustes globales y por juego.
- Cambiar pantalla completa y elegir monitor cuando hay más de uno disponible.
- Acceder a opciones avanzadas de CPU, memoria, vídeo, sonido y ejecutable del emulador.
- Abrir una consola DOS con las unidades y las imágenes del juego montadas.
- Usar XTree Gold o Norton Commander cuando la biblioteca aporte esas herramientas.
- Reconocer ediciones y perfiles personalizados de una biblioteca VETUSTware.

Desde 0.3.1, el botón **Extras** permite consultar los documentos y medios
complementarios que aporte la biblioteca. Los PDF se muestran dentro de la
aplicación; otros formatos usan el visor del sistema. Consulta
[el formato de complementos](docs/BIBLIOTECAS.md#complementos-opcionales-desde-031).

![Ficha de un juego](docs/images/ficha.png)

Las capturas ilustran una biblioteca aportada localmente. Los juegos, imágenes
y manuales que aparecen no se incluyen en el programa ni en sus descargas.

## Empezar

La versión **0.3.1 es una versión de prueba para Linux x86_64**.
El ejecutable publicado se compiló y probó en **CachyOS con Qt 6.12**. No es un
paquete universal: necesita bibliotecas compatibles del sistema. El código
fuente permite compilar en otras distribuciones; su compatibilidad todavía
requiere pruebas.

1. Instala las dependencias indicadas en [INSTALACION.md](INSTALACION.md) y un
   emulador DOSBox. Los perfiles se han trabajado principalmente con DOSBox Staging.
2. Descarga y extrae `vetustware-0.3.1-linux-x86_64.tar.gz` desde
   [Releases](https://github.com/dejotaerre/vetustware/releases).
3. Dentro de la carpeta extraída, ejecuta:

   ```sh
   python3 helpers/install.py --desktop
   ```

4. Abre VETUSTware y elige tu biblioteca local compatible.

El destino predeterminado es `~/vetustware`. La biblioteca puede estar en otro
disco. Las copias jugables se crean al necesitarlas en `data/games` dentro de
la instalación, o en la carpeta alternativa elegida por el usuario.

El motor **no incluye ni descarga juegos, ISOs, ROMs, carátulas, manuales,
emuladores ni herramientas DOS de terceros**. También funciona con una
biblioteca personalizada ya preparada, siempre que se aporte por separado.

## Motor, biblioteca y partidas

| Parte | Contenido | Ubicación |
|---|---|---|
| Motor | Aplicación, helpers, ajustes y datos locales | `~/vetustware` por defecto |
| Biblioteca | Archivos originales, metadatos y personalizaciones opcionales | Cualquier carpeta o disco disponible |
| Copias jugables | Archivos extraídos y partidas de cada usuario | `data/games` o destino personalizado |

La biblioteca se consulta en lectura. Las partidas y cambios de cada jugador
se guardan en sus copias jugables. Recuperar una copia desde la biblioteca no
recupera partidas posteriores que se hayan borrado.

La versión actual admite la estructura eXoDOS descrita en la guía y las
bibliotecas personalizadas VETUSTware. No es un importador general de carpetas
arbitrarias de juegos o ISOs. Consulta [el formato de biblioteca](docs/BIBLIOTECAS.md).

## Compilar y probar

Se necesitan C++20, CMake 3.21 o posterior, pkg-config, Qt 6.5 o posterior
(Quick, QuickControls2, Sql/SQLite, Concurrent, PdfQuick y Test), libzip y SDL2.
Python 3 se utiliza para instalar e importar metadatos.

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build -j 4
ctest --test-dir native/build --output-on-failure
python3 helpers/install.py --desktop
```

Las pruebas utilizan datos sintéticos y no necesitan una colección de juegos.
Incluyen extracción ZIP, catálogo, emulación y navegación de la interfaz.

## Estado y límites

- Interfaz en español; los nombres y descripciones importados dependen de la biblioteca.
- Cada juego puede requerir un motor DOSBox o ajustes específicos. Importar un perfil no verifica su compatibilidad.
- Escritorio Linux con X11 o XWayland; otras plataformas y Wayland nativo no están validados en esta versión.
- El selector de monitor depende de las capacidades del emulador y del entorno gráfico.
- No hay descargas automáticas de bibliotecas, traducciones ni imágenes.

Para colaborar o informar de errores, consulta [CONTRIBUTING.md](CONTRIBUTING.md).

## Licencia

El código propio de VETUSTware se publica con licencia [MIT](LICENSE).
Las dependencias y los contenidos de terceros conservan sus propias licencias.
Las imágenes de juegos visibles en las capturas no están cubiertas por la
licencia MIT del código. Más información en [THIRD_PARTY.md](THIRD_PARTY.md).
