#!/usr/bin/env python3
"""Gestiona runtimes privados, paquetes verificados y restauración sin tocar partidas."""
import argparse
from contextlib import contextmanager
import fcntl
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile
import time
import urllib.parse
import urllib.request

ENGINES = ('stella', 'scummvm')
REPO = 'dejotaerre/vetustware'
API = f'https://api.github.com/repos/{REPO}/releases?per_page=50'
MANIFEST = 'vetustware-runtimes-v1.json'
LIMIT = 1024 * 1024 * 1024


def read(path, fallback=None):
    return json.loads(path.read_text()) if path.is_file() else ({} if fallback is None else fallback)


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n')
    os.replace(temporary, path)


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def version(value):
    if not isinstance(value, str) or not re.fullmatch(r'\d+(?:\.\d+){1,3}', value):
        raise ValueError('Invalid runtime version')
    return tuple(map(int, value.split('.')))


def compatible(meta):
    try:
        required = meta['compatibility']
        system = platform.freedesktop_os_release()
        ids = {system.get('ID', ''), *system.get('ID_LIKE', '').split()}
        flags = set(Path('/proc/cpuinfo').read_text().split())
        return (platform.system() == 'Linux' and required['arch'] == platform.machine()
                and (not required.get('distribution') or required['distribution'] in ids)
                and version(platform.libc_ver()[1]) >= version(required['glibc'])
                and set(required.get('cpu_flags', [])).issubset(flags))
    except (KeyError, ValueError, OSError):
        return False


def trusted_url(url):
    p = urllib.parse.urlsplit(url)
    return (p.scheme == 'https' and not p.username and not p.password and p.port in (None, 443)
            and p.hostname == 'github.com' and p.path.startswith(f'/{REPO}/releases/download/'))


class SecureRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        if urllib.parse.urlsplit(newurl).scheme != 'https':
            raise ValueError('Insecure download redirect')
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def download(url, destination, maximum, expected=None):
    if url != API and not trusted_url(url):
        raise ValueError('Untrusted update URL')
    request = urllib.request.Request(url, headers={'User-Agent': 'VETUSTware-runtime-manager', 'Accept': 'application/vnd.github+json' if url == API else 'application/octet-stream'})
    opener = urllib.request.build_opener(SecureRedirect())
    count = 0
    with opener.open(request, timeout=30) as response, destination.open('wb') as output:
        while chunk := response.read(1024 * 1024):
            count += len(chunk)
            if count > maximum:
                raise ValueError('Download is too large')
            output.write(chunk)
    if expected and sha(destination) != expected:
        raise ValueError('Downloaded package checksum mismatch')


def validate_offer(item):
    if item.get('engine') not in ENGINES or not trusted_url(item.get('url', '')):
        raise ValueError('Invalid runtime offer')
    version(item.get('version'))
    if type(item.get('revision')) is not int or item['revision'] < 1:
        raise ValueError('Invalid runtime revision')
    if not re.fullmatch('[0-9a-f]{64}', item.get('sha256', '')):
        raise ValueError('Missing package checksum')
    if type(item.get('size')) is not int or not 0 < item['size'] <= LIMIT:
        raise ValueError('Invalid package size')
    if not isinstance(item.get('compatibility'), dict):
        raise ValueError('Missing compatibility information')
    return item


def check(root):
    offers = []
    with tempfile.TemporaryDirectory(dir=root / 'cache') as temporary:
        folder = Path(temporary)
        download(API, folder / 'releases.json', 8 * 1024 * 1024)
        for release in read(folder / 'releases.json', []):
            if release.get('draft') or release.get('prerelease'):
                continue
            for asset in release.get('assets', []):
                if asset.get('name') != MANIFEST:
                    continue
                download(asset['browser_download_url'], folder / 'manifest.json', 1024 * 1024)
                manifest = read(folder / 'manifest.json')
                if manifest.get('schema') != 1:
                    raise ValueError('Unsupported update manifest')
                for item in manifest.get('packages', []):
                    validate_offer(item)
                    if compatible(item):
                        offers.append(item)
    latest = {}
    for item in offers:
        key = item['engine']
        if key not in latest or (version(item['version']), item['revision']) > (version(latest[key]['version']), latest[key]['revision']):
            latest[key] = item
    write(root / 'cache/runtime-updates.json', {'checked': int(time.time()), 'packages': latest})


