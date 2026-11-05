#!/usr/bin/env python3
"""Pruebas aisladas de actualizaciones, integridad, reversión y extracción segura."""
import copy
import io
import json
from pathlib import Path
import platform
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch

import runtime_manager as manager
from runtime_package import pack, launcher


class RuntimeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        (self.root / 'runtime').mkdir()
        (self.root / 'cache').mkdir()
        (self.root / 'config').mkdir()
        (self.root / 'config/settings.json').write_text('personal')
        (self.root / 'data').mkdir()
        (self.root / 'data/save.dat').write_text('partida')

    def tearDown(self):
        self.assertEqual((self.root / 'config/settings.json').read_text(), 'personal')
        self.assertEqual((self.root / 'data/save.dat').read_text(), 'partida')
        self.temp.cleanup()

    def prepared(self, name, number):
        folder = self.root / name
        folder.mkdir()
        executable = folder / 'stella'
        executable.write_text('#!/bin/sh\nprintf "Stella test\\n"\n')
        executable.chmod(0o755)
        manager.write(folder / 'runtime.json', {'schema': 1, 'engine': 'stella', 'version': number, 'revision': 1,
            'compatibility': {'arch': platform.machine(), 'glibc': '2.0', 'cpu_flags': []},
            'files': {'stella': manager.sha(executable)}})
        return folder

    def test_install_and_rollback_keep_data(self):
        first = self.prepared('first', '1.0')
        manager.verify(first, 'stella')
        manager.activate(self.root, 'stella', first)
        second = self.prepared('second', '2.0')
        manager.activate(self.root, 'stella', second)
        self.assertEqual(manager.status(self.root)['engines'][0]['version'], '2.0')
        manager.rollback(self.root, 'stella')
        self.assertEqual(manager.status(self.root)['engines'][0]['version'], '1.0')
        manager.rollback(self.root, 'stella')
        self.assertEqual(manager.status(self.root)['engines'][0]['version'], '2.0')

    def test_corrupt_runtime_is_not_executed(self):
        folder = self.prepared('invalid', '1.0')
        (folder / 'stella').write_text('#!/bin/sh\ntouch ' + str(self.root / 'executed') + '\n')
        with self.assertRaisesRegex(ValueError, 'checksum'):
            manager.verify(folder, 'stella')
        self.assertFalse((self.root / 'executed').exists())

    def test_wrong_arch_and_cpu_rejected(self):
        folder = self.prepared('wrong', '1.0')
        meta = manager.read(folder / 'runtime.json')
        meta['compatibility']['arch'] = 'wrong'
        self.assertFalse(manager.compatible(meta))
        meta['compatibility']['arch'] = platform.machine()
        meta['compatibility']['cpu_flags'] = ['impossible-cpu-feature']
        self.assertFalse(manager.compatible(meta))

    def test_unsafe_archives(self):
        for name, kind in [('../escape', tarfile.REGTYPE), ('/tmp/escape', tarfile.REGTYPE), ('link', tarfile.SYMTYPE), ('hard', tarfile.LNKTYPE), ('fifo', tarfile.FIFOTYPE)]:
            with self.subTest(name=name):
                archive = self.root / 'bad.tar.gz'
                with tarfile.open(archive, 'w:gz') as stream:
                    info = tarfile.TarInfo(name)
                    info.type = kind
                    info.linkname = '../../escape'
                    stream.addfile(info)
                with self.assertRaises(ValueError):
                    manager.unpack(archive, self.root / 'extract')

    def test_recovery_after_interrupted_switch(self):
        folder = self.prepared('first', '1.0')
        manager.activate(self.root, 'stella', folder)
        previous = self.root / 'runtime/.previous/stella'
        previous.parent.mkdir(exist_ok=True)
        manager.write(self.root / 'runtime/.transaction.json', {'engine': 'stella'})
        (self.root / 'runtime/stella').rename(previous)
        manager.recover(self.root)
        self.assertTrue((self.root / 'runtime/stella/stella').exists())
        self.assertFalse((self.root / 'runtime/.transaction.json').exists())

    def test_offers_no_downgrade(self):
        folder = self.prepared('current', '2.0')
        manager.activate(self.root, 'stella', folder)
        older = self.prepared('older', '1.0')
        offer = pack(older, self.root, 'runtime-test')
        manager.validate_offer(offer)
        manager.write(self.root / 'cache/runtime-updates.json', {'packages': {'stella': offer}})
        self.assertFalse(manager.status(self.root)['engines'][0]['update'])
        bad = copy.deepcopy(offer)
        bad['url'] = 'https://github.com/attacker/other/releases/download/v1/code.tar.gz'
        with self.assertRaises(ValueError):
            manager.validate_offer(bad)
        for url in ['http://github.com/dejotaerre/vetustware/releases/download/v1/x', 'https://github.com@evil.example/x', 'file:///tmp/x']:
            self.assertFalse(manager.trusted_url(url))

    def test_download_failure_keeps_current(self):
        current = self.prepared('current', '1.0')
        manager.activate(self.root, 'stella', current)
        with patch.object(manager, 'check', side_effect=OSError('offline')):
            with self.assertRaises(OSError):
                manager.install(self.root, 'stella')
        self.assertEqual(manager.status(self.root)['engines'][0]['version'], '1.0')

    def test_complete_update_with_mock_network(self):
        manager.activate(self.root, 'stella', self.prepared('first', '1.0'))
        offer = pack(self.prepared('second', '2.0'), self.root, 'runtime-test')
        package = self.root / Path(offer['url']).name
        manifest_url = 'https://github.com/dejotaerre/vetustware/releases/download/runtime-test/' + manager.MANIFEST
        def download(url, destination, maximum, expected=None):
            if url == manager.API:
                destination.write_text(json.dumps([{'assets': [{'name': manager.MANIFEST, 'browser_download_url': manifest_url}]}]))
            elif url == manifest_url:
                destination.write_text(json.dumps({'schema': 1, 'packages': [offer]}))
            else:
                self.assertEqual(url, offer['url'])
                self.assertEqual(expected, manager.sha(package))
                shutil.copy2(package, destination)
        with patch.object(manager, 'download', side_effect=download):
            manager.install(self.root, 'stella')
        self.assertEqual(manager.status(self.root)['engines'][0]['version'], '2.0')
        self.assertTrue(manager.status(self.root)['engines'][0]['rollback'])

    def test_launcher_keeps_game_argument_last(self):
        folder = self.root / 'launcher'
        (folder / 'bin').mkdir(parents=True)
        executable = folder / 'bin/scummvm'
        executable.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
        executable.chmod(0o755)
        wrapper = folder / 'scummvm'
        wrapper.write_text(launcher('scummvm'))
        wrapper.chmod(0o755)
        arguments = subprocess.check_output([str(wrapper), '--config=personal.ini', 'selected-game'], text=True).splitlines()
        self.assertEqual(arguments[-2:], ['--config=personal.ini', 'selected-game'])
        self.assertTrue(arguments[0].startswith('--extrapath='))

    def test_running_private_emulator_blocks_update(self):
        executable = self.root / 'runtime/sleep'
        shutil.copy2('/usr/bin/sleep', executable)
        process = subprocess.Popen([str(executable), '20'])
        try:
            with self.assertRaisesRegex(RuntimeError, 'Close the emulator'):
                manager.ensure_idle(self.root)
        finally:
            process.terminate()
            process.wait(timeout=5)

    def test_download_checksum_rejected(self):
        response = io.BytesIO(b'corrupted')
        with patch.object(manager.urllib.request, 'build_opener') as opener:
            opener.return_value.open.return_value = response
            with self.assertRaisesRegex(ValueError, 'checksum'):
                manager.download('https://github.com/dejotaerre/vetustware/releases/download/v1/test', self.root / 'bad', 100, '0' * 64)


if __name__ == '__main__':
    unittest.main()
