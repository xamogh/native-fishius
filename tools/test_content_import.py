#!/usr/bin/env python3
"""Source-backed regression checks for atomic, repeatable v4 imports."""
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
import openpyxl
from check_catalog import audit
from import_v4 import decor_purchase_xp
ROOT=Path(__file__).resolve().parents[1]
class ContentImportTests(unittest.TestCase):
    def test_decor_price_rewards_and_rounding(self):
        for price,expected in [(1,1),(4,1),(5,1),(14,1),(15,2),(25,3),(70,7),(305,31),(1000,100),(41705,4171)]:
            self.assertEqual(decor_purchase_xp(price,'coins'),expected)
        for price,expected in [(1,10),(12,120),(18,180),(20,200),(30,300)]:
            self.assertEqual(decor_purchase_xp(price,'pearls'),expected)
        for price,currency in [(0,'coins'),(-1,'pearls'),(1.5,'coins'),(True,'coins'),(25,'gift')]:
            with self.assertRaises(ValueError):decor_purchase_xp(price,currency)

    def test_repeatable_complete_import_and_independent_audit(self):
        with tempfile.TemporaryDirectory(prefix='aquarium-v4-import-') as folder:
            out=Path(folder)/'content.json'
            command=[sys.executable,str(ROOT/'tools/import_content.py'),'--out',str(out)]
            subprocess.run(command,check=True,capture_output=True)
            first=out.read_bytes();data=json.loads(first)
            subprocess.run(command,check=True,capture_output=True)
            self.assertEqual(out.read_bytes(),first)
            self.assertEqual(audit(ROOT/'design/aquarium_game_design_v4.xlsx',data)['errors'],[])
            self.assertEqual(data['overrides']['hatch_ms'],6000)
            self.assertTrue(data['overrides']['hungry_pauses_growth'])
            self.assertFalse(data['overrides']['legacy_save_migration'])
            self.assertFalse(data['overrides']['keep_action'])
            self.assertEqual(data['overrides']['fish_capacity'],'shared')
            self.assertEqual(data['overrides']['fish_lifecycle'],'shared')
            fish={s['id']:s for s in data['species']}
            self.assertEqual(fish['bubbleEyeGoldfish']['currency'],'pearls')
            self.assertEqual(fish['bubbleEyeGoldfish']['price'],1)
            self.assertFalse(fish['bubbleEyeGoldfish']['one_time'])
            for species in fish.values():
                self.assertNotIn('companion',species)
                self.assertNotIn('non_resellable',species)
                self.assertGreater(species['duration_ms'],0)
                self.assertGreater(species['sale_coins'][4],0)
                self.assertGreater(species['sale_xp'][4],0)
            self.assertFalse(data['overrides']['tank_upgrade_level_gate'])
            self.assertTrue(data['overrides']['decor_purchase_xp']['first_purchase_only'])
            decor={d['id']:d for d in data['decorations']['items']}
            self.assertEqual([decor[key]['buy_xp'] for key in ('CP-01','CD-01','CP-04','PP-01')],[3,7,31,120])
            self.assertEqual(len(data['tank_entitlements']),35)
            self.assertEqual(sum(t['added_slots'] for t in data['tank_entitlements']),200)
            for tank,level in enumerate((1,7,16,25,34),1):
                steps=sorted((t for t in data['tank_entitlements'] if t['tank']==tank),key=lambda t:t['slots'])
                self.assertEqual([t['slots'] for t in steps],[10,15,20,25,30,35,40])
                self.assertEqual([t['level'] for t in steps],[level,0,0,0,0,0,0])
            first=[t for t in data['tank_entitlements'] if t['tank']==1]
            self.assertEqual([t['coins'] for t in first],[0,300,900,1800,3000,4500,6300])
            self.assertEqual([t['pearls'] for t in first],[0,2,3,4,5,6,7])
            self.assertEqual(data['economy']['base_xp_day'],576)
            self.assertEqual(data['economy']['xp_slope_bps'],1500)
            self.assertEqual(data['pearl_earning'],dict(sales_target=20,reward=1))
            self.assertEqual(fish['neonTetra']['preview_xp'],10)
            self.assertEqual(fish['koi']['price'],3)
            self.assertEqual(fish['platinumArowana']['price'],14)
            self.assertFalse(any(data['feature_flags'].values()))
            self.assertEqual(data['supplement'],{})
            self.assertIn('Aquascape Projects',data['raw_sheets'])
            self.assertFalse(list(Path(folder).glob('*.tmp')))
    def test_invalid_workbook_leaves_output_untouched(self):
        with tempfile.TemporaryDirectory(prefix='aquarium-v4-failure-') as folder:
            folder=Path(folder);out=folder/'content.json';out.write_text('existing output')
            book=openpyxl.load_workbook(ROOT/'design/aquarium_game_design_v4.xlsx',data_only=True)
            book['Coin Fish']['U5']=50;bad=folder/'bad.xlsx';book.save(bad);book.close()
            result=subprocess.run([sys.executable,str(ROOT/'tools/import_content.py'),str(bad),'--out',str(out)],capture_output=True)
            self.assertNotEqual(result.returncode,0);self.assertEqual(out.read_text(),'existing output');self.assertFalse(list(folder.glob('*.tmp')))
if __name__=='__main__':unittest.main()