def status(root):
    catalog = read(root / 'cache/runtime-updates.json')
    result = []
    for engine in ENGINES:
        current = read(root / 'runtime' / engine / 'runtime.json')
        previous = read(root / 'runtime/.previous' / engine / 'runtime.json')
        candidate = catalog.get('packages', {}).get(engine, {})
        installed = os.access(root / 'runtime' / engine / engine, os.X_OK)
        update = bool(candidate and compatible(candidate) and (not current or (version(candidate['version']), candidate['revision']) > (version(current['version']), current['revision'])))
        result.append({'id': engine, 'name': 'Stella' if engine == 'stella' else 'ScummVM',
                       'checked': bool(catalog.get('checked')), 'installed': installed, 'version': current.get('version', ''),
                       'availableVersion': candidate.get('version', ''), 'update': update,
                       'rollback': bool(previous), 'previousVersion': previous.get('version', ''),
                       'systemAvailable': bool(shutil.which(engine)),
                       'path': str(root / 'runtime' / engine / engine),
                       'compatibility': current.get('compatibility', {})})
    return {'engines': result, 'checked': catalog.get('checked', 0)}


def safe_name(name):
    p = PurePosixPath(name)
    if not name or p.is_absolute() or '..' in p.parts or '\\' in name or '\x00' in name:
        raise ValueError('Unsafe archive path')
    return p


def unpack(archive, destination):
    total = 0
    seen = set()
    with tarfile.open(archive, 'r:gz') as package:
        for member in package:
            name = str(safe_name(member.name))
            if name in seen or not (member.isdir() or member.isfile()):
                raise ValueError('Duplicate entry, link or special file in package')
            seen.add(name)
            total += member.size
            if total > 2 * LIMIT or len(seen) > 20000:
                raise ValueError('Expanded package is too large')
            target = destination / name
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with package.extractfile(member) as source, target.open('wb') as output:
                    shutil.copyfileobj(source, output)
                target.chmod(0o755 if member.mode & 0o111 else 0o644)


def probe(folder, engine):
    executable = folder / engine
    if not executable.is_file() or not os.access(executable, os.X_OK):
        raise ValueError('Runtime executable is missing')
    environment = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    environment.pop('LD_PRELOAD', None)
    with tempfile.TemporaryDirectory() as home:
        environment.update(HOME=home, XDG_CONFIG_HOME=home, XDG_DATA_HOME=home, XDG_CACHE_HOME=home)
        result = subprocess.run([str(executable), '-help' if engine == 'stella' else '--version'], env=environment,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=25)
    if result.returncode or engine.lower() not in result.stdout.decode(errors='replace').lower():
        raise ValueError('Runtime startup check failed: ' + result.stdout.decode(errors='replace')[-1500:])


def verify(folder, engine, offer=None):
    meta = read(folder / 'runtime.json')
    if meta.get('schema') != 1 or meta.get('engine') != engine or not compatible(meta):
        raise ValueError('Runtime is not compatible with this computer')
    version(meta.get('version'))
    if offer and any(meta.get(k) != offer.get(k) for k in ('engine', 'version', 'revision', 'compatibility')):
        raise ValueError('Package metadata does not match the update')
    expected = meta.get('files', {})
    actual = {str(p.relative_to(folder)) for p in folder.rglob('*') if p.is_file() and p != folder / 'runtime.json'}
    if not expected or actual != set(expected):
        raise ValueError('Incomplete runtime manifest')
    for name, digest in expected.items():
        safe_name(name)
        file = folder / name
        if file.is_symlink() or sha(file) != digest:
            raise ValueError('Runtime file checksum mismatch: ' + name)
    probe(folder, engine)
    return meta


def ensure_idle(root):
    runtime = (root / 'runtime').resolve()
    for entry in Path('/proc').iterdir():
        if entry.name.isdigit():
            try:
                executable = (entry / 'exe').resolve()
                if executable.is_relative_to(runtime):
                    raise RuntimeError('Close the emulator before changing its runtime')
            except (OSError, RuntimeError) as error:
                if isinstance(error, RuntimeError) and str(error).startswith('Close'):
                    raise


