#!/usr/bin/env python3
"""Crea paquetes limpios mediante una lista explícita de archivos publicables."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tarfile
import tempfile
import time

VERSION = '0.3.4'
NATIVE = '''CMakeLists.txt PdfReader.qml Main.qml GameCustomizationDialog.qml CatalogView.qml BackgroundsDialog.qml LibraryBackdrop.qml EmulatorDialog.qml StorageDialog.qml ExtrasDialog.qml MagazinesView.qml ScummView.qml
RuntimesDialog.qml runtimes.cpp runtimes.h ArcadeView.qml arcade.cpp arcade.h arcade_tests.cpp
assets/scummvm-header.png assets/atari2600-header.png assets/arcade-header.png assets/retro-library.jpg assets/magazines.jpg
scummvm.cpp scummvm.h scummvm_tests.cpp AtariView.qml atari.cpp atari.h atari_tests.cpp
archive.cpp archive.h archive_tests.cpp backend.cpp backend.h backend_customization.cpp backend_magazines.cpp
magazines.cpp magazines.h magazines_tests.cpp displays.cpp displays.h
emulation.cpp emulation.h emulation_tests.cpp home.cpp home_tests.cpp main.cpp
storage.cpp storage.h librarylayout.h extras.cpp extras.h extras_tests.cpp icon.svg i18n.cpp i18n.h i18n_tests.cpp'''.split()
HELPERS = ['library_layout.py', 'library_layout_tests.py', 'install.py', 'catalog_index.py', 'import_library.py', 'package_release.py', 'check_translations.py', 'atari_index.py', 'atari_index_tests.py', 'mame_index.py', 'mame_launch.py', 'mame_tests.py', 'runtime_manager.py', 'runtime_package.py', 'runtime_manager_tests.py']
DOCS = ['LICENSE', 'THIRD_PARTY.md', 'INSTALACION.md', 'INSTALLATION.md', 'CHANGELOG.md', 'CHANGELOG.es.md', 'THIRD_PARTY.es.md', 'docs/LIBRARIES.md', 'docs/BIBLIOTECAS.md', 'docs/TRANSLATING.md', 'docs/CUSTOMIZING.md', 'docs/RUNTIMES.md', 'docs/ARCADE.md']


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    with log.open('a') as stream:
        stream.write(result.stdout)
    if result.returncode:
        raise RuntimeError(f'Falló {command[0]}; consulta {log}')


def pack(stage, names, output, label):
    # No se incluye ningún directorio entero: no pueden filtrarse datos o cachés nuevos.
    manifest = {name: sha(stage / name) for name in names}
    manifest_name = 'MANIFEST.sha256'
    (stage / manifest_name).write_text(''.join(f'{digest}  {name}\n' for name, digest in sorted(manifest.items())))
    names = sorted(names + [manifest_name])
    private = [str(Path.home()).encode()]
    for name in names:
        data = (stage / name).read_bytes()
        if any(value in data for value in private):
            raise RuntimeError(f'Se detectó una ruta privada en {name}')
    with tarfile.open(output, 'w:gz', format=tarfile.PAX_FORMAT) as archive:
        for name in names:
            path = stage / name
            info = archive.gettarinfo(path, arcname=label + '/' + name)
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mtime = int(os.environ.get('SOURCE_DATE_EPOCH', '1793318400'))
            info.pax_headers = {}
            with path.open('rb') as stream:
                archive.addfile(info, stream)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.absolute()
    output.mkdir(parents=True, exist_ok=True)
    label = f'vetustware-{VERSION}'
    binary_archive = output / f'{label}-linux-x86_64.tar.gz'
    source_archive = output / f'{label}-fuentes.tar.gz'
    if any(p.exists() for p in (binary_archive, source_archive)):
        raise RuntimeError('El destino ya contiene esta versión; no se sobrescribe.')
    if platform.machine() != 'x86_64':
        raise RuntimeError('Este generador de binarios se prueba en Linux x86_64.')
    log = output / 'build.log'
    with tempfile.TemporaryDirectory(prefix='vetustware-release-') as temporary:
        stage = Path(temporary) / label
        stage.mkdir()
        names = [f'native/{name}' for name in NATIVE] + [f'helpers/{name}' for name in HELPERS] + DOCS + ['translations/' + path.name for path in sorted((source / 'translations').glob('*.json'))]
        for name in names:
            dst = stage / name
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / name, dst)
        build = stage / 'native/build'
        flags = f'-march=x86-64 -mtune=generic -ffile-prefix-map={stage}=. -fdebug-prefix-map={stage}=.'
        run(['cmake', '-S', str(stage / 'native'), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_FLAGS=' + flags], log)
        run(['cmake', '--build', str(build), '-j', '4'], log)
        run(['ctest', '--test-dir', str(build), '--output-on-failure'], log)
        shutil.copy2(build / 'vetust', stage / 'vetustware')
        run(['strip', '--strip-unneeded', str(stage / 'vetustware')], log)
        shutil.copy2(stage / 'native/icon.svg', stage / 'icon.svg')
        (stage / 'vetustware.install').write_text('1\n')
        build_info = {'version': VERSION, 'system_tested': 'CachyOS Linux', 'architecture': platform.machine(),
                      'cpu_target': 'x86-64 baseline', 'qt': subprocess.check_output(['pkg-config', '--modversion', 'Qt6Core'], text=True).strip(),
                      'glibc': platform.libc_ver()[1], 'compiler': subprocess.check_output(['c++', '-dumpfullversion'], text=True).strip(),
                      'bundled_emulators': False, 'bundled_shared_libraries': False, 'license': 'MIT'}
        (stage / 'BUILD_INFO.json').write_text(json.dumps(build_info, indent=2) + '\n')
        names += ['BUILD_INFO.json']
        pack(stage, names, source_archive, label)
        pack(stage, names + ['vetustware', 'icon.svg', 'vetustware.install'], binary_archive, label)
    (output / 'SHA256SUMS').write_text(''.join(f'{sha(path)}  {path.name}\n' for path in (binary_archive, source_archive)))
    print(binary_archive)
    print(source_archive)


if __name__ == '__main__':
    main()
