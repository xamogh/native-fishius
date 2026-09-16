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
ROOT=Path(__file__).resolve().parents[1]
class ContentImportTests(unittest.TestCase):
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
