"""Comprueba importación XML y referencias compartidas en ambas estructuras."""
import json
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile
from catalog_index import build
from library_layout import resource


class LayoutTests(unittest.TestCase):
    def test_embedded_resources_follow_collection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for engine in ('dosbox', 'mame'):
                (root / 'games' / engine / 'resources').mkdir(parents=True)
            self.assertEqual(resource(root, 'resources/audio/soundfonts/test.sf2', 'unused'), root / 'games/dosbox/resources/audio/soundfonts/test.sf2')
            self.assertEqual(resource(root, 'resources/dos/herramientas.zip', 'unused'), root / 'games/dosbox/resources/dos/herramientas.zip')
            self.assertEqual(resource(root, 'resources/mame/hash/test.xml', 'unused'), root / 'games/mame/resources/hash/test.xml')

    def test_rebuild_preserves_shared_media(self):
        for modern in (False, "biblioteca.json", "library.json", "embedded"):
            with self.subTest(organized=modern), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                if modern == "embedded":
                    (root / "library.json").write_text('{"format":1}')
                    (root / "games/dosbox/metadata").mkdir(parents=True)
                elif modern:
                    (root / modern).write_text('{"format":1,"layout":"organized"}')
                archive = resource(root, 'metadata/dosbox/recursos.zip', 'Content/XODOSMetadata.zip')
                self.assertEqual(archive.parts[-3], 'dosbox' if modern == 'embedded' else 'metadata' if modern == 'library.json' else ('metadatos' if modern else root.name))
                archive.parent.mkdir(parents=True, exist_ok=True)
                alias = 'Images/MS-DOS/Box - Front/Test-01.png'
                actual = 'Images/MS-DOS/Box - Front/Shared.png'
                xml = '<LaunchBox><Game><ID>test</ID><Title>Test</Title><ApplicationPath>game/Test.bat</ApplicationPath></Game></LaunchBox>'
                with ZipFile(archive, 'w') as z:
                    z.writestr('xml/MS-DOS.xml', xml)
                    z.writestr(actual, b'image')
                (archive.parent / 'recursos-compartidos.json').write_text(json.dumps({'format': 1, 'archives': {archive.name: {alias: {'entry': actual}}}}))
                catalog = build(root, root / 'output')
                self.assertEqual(len(catalog['games']), 1)
                cover = catalog['games'][0]['cover']
                self.assertIsNotNone(cover)
                self.assertEqual(catalog['assets'][cover], actual)


if __name__ == '__main__':
    unittest.main()
