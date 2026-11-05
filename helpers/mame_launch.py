#!/usr/bin/env python3
"""Lanza MAME fijado a 0.241, con datos personales separados y ROMs auditadas."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
from mame_index import check_version


def seed(library, personal):
    # Importar solo una vez cada archivo; nunca sobrescribir ajustes o partidas actuales.
    for folder in ('cfg', 'nvram', 'sta', 'hi', 'inp', 'diff', 'comments'):
        target = personal / folder; target.mkdir(parents=True, exist_ok=True)
        source = library / folder
        if not source.is_dir(): continue
        for file in source.rglob('*'):
            if not file.is_file() or file.is_symlink(): continue
            dest = target / file.relative_to(source)
            if dest.exists(): continue
            dest.parent.mkdir(parents=True, exist_ok=True)
            with file.open('rb') as src:
                try:
                    with dest.open('xb') as out: shutil.copyfileobj(src, out)
                except FileExistsError: pass
    for folder in ('snap', 'ini'): (personal / folder).mkdir(exist_ok=True)


def arguments(library, personal, game='', fullscreen=True, screen='', artwork=True, metadata=None):
    metadata = metadata if metadata is not None else library
    args = ['-rompath', f'{library}/roms;{library}/bios', '-samplepath', str(library / 'samples'),
            '-artpath', str(metadata / 'artwork'), '-homepath', str(personal), '-inipath', str(personal / 'ini'),
            '-cfg_directory', str(personal / 'cfg'), '-nvram_directory', str(personal / 'nvram'),
            '-state_directory', str(personal / 'sta'), '-diff_directory', str(personal / 'diff'),
            '-input_directory', str(personal / 'inp'), '-snapshot_directory', str(personal / 'snap'),
            '-comment_directory', str(personal / 'comments'), '-video', 'opengl',
            '-window' if not fullscreen else '-nowindow', '-keepaspect', '-unevenstretch',
            '-artwork_crop' if not artwork else '-noartwork_crop', '-skip_gameinfo', '-joystick']
    if screen: args += ['-screen', screen]
    if game: args += [game]
    return args


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mame', type=Path, required=True)
    p.add_argument('--library', type=Path, required=True)
    p.add_argument('--personal', type=Path, required=True)
    p.add_argument('--metadata', type=Path)
    p.add_argument('--game', default='')
    p.add_argument('--window', action='store_true')
    p.add_argument('--screen', default='')
    p.add_argument('--crop', action='store_true')
    args = p.parse_args()
    if args.game and not re.fullmatch(r'[a-z0-9_]+', args.game): raise ValueError('Invalid game identifier')
    check_version(args.mame)
    if not (args.library / 'roms').is_dir(): raise ValueError('MAME library is unavailable')
    if args.game:
        audit = subprocess.run([str(args.mame), '-rompath', f'{args.library}/roms;{args.library}/bios', '-verifyroms', args.game], capture_output=True, text=True, timeout=180)
        print(audit.stdout + audit.stderr, flush=True)
        if audit.returncode: raise SystemExit(audit.returncode)
    seed(args.library, args.personal)
    os.chdir(args.personal)
    os.execv(str(args.mame), [str(args.mame), *arguments(args.library, args.personal, args.game, not args.window, args.screen, not args.crop, args.metadata)])


if __name__ == '__main__':
    main()
