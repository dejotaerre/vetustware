"""Catálogo e imágenes derivados de la colección, sin modificar sus archivos."""
from collections import defaultdict
from pathlib import Path, PureWindowsPath, PurePosixPath
from zipfile import ZipFile
from library_layout import resource as library_resource
import hashlib
import json
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
LIBRARY = None
METADATA = None
DATA = None

def normalized(value):
    return re.sub(r'''[<>:"/\\|?*']''', '_', value).casefold().strip()

def build(library, destination, metadata=None, dosbox_metadata=None):
    DATA = Path(destination)
    METADATA = Path(dosbox_metadata) / "recursos.zip" if dosbox_metadata else Path(metadata) / "dosbox/recursos.zip" if metadata else library_resource(library, "metadata/dosbox/recursos.zip", "Content/XODOSMetadata.zip")
    DATA.mkdir(parents=True, exist_ok=True)
    with ZipFile(METADATA) as archive:
        root = ET.fromstring(archive.read('xml/MS-DOS.xml'))
        files = {i.filename for i in archive.infolist() if not i.is_dir()}
    shared = Path(dosbox_metadata) / 'recursos-compartidos.json' if dosbox_metadata else Path(metadata) / 'dosbox/recursos-compartidos.json' if metadata else library_resource(library, 'metadata/dosbox/recursos-compartidos.json', 'Content/recursos-compartidos.json')
    shared_names = {}
    if shared.exists():
        document = json.loads(shared.read_text())
        if document.get('format') != 1:
            raise ValueError('Formato de recursos compartidos no admitido')
        for name, resource in document.get('archives', {}).get(METADATA.name, {}).items():
            target = resource['entry']
            if any(PurePosixPath(p).is_absolute() or '..' in PurePosixPath(p).parts or '\\' in p for p in (name, target)) or target not in files:
                raise ValueError('Referencia de recurso compartido inválida: ' + name)
            if name in files and name != target:
                raise ValueError('El recurso compartido todavía tiene una copia independiente: ' + name)
            shared_names[name] = target
        # Los nombres históricos conservan la asociación al juego y los identificadores de imagen.
        files |= shared_names.keys()
    games = [g for g in root.findall('Game') if g.findtext('ApplicationPath')]
    aliases = defaultdict(list)
    for item in root.findall('AlternateName'):
        aliases[item.findtext('GameID')].append(item.findtext('Name') or '')
    owners = defaultdict(set)
    title_owners = defaultdict(set)
    for g in games:
        title_owners[normalized(g.findtext('Title'))].add(g.findtext('ID'))
        for title in [g.findtext('Title'), *aliases[g.findtext('ID')]]:
            owners[normalized(title)].add(g.findtext('ID'))
    media = defaultdict(lambda: defaultdict(list))
    assets = {}
    ids = {g.findtext('ID') for g in games}
    for name in sorted(files):
        parts = name.split('/')
        if len(parts) < 4 or parts[:2] != ['Images', 'MS-DOS']:
            continue
        if Path(name).suffix.lower() not in {'.jpg', '.jpeg', '.png', '.gif', '.bmp', '.webp'}:
            continue
        key = normalized(re.sub(r'-\d+$', '', Path(name).stem))
        suffix = re.search(r'\.([0-9a-f-]{36})$', key)
        base_key = key[:suffix.start()] if suffix else key
        matches = {suffix[1]} if suffix and suffix[1] in ids else title_owners.get(base_key, owners.get(base_key, set()))
        if len(matches) != 1:
            continue
        asset = hashlib.sha256(name.encode()).hexdigest()[:24]
        assets[asset] = shared_names.get(name, name)
        media[next(iter(matches))][parts[2]].append(asset)
    rows = []
    for g in games:
        def val(key): return g.findtext(key) or ''
        gid = val('ID'); images = media[gid]
        path = PureWindowsPath(val('ApplicationPath'))
        cover = images.get('Box - Front', [])
        shots = images.get('Screenshot - Gameplay', [])
        titles = images.get('Screenshot - Game Title', [])
        manual = val('ManualPath').replace('\\', '/')
        manual_found = manual in files
        rows.append(dict(id=gid, title=val('Title'), aliases=aliases[gid], year=val('ReleaseDate')[:4],
                         developer=val('Developer'), publisher=val('Publisher'), genre=val('Genre'),
                         description=val('Notes'), playMode=val('PlayMode'), folder=path.parent.name,
                         archive=path.stem+'.zip', cover=(cover or titles or shots or [None])[0],
                         realCover=bool(cover), shots=shots+titles, box3d=(images.get('Box - 3D') or [None])[0],
                         background=(images.get('Fanart - Background') or [None])[0],
                         manual=shared_names.get(manual, manual) if manual_found else None))
    result = {'games': rows, 'assets': assets}
    (DATA/'catalog.json').write_text(json.dumps(result,ensure_ascii=False))
    return result
