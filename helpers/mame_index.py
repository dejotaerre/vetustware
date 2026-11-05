#!/usr/bin/env python3
"""Indexa y audita una colección con su motor MAME 0.241, sin modificar ROMs."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

VERSION = '0.241'


def check_version(executable):
    result = subprocess.run([str(executable), '-help'], capture_output=True, text=True, timeout=30)
    if result.returncode or not re.search(r'\bMAME v?0\.241(?:\s|\()', result.stdout):
        raise ValueError('This library requires MAME 0.241.')


def machines(executable):
    process = subprocess.Popen([str(executable), '-listxml'], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    result = {}
    try:
        context = ET.iterparse(process.stdout, events=('start', 'end'))
        _, root = next(context)
        for event, e in context:
            if event != 'end' or e.tag != 'machine':
                continue
            if e.get('isdevice') != 'yes' and e.get('isbios') != 'yes' and e.get('runnable') != 'no':
                driver = e.find('driver'); inp = e.find('input'); display = e.find('display')
                result[e.get('name')] = dict(
                    id=e.get('name'), parent=e.get('cloneof', ''), romof=e.get('romof', ''),
                    title=e.findtext('description', e.get('name')), year=e.findtext('year', ''),
                    manufacturer=e.findtext('manufacturer', ''),
                    status=driver.get('status', 'preliminary') if driver is not None else 'preliminary',
                    savestate=driver.get('savestate') == 'supported' if driver is not None else False,
                    players=inp.get('players', '1') if inp is not None else '1',
                    controls=', '.join(sorted({c.get('type', '') for c in e.findall('input/control')})),
                    rotation=display.get('rotate', '0') if display is not None else '0')
            e.clear(); root.clear()
    finally:
        process.stdout.close()
        if process.wait() != 0:
            raise ValueError('Could not read the MAME catalog.')
    return result


def candidates(data, archives):
    def available(key, seen):
        if key in archives:
            return True
        if key in seen or key not in data:
            return False
        seen.add(key)
        return any(available(data[key].get(field, ''), seen) for field in ('parent', 'romof'))
    return {key: row for key, row in data.items() if available(key, set())}


def audit(executable, library, names, logfile):
    result = {}
    with Path(logfile).open('w') as log:
        for start in range(0, len(names), 160):
            batch = names[start:start + 160]
            run = subprocess.run([str(executable), '-rompath', f'{library}/roms;{library}/bios', '-verifyroms', *batch],
                                 capture_output=True, text=True, timeout=600)
            text = run.stdout + run.stderr
            log.write(text); log.flush()
            if run.returncode not in (0, 2):
                raise ValueError('MAME audit failed: ' + text[-1200:])
            for line in text.splitlines():
                match = re.match(r'romset (\S+)(?: \[[^]]+\])? is (good|best available|bad)', line)
                if match:
                    result[match[1]] = match[2]
                match = re.match(r'romset (\S+)(?: \[[^]]+\])? (?:not found|is missing)', line)
                if match:
                    result[match[1]] = 'missing'
            print(f'{min(start + len(batch), len(names))}/{len(names)}', flush=True)
    return {name: result.get(name, 'missing') for name in names}


def categories(library):
    result = {}; section = ''
    path = library / 'folders/catver.ini'
    if path.is_file():
        for line in path.read_text(errors='replace').splitlines():
            line = line.strip()
            if line.startswith('['): section = line
            elif section == '[Category]' and '=' in line:
                key, value = line.split('=', 1); result[key] = value
    return result


def histories(library, names):
    path = library / 'history/history.dat'
    if not path.is_file(): path = library / 'dats/history.dat'
    result = {}; selected = []; lines = []; bio = False
    if not path.is_file(): return result
    for line in path.read_text(errors='replace').splitlines():
        if line.startswith('$info='):
            selected = [x for x in line[6:].strip(',').split(',') if x in names]; lines = []; bio = False
        elif line == '$bio': bio = True
        elif line == '$end':
            for key in selected: result[key] = '\n'.join(lines).strip()
            selected = []; bio = False
        elif selected and bio: lines.append(line)
    return result


def make_catalog(data, audits, library):
    cats = categories(library); stories = histories(library, set(data)); grouped = {}
    media = {folder: {p.stem: str(p.relative_to(library)) for p in (library / folder).glob('*.png')} for folder in ['flyers', 'snap', 'cabinets', 'marquees', 'titles', 'cpanel']}
    for key, original in data.items():
        row = dict(original); parent = row['parent']; group = parent if parent in data else key
        while group in data and data[group]['parent'] in data:
            group = data[group]['parent']
        row['audit'] = audits.get(key, 'missing')
        row['playable'] = row['audit'] in ('good', 'best available')
        row['category'] = cats.get(key, cats.get(group, ''))
        row['history'] = stories.get(key, stories.get(group, ''))
        for field, folder in [('cover', 'flyers'), ('image', 'snap'), ('cabinet', 'cabinets'), ('marquee', 'marquees'), ('titleImage', 'titles'), ('panel', 'cpanel')]:
            row[field] = media[folder].get(key, media[folder].get(group, ''))
        grouped.setdefault(group, []).append(row)
    games = []
    for key, variants in grouped.items():
        variants.sort(key=lambda v: (not v['playable'], v['status'] == 'preliminary', v['id'] != key, v['title']))
        games.append({'id': key, 'title': data.get(key, variants[0])['title'], 'variants': variants})
    return {'format': 1, 'engine': VERSION, 'games': sorted(games, key=lambda g: g['title'].casefold()),
            'variants': len(data), 'verified': sum(v in ('good', 'best available') for v in audits.values()),
            'categorySource': 'Local catver.ini; older categories may be incomplete'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--mame', type=Path, required=True)
    parser.add_argument('--metadata', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); library = args.library.resolve()
    if not (library / 'roms').is_dir(): raise ValueError('Select a MAME library containing a roms folder.')
    check_version(args.mame)
    data = machines(args.mame)
    archives = {p.stem for p in (library / 'roms').iterdir() if p.suffix.lower() in ('.zip', '.7z') or p.is_dir()}
    data = candidates(data, archives)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    audits = audit(args.mame, library, sorted(data), str(args.output) + '.audit.log')
    catalog = make_catalog(data, audits, args.metadata or (library / "metadata" if (library / "metadata").is_dir() else library))
    with tempfile.NamedTemporaryFile(mode='w', dir=args.output.parent, delete=False, encoding='utf-8') as f:
        json.dump(catalog, f, ensure_ascii=False); temporary = f.name
    os.replace(temporary, args.output)
    print(json.dumps({key: catalog[key] for key in ('variants', 'verified')}))


if __name__ == '__main__':
    main()
