#!/usr/bin/env python3
"""Prepara runtimes privados desde paquetes nativos de Arch/CachyOS verificados localmente."""
import argparse
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tarfile

from runtime_manager import REPO, MANIFEST, sha, write, probe, version, verify

# Glibc y los controladores gráficos pertenecen al sistema receptor.
SYSTEM_LIB = re.compile(r'^(?:ld-linux|lib(?:c|m|mvec|pthread|dl|rt|resolv|nss_\w+|util)\.so|lib(?:GL|EGL|GLX|GLdispatch|drm|vulkan))')
V3_FLAGS = ['avx', 'avx2', 'bmi1', 'bmi2', 'f16c', 'fma', 'movbe', 'xsave', 'ssse3', 'sse4_1', 'sse4_2', 'popcnt', 'cx16', 'lahf_lm']


def run(args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT, timeout=30)


def launcher(engine):
    extra = '' if engine == 'stella' else ' --extrapath="$base/share/scummvm" --themepath="$base/share/scummvm"'
    return '#!/bin/sh\nbase=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\nexport LD_LIBRARY_PATH="$base/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$base/bin/' + engine + '"' + extra + ' "$@"\n'


def prepare_system(engine, destination):
    system = platform.freedesktop_os_release()
    if 'arch' not in {system.get('ID'), *system.get('ID_LIKE', '').split()} or platform.machine() != 'x86_64':
        raise ValueError('System import currently supports Arch/CachyOS x86_64')
    for command in ('pacman', 'ldd', 'readelf'):
        if not shutil.which(command):
            raise ValueError('Required system tool is missing: ' + command)
    binary = Path('/usr/bin') / engine
    if not binary.is_file():
        raise ValueError('System emulator is not installed')
    package_version = run(['pacman', '-Q', engine]).split()[1]
    number = package_version.split('-')[0].split(':')[-1]
    version(number)
    destination.mkdir(parents=True)
    (destination / 'bin').mkdir()
    (destination / 'lib').mkdir()
    shutil.copy2(binary, destination / 'bin' / engine)
    libraries = {}
    queue = [binary]
    # SDL2-compat carga SDL3 dinámicamente y ldd no siempre la muestra.
    if Path('/usr/lib/libSDL3.so.0').exists():
        libraries['libSDL3.so.0'] = Path('/usr/lib/libSDL3.so.0')
        queue.append(Path('/usr/lib/libSDL3.so.0'))
    visited = set()
    while queue:
        file = queue.pop()
        if file.resolve() in visited:
            continue
        visited.add(file.resolve())
        report = run(['ldd', str(file)])
        if 'not found' in report:
            raise ValueError('The installed emulator has missing dependencies')
        for name, path in re.findall(r'^\s*(\S+) => (/\S+) ', report, re.M):
            name = Path(name).name
            if SYSTEM_LIB.match(name):
                continue
            if name not in libraries:
                libraries[name] = Path(path)
                queue.append(Path(path))
    packages = {engine}
    needs_v3 = False
    needs_v4 = False
    for name, file in [(engine, binary), *libraries.items()]:
        notes = run(['readelf', '-n', str(file)])
        owner = run(['pacman', '-Qoq', str(file.resolve())]).strip().splitlines()[0]
        packages.add(owner)
        information = subprocess.check_output(['pacman', '-Qi', owner], text=True, env=dict(os.environ, LC_ALL='C'))
        architecture = re.search(r'^Architecture\s*:\s*(\S+)', information, re.M).group(1)
        needed = next((line for line in notes.splitlines() if 'ISA needed:' in line), '')
        needs_v3 |= architecture in ('x86_64_v3', 'x86_64_v4') or 'x86-64-v3' in needed
        needs_v4 |= architecture == 'x86_64_v4' or 'x86-64-v4' in needed
        if name != engine:
            shutil.copy2(file, destination / 'lib' / name)
    # Solo recursos pertenecientes a paquetes: no copiar añadidos personales de /usr/share.
    provenance = []
    for package in sorted(packages):
        pkg_version = run(['pacman', '-Q', package]).strip().split()[1]
        provenance.append({'package': package, 'version': pkg_version})
        for name in run(['pacman', '-Qlq', package]).splitlines():
            file = Path(name)
            if not file.is_file():
                continue
            resource = package == engine and name.startswith((f'/usr/share/{engine}/', f'/usr/share/doc/{engine}/'))
            license_file = name.startswith('/usr/share/licenses/')
            if resource or license_file:
                target = destination / name.removeprefix('/usr/')
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(file, target)
    common = Path('/usr/share/licenses/common')
    if common.exists():
        shutil.copytree(common, destination / 'share/licenses/common', symlinks=False)
    wrapper = launcher(engine)
    (destination / engine).write_text(wrapper)
    (destination / engine).chmod(0o755)
    write(destination / 'PROVENANCE.json', {'origin': 'installed native packages', 'packages': provenance,
          'system': system.get('PRETTY_NAME'), 'upstream': 'https://stella-emu.github.io/' if engine == 'stella' else 'https://www.scummvm.org/'})
    (destination / 'README.txt').write_text('Private runtime for VETUSTware. Configurations and saves are kept outside this folder.\n'
        'Built from native Arch/CachyOS packages; not a universal Linux binary.\n'
        'System glibc, graphics drivers and desktop/audio services are still required.\n'
        'Before redistribution, provide corresponding sources and license notices for every package in PROVENANCE.json.\n')
    meta = {'schema': 1, 'engine': engine, 'version': number, 'revision': 1,
            'compatibility': {'arch': 'x86_64', 'distribution': 'arch', 'glibc': platform.libc_ver()[1], 'cpu_flags': (V3_FLAGS if needs_v3 or needs_v4 else []) + (['avx512f', 'avx512bw', 'avx512cd', 'avx512dq', 'avx512vl'] if needs_v4 else [])},
            'files': {str(p.relative_to(destination)): sha(p) for p in destination.rglob('*') if p.is_file()}}
    write(destination / 'runtime.json', meta)
    probe(destination, engine)
    return meta


