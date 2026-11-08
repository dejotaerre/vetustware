#!/usr/bin/env python3
"""Instala o actualiza VETUSTware sin sobrescribir datos personales."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import sqlite3
import tempfile


def dump(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2))


def running_under(root):
    for entry in Path('/proc').iterdir():
        if entry.name.isdigit():
            try:
                executable = Path(os.readlink(entry / 'exe').removesuffix(' (deleted)'))
                if executable.is_relative_to(root):
                    return True
            except (OSError, RuntimeError):
                pass
    return False


def copy_program(source, stage, personal):
    binary = source / 'native/build/vetust'
    if not binary.is_file():
        binary = source / 'vetustware'
    if not binary.is_file():
        raise RuntimeError('Compila primero el ejecutable nativo.')
    shutil.copy2(binary, stage / 'vetustware')
    icon = source / 'native/icon.svg'
    shutil.copy2(icon if icon.exists() else source / 'icon.svg', stage / 'icon.svg')
    shutil.copytree(source / 'helpers', stage / 'helpers', ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copytree(source / 'translations', stage / 'translations')
    shutil.copy2(source / 'docs/TRANSLATING.md', stage / 'TRANSLATING.md')
    shutil.copy2(source / 'docs/CUSTOMIZING.md', stage / 'CUSTOMIZING.md')
    runtime = source / 'runtime'
    if runtime.exists():
        shutil.copytree(runtime, stage / 'runtime', symlinks=True,
                        ignore=None if personal else shutil.ignore_patterns('dos-tools'))
    else:
        (stage / 'runtime').mkdir()
    for name in ('INSTALACION.md', 'INSTALLATION.md', 'LICENSE', 'THIRD_PARTY.md', 'THIRD_PARTY.es.md', 'BUILD_INFO.json'):
        doc = source / name
        if doc.exists():
            shutil.copy2(doc, stage / name)
    (stage / 'vetustware.install').write_text('1\n')


def migrate(source, stage, library, games):
    if not games.is_dir():
        raise RuntimeError('La carpeta de juegos no está disponible.')
    old_games = source / 'data/games'
    if (source / 'config/storage.json').exists():
        old_games = Path(json.loads((source / 'config/storage.json').read_text())['games'])
    for path in (source / 'data').iterdir():
        if path.name in {'games', 'spanish-source', 'native.lock', 'emulation.json', 'emulator-config', 'logs', 'personal.sqlite', 'personal.sqlite-wal', 'personal.sqlite-shm'}:
            continue
        target = stage / 'data' / path.name
        if path.is_dir():
            shutil.copytree(path, target, symlinks=True)
        elif path.is_file():
            shutil.copy2(path, target)
    database = source / 'data/personal.sqlite'
    if database.exists():
        with sqlite3.connect(f'file:{database}?mode=ro', uri=True) as old, sqlite3.connect(stage / 'data/personal.sqlite') as new:
            old.backup(new)
    if (source / 'config').is_dir():
        shutil.copytree(source / 'config', stage / 'config', dirs_exist_ok=True, symlinks=True)
    settings = source / 'config/emulation.json'
    if not settings.exists():
        settings = source / 'data/emulation.json'
    if settings.exists():
        # Las rutas propias se convierten en rutas relativas al instalar definitivamente.
        content = settings.read_text()
        data = json.loads(content)
        for engine, value in data.get('paths', {}).items():
            if value.startswith(str(source) + '/'):
                data['paths'][engine] = '${APP}/' + str(Path(value).relative_to(source))
        dump(stage / 'config/emulation.json', data)
    if (source / 'data/emulator-config').exists():
        shutil.copytree(source / 'data/emulator-config', stage / 'config/emulators', dirs_exist_ok=True)
    if (source / 'data/logs').exists():
        shutil.copytree(source / 'data/logs', stage / 'logs', dirs_exist_ok=True)
    for name in ('original-profiles.json', 'spanish-profiles.json'):
        path = stage / 'data' / name
        if path.exists():
            path.write_text(path.read_text().replace(str(old_games), '${GAMES}'))
    for profile in old_games.glob('*/vetust.conf'):
        target = stage / 'config/legacy-profiles' / (profile.parent.name + '.conf')
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(profile.read_text().replace(str(old_games), '${GAMES}'))
    dump(stage / 'config/storage.json', {'library': str(library), 'games': str(games), 'previousGameRoots': [str(old_games)]})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--dest', type=Path, default=Path.home() / 'vetustware')
    parser.add_argument('--migrate', type=Path)
    parser.add_argument('--library', type=Path)
    parser.add_argument('--games', type=Path)
    parser.add_argument('--desktop', action='store_true')
    args = parser.parse_args()
    source, destination = args.source.resolve(), args.dest.expanduser().absolute()
    if source == destination or destination.is_relative_to(source):
        raise RuntimeError('Elige un destino separado de las fuentes.')
    if running_under(destination) or (args.migrate and running_under(args.migrate.resolve())):
        raise RuntimeError('Cierra VETUSTware y sus emuladores antes de instalar.')
    if args.migrate and destination.exists():
        raise RuntimeError('La migración requiere un destino nuevo; la instalación existente se conserva.')
    if args.migrate and (not args.library or not args.games):
        raise RuntimeError('Para migrar indica --library y --games.')
    if destination.exists() and not (destination / 'vetustware.install').is_file():
        raise RuntimeError('El destino existe y no es una instalación de VETUSTware.')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=destination.parent, prefix='.vetustware-install-') as temporary:
        stage = Path(temporary) / 'app'
        stage.mkdir()
        copy_program(source, stage, bool(args.migrate))
        for name in ('config', 'data', 'cache', 'logs', 'backups'):
            (stage / name).mkdir(exist_ok=True)
        if args.migrate:
            migrate(args.migrate.resolve(), stage, args.library.absolute(), args.games.absolute())
        elif not destination.exists():
            # Se elige un motor del equipo receptor, nunca una ruta del autor del paquete.
            bundled = stage / 'runtime/dosbox-staging-linux-x86_64-0.83.0-7b400/dosbox'
            engine = 'staging' if bundled.is_file() or shutil.which('dosbox-staging') else 'classic' if shutil.which('dosbox') else 'dosbox-x' if shutil.which('dosbox-x') else 'staging'
            dump(stage / 'config/emulation.json', {'global': {'engine': engine, 'options': {engine: {'sdl.fullscreen': 'true'}}}, 'paths': {}})

        if not destination.exists():
            os.rename(stage, destination)
        else:
            backup = destination / 'backups' / (datetime.now().strftime('%Y%m%d_%H%M%S_%f') + '-update')
            backup.mkdir(parents=True)
            # Actualizar solo programa y helpers. Configuración, partidas y herramientas propias se conservan.
            for name in ('vetustware', 'icon.svg', 'helpers', 'translations', 'TRANSLATING.md', 'CUSTOMIZING.md', 'INSTALACION.md', 'INSTALLATION.md', 'LICENSE', 'THIRD_PARTY.md', 'THIRD_PARTY.es.md', 'BUILD_INFO.json'):
                new = stage / name
                if not new.exists():
                    continue
                old = destination / name
                if name == 'translations' and old.is_dir():
                    for translation in old.glob('*.json'):
                        if not (new / translation.name).exists():
                            shutil.copy2(translation, new / translation.name)
                if old.exists():
                    shutil.move(old, backup / name)
                shutil.move(new, old)
            for engine in (stage / 'runtime').iterdir():
                old = destination / 'runtime' / engine.name
                if not old.exists():
                    shutil.move(engine, old)
    if args.desktop:
        applications = Path.home() / '.local/share/applications'
        applications.mkdir(parents=True, exist_ok=True)
        desktop = applications / 'vetust.desktop'
        if desktop.exists():
            saved = destination / 'backups' / (datetime.now().strftime('%Y%m%d_%H%M%S_%f') + '-vetust.desktop')
            shutil.copy2(desktop, saved)
        def quoted(path):
            return '"' + str(path).replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$').replace('%', '%%') + '"'
        desktop.write_text('[Desktop Entry]\nVersion=1.0\nType=Application\nName=VETUSTware\nComment=Biblioteca nativa de juegos MS-DOS\nExec=/usr/bin/env QT_QPA_PLATFORM=xcb ' + quoted(destination / 'vetustware') + '\nIcon=' + str(destination / 'icon.svg') + '\nTerminal=false\nCategories=Game;Emulator;\nStartupNotify=true\nStartupWMClass=VETUST\n')
    print(destination)


if __name__ == '__main__':
    main()
