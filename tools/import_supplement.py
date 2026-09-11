#!/usr/bin/env python3
"""Import explicit launch quest rules and rewards from cached workbook rows.

Every mapped field is rebuilt. Unmapped local configuration and deferred
onboarding data are preserved; unsupported payouts are never invented.
"""
from __future__ import annotations
import argparse
import copy
import json
import os
import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def source(sheet, row, column, heading=None):
    result = {'sheet': sheet, 'row': row, 'column': column}
    if heading:
        result['heading'] = heading
    return result


def import_supplement(content):
    """Return fresh mapped supplements without mutating the caller's content."""
    result = copy.deepcopy(content)
    raw = result['raw_sheets']
    rules = [
        ('daily-feed', 'Feed Caretaker', 'healthy-feed', 4, 'Per-quest coin amount is not specified.'),
        ('daily-sell', 'Junior Seller', 'sell', 5, 'Per-quest coin amount is not specified.'),
        ('daily-adult', 'Adult Harvest', 'adult-sale', 6, 'Per-quest coin amount is not specified.'),
        ('daily-decor', 'Tank Stylist', 'decor', 7, 'Decor score reward is not specified.'),
        ('weekly-collection', 'Collection Chapter', 'collection', 8, 'Collection themes and Pearl or event egg rewards are not specified.'),
        ('weekly-helper', 'Neighbor Helper', 'gift', 9, 'Gift Token and coin rewards are not specified.'),
    ]
    definitions = []
    for qid, label, event, row, missing in rules:
        cells = raw['Quests & Live Ops'][row-1]
        if cells[1] != label:
            raise ValueError(f'Unexpected quest at Quests & Live Ops!B{row}: {cells[1]}')
        parsed_target = re.search(r'\b(\d+)\b', cells[2])
        parsed_level = re.fullmatch(r'L(\d+)\+', cells[4])
        if not parsed_target or not parsed_level:
            raise ValueError(f'Quest target or unlock cannot be read at row {row}')
        target, level = int(parsed_target[1]), int(parsed_level[1])
        weekly = cells[0] == 'Weekly'
        definitions.append(dict(id=qid, label=label, event=event, target=target,
                                level=level, weekly=weekly, configured=not weekly,
                                missing=missing, source=source('Quests & Live Ops', row, 3)))

    rows = raw['Quest Scaling']
    expected = ['Level', 'Next-Level XP Delta', 'Daily XP Pool', 'Feed Caretaker XP',
                'Junior Seller XP', 'Adult Harvest XP', 'Tank Stylist XP', 'Weekly XP',
                'Est. Daily Coin Pool', 'Quest Balance Flag']
    if rows[2][:10] != expected:
        raise ValueError('Quest Scaling headings changed; review explicit mapping')
    columns = {'daily-feed': 3, 'daily-sell': 4, 'daily-adult': 5,
               'daily-decor': 6, 'weekly-collection': 7}
    profiles = {str(level): {} for level in range(1, 41)}
    pools = {}
    for rowno, row in enumerate(rows[3:], 4):
        if not isinstance(row[0], (int, float)) or not 1 <= row[0] <= 40:
            continue
        level = int(row[0])
        for qid, column in columns.items():
            reward = row[column]
            if isinstance(reward, bool) or not isinstance(reward, (int, float)) or reward < 0:
                raise ValueError(f'Missing cached quest reward at row {rowno}, column {column+1}')
            profiles[str(level)][qid] = {'xp': int(reward), 'source': {
                'xp': source('Quest Scaling', rowno, column+1, expected[column])}}
        pools[str(level)] = {'daily_xp': int(row[2]), 'daily_coins': int(row[8]),
                             'source': {'daily_xp': source('Quest Scaling', rowno, 3),
                                        'daily_coins': source('Quest Scaling', rowno, 9)}}

    # Preserve only fields that this importer does not map. Replacing whole
    # mapped sections prevents removed or retuned workbook rules staying stale.
    supplement = result.get('supplement', {})
    if not isinstance(supplement, dict):
        raise ValueError('Supplement must be an object')
    supplement.update({
        'quest_definitions': definitions,
        'quests_by_level': profiles,
        'quest_pools_by_level': pools,
        'mastery': {'adult_targets': [int(n) for n in re.search(r'(\d+/\d+/\d+)', raw['Quests & Live Ops'][9][2])[1].split('/')],
                    'source': source('Quests & Live Ops', 10, 3),
                    'missing': ['XP reward amounts', 'Statue decoration specification']},
        'missing_specifications': {
            'mastery': ['XP reward amounts', 'Statue decoration specification'],
            'collection': ['Theme names and species membership', 'Pearl or event egg reward'],
            'decorator': ['Decor score thresholds', 'Cosmetic titles and backgrounds'],
            'daily_egg': ['Weekly drop tables and probabilities', 'Retired fish pool', 'Resale guardrail'],
            'quests': ['Daily coin pool split', 'Tank Stylist decor score reward', 'Neighbor Helper coin and token rewards'],
        },
        'onboarding_rewards': supplement.get('onboarding_rewards', {}),
        'import_status': {'quest_profiles': sum(bool(v) for v in profiles.values()),
                          'quest_definitions': len(definitions), 'tutorial': 'deferred'},
    })
    result['supplement'] = supplement
    return result


def write_json_atomic(path, value):
    """Replace a complete JSON artifact only after generation and sync succeed."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=path.parent,
                                         prefix=path.name + '.', suffix='.tmp', delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(json.dumps(value, indent=2, default=str) + '\n')
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--content', type=Path, default=ROOT / 'assets/content.json')
    parser.add_argument('--audit', type=Path, default=ROOT / 'evidence/supplement-import.json')
    args = parser.parse_args()
    content = import_supplement(json.loads(args.content.read_text()))
    write_json_atomic(args.content, content)
    write_json_atomic(args.audit, content['supplement'])
    print(json.dumps(content['supplement']['import_status']))


if __name__ == '__main__':
    main()