@contextmanager
def lock(root):
    (root / 'runtime').mkdir(parents=True, exist_ok=True)
    (root / 'cache').mkdir(parents=True, exist_ok=True)
    with (root / 'runtime/.update.lock').open('a') as stream:
        fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        recover(root)
        yield


def recover(root):
    journal = root / 'runtime/.transaction.json'
    if not journal.exists():
        return
    data = read(journal)
    engine = data.get('engine')
    if engine not in ENGINES:
        raise ValueError('Invalid runtime recovery record')
    current = root / 'runtime' / engine
    saved = root / 'runtime/.previous' / engine
    pending = root / 'runtime/.swap' / engine
    # Una interrupción conserva siempre al menos una copia utilizable.
    if not current.exists():
        if pending.exists():
            pending.rename(current)
        elif saved.exists():
            saved.rename(current)
    if pending.exists() and not saved.exists():
        pending.rename(saved)
    journal.unlink()


def activate(root, engine, prepared):
    ensure_idle(root)
    current = root / 'runtime' / engine
    previous = root / 'runtime/.previous' / engine
    previous.parent.mkdir(exist_ok=True)
    # El respaldo anterior se archiva, nunca se destruye al instalar otra versión.
    if previous.exists():
        archive = root / 'runtime/.history' / (engine + '-' + str(time.time_ns()))
        archive.parent.mkdir(exist_ok=True)
        previous.rename(archive)
    journal = root / 'runtime/.transaction.json'
    write(journal, {'engine': engine})
    try:
        if current.exists():
            current.rename(previous)
        prepared.rename(current)
        journal.unlink()
    except BaseException:
        recover(root)
        raise


def install(root, engine):
    # Volver a consultar evita instalar una oferta almacenada y ya retirada.
    check(root)
    row = next(item for item in status(root)['engines'] if item['id'] == engine)
    if not row['update']:
        raise ValueError('No newer compatible runtime is published')
    item = validate_offer(read(root / 'cache/runtime-updates.json')['packages'][engine])
    with tempfile.TemporaryDirectory(prefix='.download-', dir=root / 'runtime') as temporary:
        folder = Path(temporary)
        archive = folder / 'package.tar.gz'
        download(item['url'], archive, item['size'], item['sha256'])
        if archive.stat().st_size != item['size']:
            raise ValueError('Package size mismatch')
        prepared = folder / 'prepared'
        prepared.mkdir()
        unpack(archive, prepared)
        verify(prepared, engine, item)
        activate(root, engine, prepared)


def rollback(root, engine):
    ensure_idle(root)
    current = root / 'runtime' / engine
    previous = root / 'runtime/.previous' / engine
    verify(previous, engine)
    swap = root / 'runtime/.swap' / engine
    swap.parent.mkdir(exist_ok=True)
    journal = root / 'runtime/.transaction.json'
    write(journal, {'engine': engine})
    try:
        current.rename(swap)
        previous.rename(current)
        swap.rename(previous)
        journal.unlink()
    except BaseException:
        recover(root)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('action', choices=['status', 'check', 'install', 'rollback', 'import-system'])
    parser.add_argument('--engine', choices=ENGINES)
    args = parser.parse_args()
    root = args.root.absolute()
    try:
        with lock(root):
            if args.action in ('install', 'rollback', 'import-system'):
                if not args.engine:
                    raise ValueError('Choose an emulator')
                ensure_idle(root)
            if args.action == 'check':
                check(root)
            elif args.action == 'install':
                install(root, args.engine)
            elif args.action == 'rollback':
                rollback(root, args.engine)
            elif args.action == 'import-system':
                from runtime_package import prepare_system
                with tempfile.TemporaryDirectory(prefix='.import-', dir=root / 'runtime') as temporary:
                    prepared = Path(temporary) / 'prepared'
                    prepare_system(args.engine, prepared)
                    verify(prepared, args.engine)
                    activate(root, args.engine, prepared)
            print(json.dumps({'ok': True, **status(root)}))
    except Exception as error:
        print(json.dumps({'ok': False, 'error': str(error)}))
        raise SystemExit(1)


if __name__ == '__main__':
    main()
