#!/usr/bin/env python3
"""Indexa ROMs e imágenes de una biblioteca Atari sin modificarla."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
import unicodedata


def title(name):
    value = re.split(r'\s*[\[(]', name, maxsplit=1)[0].strip()
    if value.endswith(', The'):
        value = 'The ' + value[:-5]
    return value or name


def normalized(value):
    return re.sub(r'[^a-z0-9]', '', unicodedata.normalize('NFKD', value).encode('ascii', 'ignore').decode().lower())


def contained(path, root):
    return path.resolve().is_relative_to(root.resolve())


def build(base, stella='', metadata=None):
    base = base.resolve()
    metadata = Path(metadata).resolve() if metadata is not None else base / "metadata" if (base / "metadata").is_dir() else base
    folder = base / 'roms' if (base / 'roms').is_dir() else base
    database = {}
    if stella:
        try:
            with tempfile.TemporaryDirectory(prefix='vetust-stella-') as temporary:
                result = subprocess.run([stella, '-basedir', temporary, '-listrominfo'], capture_output=True, text=True, errors='replace', timeout=30, check=True)
            lines = result.stdout.splitlines()
            header = next(i for i, line in enumerate(lines) if line.startswith('Cart_MD5|'))
            keys = lines[header].split('|')
            database = {row['Cart_MD5']: row for line in lines[header + 1:] if (row := dict(zip(keys, line.split('|')))).get('Cart_MD5')}
        except (OSError, subprocess.SubprocessError, StopIteration):
            pass
    images = sorted(p for p in (metadata / 'snapshots').rglob('*') if p.is_file() and p.suffix.lower() in {'.png', '.jpg', '.jpeg'} and contained(p, metadata))
    exact, by_title = {}, {}
    for image in images:
        exact.setdefault(image.stem.casefold(), image)
        by_title.setdefault(normalized(title(image.stem)), image)
    variants = {}
    rom_files = sorted(p for p in folder.rglob('*') if p.is_file() and p.suffix.lower() in {'.bin', '.a26'} and contained(p, base) and 'legacy' not in p.relative_to(base).parts)
    for rom in rom_files:
        if rom.stat().st_size > 16 * 1024 * 1024:
            continue
        data = rom.read_bytes()
        md5 = hashlib.md5(data).hexdigest()
        relative = rom.relative_to(base).as_posix()
        if md5 in variants:
            variants[md5]['files'].append(relative)
            continue
        props = database.get(md5, {})
        name = title(rom.stem)
        image = exact.get(rom.stem.casefold()) or exact.get(props.get('Cart_Name', '').casefold()) or by_title.get(normalized(name))
        year = re.search(r'\b(?:19|20)\d{2}\b', rom.stem)
        tags = re.findall(r'\(([^()]*)\)', rom.stem)
        manufacturer = props.get('Cart_Manufacturer') or next((t for t in tags if not re.fullmatch(r'\d{4}', t) and t not in {'PAL', 'NTSC', 'Unknown', 'Prototype'}), '')
        variants[md5] = dict(id=md5, title=rom.stem, name=name, files=[relative], image=image.relative_to(metadata).as_posix() if image else '', year=year.group() if year else '', manufacturer=manufacturer, controller=props.get('Controller_Left', 'AUTO'), format=props.get('Display_Format', 'AUTO'), note=props.get('Cart_Note', ''), recognized=bool(props))
    groups = {}
    for variant in variants.values():
        key = normalized(variant['name'])
        group = groups.setdefault(key, dict(id=hashlib.sha256(key.encode()).hexdigest()[:24], title=variant['name'], variants=[]))
        group['variants'].append(variant)
    games = sorted(groups.values(), key=lambda g: g['title'].casefold())
    for game in games:
        game['variants'].sort(key=lambda v: ('(PAL)' in v['title'], 'Hack' in v['title'], v['title'].casefold()))
    mapping = {}
    if (metadata / 'covers.json').is_file():
        try:
            document = json.loads((metadata / 'covers.json').read_text(encoding='utf-8'))
            if document.get('format') == 1 and isinstance(document.get('variants'), dict):
                mapping = document['variants']
        except (OSError, ValueError, AttributeError):
            pass
    for game in games:
        for variant in game['variants']:
            relative = mapping.get(variant['id'], '')
            cover = metadata / relative if isinstance(relative, str) else metadata
            variant['cover'] = relative if isinstance(relative, str) and relative and not Path(relative).is_absolute() and cover.is_file() and contained(cover, metadata) else ''
    return dict(format=1, romFiles=len(rom_files), uniqueRoms=len(variants), games=games)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--stella', default='')
    parser.add_argument('--metadata', type=Path)
    args = parser.parse_args()
    if not args.library.is_dir():
        parser.error('La carpeta no está disponible.')
    data = build(args.library, args.stella, args.metadata)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f"{len(data['games'])} titles / {data['uniqueRoms']} ROMs")


if __name__ == '__main__':
    main()
