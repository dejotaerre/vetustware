#!/usr/bin/env python3
"""Importa metadatos de una colección eXoDOS proporcionada por el usuario."""
import argparse
import json
import os
from pathlib import Path
import re
import tempfile
from zipfile import ZipFile
from catalog_index import build
from library_layout import resource


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--library', type=Path, default=Path())
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--dosbox', type=Path)
    parser.add_argument('--metadata', type=Path)
    parser.add_argument('--dosbox-metadata', type=Path)
    args = parser.parse_args()
    dos_metadata = args.dosbox_metadata or (args.metadata / "dosbox" if args.metadata else None)
    if dos_metadata is None and args.dosbox and (args.dosbox / "metadata").is_dir():
        dos_metadata = args.dosbox / "metadata"
    args.data.mkdir(parents=True, exist_ok=True)
    # Los índices existentes nunca se sustituyen durante el asistente inicial.
    if (args.data / 'catalog.json').exists() or (args.data / 'original-profiles.json').exists():
        raise RuntimeError('Ya existen índices. Se conservan sin sobrescribir.')
    with tempfile.TemporaryDirectory(dir=args.data, prefix='.import-') as temporary:
        output = Path(temporary)
        custom = dos_metadata / 'perfiles.zip' if dos_metadata else resource(args.library, 'metadata/dosbox/perfiles.zip', 'Personalizaciones/perfiles.zip')
        if custom.is_file():
            with ZipFile(custom) as archive:
                for name in ('catalog.json', 'original-profiles.json', 'spanish-profiles.json'):
                    document = json.loads(archive.read(name))
                    if not isinstance(document, dict):
                        raise RuntimeError('Metadatos personalizados inválidos: ' + name)
                    (output / name).write_text(json.dumps(document, ensure_ascii=False))
            catalog = json.loads((output / 'catalog.json').read_text())
        else:
            catalog = build(args.library, output, args.metadata, dos_metadata)
            archives = {p.name.casefold(): p.name for p in (args.dosbox / 'archives' if args.dosbox else resource(args.library, 'games/dosbox/archives', 'eXo/eXoDOS')).glob('*.zip')}
            profiles = {}
            with ZipFile(dos_metadata / 'perfiles-originales.zip' if dos_metadata else resource(args.library, 'metadata/dosbox/perfiles-originales.zip', 'Content/!DOSmetadata_linux.zip')) as archive:
                names = {n.casefold(): n for n in archive.namelist()}
                for game in catalog['games']:
                    prefix = 'eXo/eXoDOS/!dos/' + game['folder'] + '/'
                    config = names.get((prefix + 'dosbox_linux.conf').casefold())
                    profiles[game['id']] = dict(config=archive.read(config).decode(errors='replace') if config else '', source=config or '', archive=archives.get(game['archive'].casefold(), ''), tested=False)
            (output / 'original-profiles.json').write_text(json.dumps(profiles, ensure_ascii=False))
        # Primero los perfiles; el catálogo actúa como índice visible al abrir la aplicación.
        for name in ('original-profiles.json', 'spanish-profiles.json', 'catalog.json'):
            if (output / name).exists():
                os.replace(output / name, args.data / name)
    print(f"{len(catalog['games'])} juegos importados")


if __name__ == '__main__':
    main()
