"""Rutas internacionales y compatibilidad con bibliotecas anteriores."""
from pathlib import Path


def organized(root):
    return any((Path(root) / name).is_file() for name in ('library.json', 'biblioteca.json'))


def resource(root, current, legacy):
    root = Path(root)
    parts = Path(current).parts
    if len(parts) >= 2 and parts[0] == 'metadata':
        nested = root / 'games' / parts[1] / 'metadata'
        if nested.is_dir():
            return nested.joinpath(*parts[2:])
    if len(parts) >= 2 and parts[0] == 'resources':
        nested = root / 'games' / ('mame' if parts[1] == 'mame' else 'dosbox') / 'resources'
        if nested.is_dir():
            return nested.joinpath(*parts[2 if parts[1] == 'mame' else 1:])
    if (root / 'library.json').is_file():
        return root / current
    if not organized(root):
        return root / legacy
    folders = {'games': 'juegos', 'metadata': 'metadatos', 'resources': 'recursos',
               'magazines': 'revistas', 'archives': 'archivos', 'extras': 'complementos',
               'editions': 'ediciones'}
    return root.joinpath(*(folders.get(part, part) for part in Path(current).parts))
