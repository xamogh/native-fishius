#!/usr/bin/env python3
"""Regression checks for complete, repeatable, atomic content imports."""
import copy
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ContentImportTests(unittest.TestCase):
    def run_tool(self, name, *args, succeeds=True):
        result = subprocess.run([sys.executable, str(ROOT / 'tools' / name), *map(str, args)],
                                cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, succeeds, result.stdout + result.stderr)
        return result

    def test_standalone_import_rebuilds_mappings_and_preserves_deferred_fields(self):
        with tempfile.TemporaryDirectory(prefix='aquarium-import-') as directory:
            output = Path(directory) / 'content.json'
            current = json.loads((ROOT / 'assets/content.json').read_text())
            deferred = {'feed': {'xp': 77, 'source': 'preserved deferred test configuration'}}
            custom = {'future_feature': {'enabled': False}}
            current['supplement']['onboarding_rewards'] = deferred
            current['supplement']['custom_configuration'] = custom
            current['supplement']['quests_by_level']['2']['daily-feed']['xp'] = 999999
            current['supplement']['quest_definitions'].append({'id': 'obsolete-quest'})
            current['supplement']['mastery']['adult_targets'] = [1, 2, 3]
            output.write_text(json.dumps(current))
            self.run_tool('import_content.py', ROOT / 'design/aquarium_game_master_model.xlsx', '--out', output)
            first = output.read_bytes()
            imported = json.loads(first)
            self.assertEqual(imported['supplement']['onboarding_rewards'], deferred)
            self.assertEqual(imported['supplement']['custom_configuration'], custom)
            self.assertEqual(imported['supplement']['quests_by_level']['2']['daily-feed']['xp'], 5)
            self.assertEqual(imported['supplement']['mastery']['adult_targets'], [5, 25, 100])
            self.assertEqual(len(imported['supplement']['quest_definitions']), 6)
            self.run_tool('import_content.py', ROOT / 'design/aquarium_game_master_model.xlsx', '--out', output)
            self.assertEqual(output.read_bytes(), first)
            # import_all still invokes this compatibility CLI after the catalog
            # import. It must neither change complete output nor remove fields.
            self.run_tool('import_supplement.py', '--content', output, '--audit', Path(directory) / 'supplement.json')
            self.assertEqual(output.read_bytes(), first)
            self.run_tool('check_catalog.py', '--content', output, '--out', Path(directory) / 'audit.json')
            self.assertFalse(list(Path(directory).glob('*.tmp')))

    def test_failed_mapping_preserves_existing_destination(self):
        with tempfile.TemporaryDirectory(prefix='aquarium-import-failure-') as directory:
            output = Path(directory) / 'content.json'
            current = json.loads((ROOT / 'assets/content.json').read_text())
            current['raw_sheets']['Quest Scaling'][2][3] = 'Renamed unmapped column'
            output.write_text(json.dumps(current))
            original = output.read_bytes()
            self.run_tool('import_supplement.py', '--content', output,
                          '--audit', Path(directory) / 'supplement.json', succeeds=False)
            self.assertEqual(output.read_bytes(), original)
            self.assertFalse((Path(directory) / 'supplement.json').exists())
            self.assertFalse(list(Path(directory).glob('*.tmp')))

    def test_supplement_function_does_not_mutate_input(self):
        from import_supplement import import_supplement
        content = json.loads((ROOT / 'assets/content.json').read_text())
        original = copy.deepcopy(content)
        updated = import_supplement(content)
        updated['supplement']['mastery']['adult_targets'][0] = 123
        self.assertEqual(content, original)


if __name__ == '__main__':
    unittest.main()