def pack(folder, output, tag):
    meta = json.loads((folder / 'runtime.json').read_text())
    name = f"vetustware-{meta['engine']}-{meta['version']}-r{meta['revision']}-linux-x86_64.tar.gz"
    verify(folder, meta['engine'])
    archive = output / name
    if archive.exists():
        raise ValueError('Package already exists')
    with tarfile.open(archive, 'w:gz') as stream:
        for path in sorted(folder.rglob('*')):
            if path.is_file():
                info = stream.gettarinfo(path, str(path.relative_to(folder)))
                info.uid = info.gid = 0
                info.uname = info.gname = ''
                with path.open('rb') as source:
                    stream.addfile(info, source)
    return {k: meta[k] for k in ('engine', 'version', 'revision', 'compatibility')} | {
        'url': f'https://github.com/{REPO}/releases/download/{tag}/{name}', 'sha256': sha(archive), 'size': archive.stat().st_size}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', choices=['stella', 'scummvm'], required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--package-dir', type=Path)
    parser.add_argument('--release-tag')
    args = parser.parse_args()
    if args.package_dir and (not args.release_tag or not re.fullmatch(r'[A-Za-z0-9._-]+', args.release_tag)):
        parser.error('--package-dir requires a valid --release-tag')
    prepare_system(args.engine, args.output.absolute())
    if args.package_dir:
        args.package_dir.mkdir(parents=True, exist_ok=True)
        entry = pack(args.output.absolute(), args.package_dir, args.release_tag)
        manifest = args.package_dir / MANIFEST
        data = json.loads(manifest.read_text()) if manifest.exists() else {'schema': 1, 'packages': []}
        data['packages'] = [p for p in data['packages'] if p['engine'] != args.engine] + [entry]
        write(manifest, data)


if __name__ == '__main__':
    main()
