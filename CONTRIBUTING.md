# Colaborar con VETUSTware

Los informes de problemas y las contribuciones se reciben en GitHub Issues y
Pull Requests. La versión inicial se ha probado en CachyOS; las pruebas en
otras distribuciones son especialmente útiles.

## Informar de un problema

Indica distribución, versión de Qt, escritorio y si usas X11 o Wayland,
versión de VETUSTware, emulador y pasos para reproducirlo. En problemas de un
juego, incluye su nombre y edición sin adjuntar sus archivos.

El comando `vetustware --diagnostics` puede ayudar, pero muestra rutas locales.
Revisa los diagnósticos, registros y capturas antes de publicarlos para quitar
datos personales. No adjuntes partidas privadas, juegos, ISOs, ROMs ni claves.

## Cambiar el código

1. Compila y ejecuta las pruebas siguiendo el README.
2. Mantén los cambios centrados en el problema que resuelven.
3. Añade pruebas cuando el cambio de comportamiento lo necesite; usa datos
   sintéticos en lugar de archivos de juegos.
4. Escribe los nuevos comentarios de código en español neutro.
5. Describe el comportamiento corregido y cómo lo verificaste.

Antes de enviar cambios, comprueba que no se hayan incluido data/, config/,
runtime/, bibliotecas, registros ni rutas personales. Las contribuciones al
código propio se publican bajo la licencia MIT del proyecto.
