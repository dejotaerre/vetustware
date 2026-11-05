#!/usr/bin/env python3
"""Verifica agrupación, copias idénticas y límites de la biblioteca Atari."""
import json
from pathlib import Path
import tempfile
import unittest
from atari_index import build

class AtariIndexTests(unittest.TestCase):
    def test_variants_duplicates_art_and_external_symlinks(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary);base=root/'library';roms=base/'roms';images=base/'snapshots'
            roms.mkdir(parents=True);images.mkdir()
            (roms/'Game (1982) (Studio).bin').write_bytes(b'first rom')
            (roms/'Game (1982) (Studio) (PAL).bin').write_bytes(b'pal rom')
            (roms/'Game duplicate.BIN').write_bytes(b'first rom')
            (roms/'Game.0a.sta').write_bytes(b'preserved state')
            (images/'Game (1982) (Studio).png').write_bytes(b'image')
            (root/'outside.bin').write_bytes(b'outside');(roms/'escape.bin').symlink_to(root/'outside.bin')
            before={str(p):p.read_bytes() for p in base.rglob('*') if p.is_file()}
            result=build(base)
            self.assertEqual((result['romFiles'],result['uniqueRoms'],len(result['games'])),(3,2,1))
            variants=result['games'][0]['variants'];self.assertEqual(len(variants[0]['files']),2)
            self.assertEqual(variants[0]['image'],'snapshots/Game (1982) (Studio).png')
            self.assertEqual(before,{str(p):p.read_bytes() for p in base.rglob('*') if p.is_file()})
            self.assertNotIn(str(root),json.dumps(result))
            moved=root/'moved';base.rename(moved)
            self.assertEqual(result,build(moved))

    def test_explicit_box_mapping_keeps_snapshot_and_rejects_escape(self):
        with tempfile.TemporaryDirectory() as temporary:
            base=Path(temporary)/'library';(base/'roms').mkdir(parents=True);(base/'snapshots').mkdir();(base/'boxes').mkdir()
            (base/'roms/Game.bin').write_bytes(b'rom')
            (base/'snapshots/Game.png').write_bytes(b'snapshot')
            (base/'boxes/Different-name--USA.png').write_bytes(b'box')
            original=build(base)['games'][0]['variants'][0]
            mapping={'format':1,'variants':{original['id']:'boxes/Different-name--USA.png'}}
            (base/'covers.json').write_text(json.dumps(mapping))
            updated=build(base)['games'][0]['variants'][0]
            self.assertEqual(updated['cover'],'boxes/Different-name--USA.png')
            self.assertEqual(updated['image'],original['image'])
            self.assertEqual(updated['files'],original['files'])
            outside=base.parent/'outside.png';outside.write_bytes(b'external')
            mapping['variants'][original['id']]='../outside.png'
            (base/'covers.json').write_text(json.dumps(mapping))
            self.assertEqual(build(base)['games'][0]['variants'][0]['cover'],'')

if __name__=='__main__':unittest.main()
