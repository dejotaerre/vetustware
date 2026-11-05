#!/usr/bin/env python3
"""Comprueba variantes, auditoría y preservación de partidas de Arcade."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import mame_index as index
from mame_launch import seed, arguments

class ArcadeTests(unittest.TestCase):
    def test_merged_clones_and_unavailable_parents(self):
        data={'parent':{'parent':'','romof':''},'clone':{'parent':'parent','romof':'parent'},'other':{'parent':'','romof':'bios'}}
        self.assertEqual(set(index.candidates(data,{'parent','bios'})),{'parent','clone','other'})
        self.assertEqual(set(index.candidates(data,{'parent'})),{'parent','clone'})

    def test_reject_other_engine_version(self):
        with patch('mame_index.subprocess.run',return_value=subprocess.CompletedProcess([],0,'MAME v0.242 (x)','')):
            with self.assertRaises(ValueError):index.check_version('/mame')

    def test_audit_does_not_mark_missing_clones_as_good(self):
        with tempfile.TemporaryDirectory() as d:
            output='romset parent is good\nromset clone [parent] is bad\nromset undumped is best available\n'
            with patch('mame_index.subprocess.run',return_value=subprocess.CompletedProcess([],2,output,'')):
                result=index.audit('/mame',Path(d),['parent','clone','missing','undumped'],Path(d)/'log')
            self.assertEqual(result,{'parent':'good','clone':'bad','missing':'missing','undumped':'best available'})

    def test_failed_auditor_does_not_publish_bad_catalog(self):
        with tempfile.TemporaryDirectory() as d, patch('mame_index.subprocess.run',return_value=subprocess.CompletedProcess([],1,'','invalid option')):
            with self.assertRaises(ValueError):index.audit('/mame',Path(d),['parent'],Path(d)/'log')

    def test_seed_preserves_existing_saves_and_settings(self):
        with tempfile.TemporaryDirectory() as d:
            base=Path(d)/'library';personal=Path(d)/'personal'
            for root in [base,personal]:(root/'cfg').mkdir(parents=True)
            (base/'cfg/default.cfg').write_bytes(b'old');(personal/'cfg/default.cfg').write_bytes(b'current')
            (base/'nvram/game').mkdir(parents=True);(base/'nvram/game/save').write_bytes(b'original save')
            seed(base,personal)
            self.assertEqual((personal/'cfg/default.cfg').read_bytes(),b'current')
            self.assertEqual((personal/'nvram/game/save').read_bytes(),b'original save')
            (personal/'nvram/game/save').write_bytes(b'new save');seed(base,personal)
            self.assertEqual((personal/'nvram/game/save').read_bytes(),b'new save')
            self.assertEqual((base/'nvram/game/save').read_bytes(),b'original save')

    def test_launch_paths_and_monitor(self):
        args=arguments(Path('/library'),Path('/personal'),'puckman',False,'screen1',False)
        self.assertIn('-window',args);self.assertIn('-keepaspect',args);self.assertIn('-unevenstretch',args)
        self.assertEqual(args[args.index('-screen')+1],'screen1')
        self.assertEqual(args[args.index('-nvram_directory')+1],'/personal/nvram')
        self.assertEqual(args[-1],'puckman')

    def test_group_media_fallback_and_working_edition_first(self):
        with tempfile.TemporaryDirectory() as d:
            b=Path(d);(b/'flyers').mkdir();(b/'flyers/parent.png').write_bytes(b'image')
            common={'year':'1990','manufacturer':'Company','status':'good','players':'1','rotation':'0','savestate':True,'controls':'joy','romof':''}
            data={'parent':dict(common,id='parent',parent='',title='Game'),'clone':dict(common,id='clone',parent='parent',title='Game Japan')}
            catalog=index.make_catalog(data,{'parent':'bad','clone':'good'},b)
            self.assertEqual(len(catalog['games']),1)
            chosen=catalog['games'][0]['variants'][0]
            self.assertEqual(chosen['id'],'clone');self.assertEqual(chosen['cover'],'flyers/parent.png')
            self.assertEqual(catalog['verified'],1)

if __name__=='__main__':unittest.main()
