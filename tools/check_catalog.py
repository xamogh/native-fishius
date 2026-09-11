#!/usr/bin/env python3
"""Independently compare shipped content with cached workbook cells.

This audit reads the original workbook, never content.raw_sheets and never the
importer. Explicit sale rewards and calculated LE rewards are checked separately.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from decimal import Decimal, ROUND_HALF_UP
from pathlib import Path

import openpyxl

ROOT = Path(__file__).resolve().parents[1]


def rounded(value):
    return int(Decimal(str(value)).quantize(Decimal('1'), rounding=ROUND_HALF_UP))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workbook', type=Path, default=ROOT / 'design/aquarium_game_master_model.xlsx')
    parser.add_argument('--content', type=Path, default=ROOT / 'assets/content.json')
    parser.add_argument('--out', type=Path, default=ROOT / 'evidence/catalog-audit.json')
    args = parser.parse_args()
    result = {
        'workbook': str(args.workbook), 'content': str(args.content),
        'checked': [], 'errors': [], 'unverified': [], 'source_conflicts': [],
        'explicit_field_checks': 0, 'derived_field_checks': 0,
    }

    def check(field, actual, expected, origin, derived=False):
        result['derived_field_checks' if derived else 'explicit_field_checks'] += 1
        if actual != expected:
            result['errors'].append({'field': field, 'actual': actual, 'expected': expected, 'source': origin})
        result['checked'].append({'field': field, 'source': origin, 'matches': actual == expected, 'derived': derived})

    try:
        data = json.loads(args.content.read_text())
        workbook = openpyxl.load_workbook(args.workbook, read_only=True, data_only=True)
        # Cache ORIGINAL worksheet values for fast independent lookup.
        sheets = {sheet.title: list(sheet.values) for sheet in workbook}

        def cell(sheet, address):
            row, column = openpyxl.utils.cell.coordinate_to_tuple(address)
            values = sheets[sheet][row - 1]
            return values[column - 1] if column <= len(values) else None

        def direct(species, field, sheet, address, transform=lambda x: x):
            check(f'species.{species["id"]}.{field}', species.get(field), transform(cell(sheet, address)), f'{sheet}!{address}')

        check('workbook_sha256', data.get('workbook_sha256'), hashlib.sha256(args.workbook.read_bytes()).hexdigest(), 'workbook file')
        check('counts', data.get('counts'), {'coin': 26, 'premium': 10, 'limited': 10}, 'launch catalog row counts')
        check('species.count', len(data.get('species', [])), 46, 'catalog sheets')
        imported = {item['model_id']: item for item in data.get('species', [])}
        check('species.unique_model_ids', len(imported), 46, 'catalog sheets')
        check('species.unique_ids', len({item['id'] for item in data.get('species', [])}), 46, 'catalog sheets')
        # Fixed addresses intentionally differ from the importer's schema reader.
        mappings = [
            ('Coin Fish', 'CF', {'name': 'B', 'rarity': 'C', 'level': 'D', 'price': 'E', 'stage_ms': 'F', 'feed_ms': 'I', 'grace_ms': 'J', 'buy_xp': 'M', 'role': 'AA', 'description': 'AB', 'badge': 'AD'}, ['R', 'Q', 'P', 'N'], ['U', 'T', 'S', 'O']),
            ('Premium Fish', 'PF', {'name': 'B', 'level': 'C', 'price': 'D', 'stage_ms': 'E', 'feed_ms': 'H', 'grace_ms': 'I', 'buy_xp': 'J', 'role': 'U', 'description': 'AA'}, ['O', 'N', 'M', 'K'], ['R', 'Q', 'P', 'L']),
            ('Limited Edition', 'LE', {'name': 'B', 'level': 'E', 'price': 'G', 'stage_ms': 'H', 'feed_ms': 'K', 'grace_ms': 'L', 'buy_xp': 'O', 'event_copy': 'D'}, ['R', None, None, 'P'], ['S', None, None, 'Q']),
        ]
        seen = set()
        source_names = {}
        for sheet, prefix, mapping, coin_columns, xp_columns in mappings:
            for row_number, source_row in enumerate(sheets[sheet], 1):
                if not source_row or not isinstance(source_row[0], str) or not re.fullmatch(prefix + r'-\d+', source_row[0]):
                    continue
                model_id = source_row[0]
                seen.add(model_id)
                if model_id not in imported:
                    result['errors'].append({'field': 'species', 'missing_model_id': model_id})
                    continue
                species = imported[model_id]
                species_id = species['id']
                name = cell(sheet, f'B{row_number}')
                words = re.findall(r'[A-Za-z0-9]+', name)
                expected_id = words[0].lower() + ''.join(word[:1].upper() + word[1:] for word in words[1:])
                source_names[name] = expected_id
                check(f'species.{species_id}.id', species_id, expected_id, f'{sheet}!B{row_number}', True)
                direct(species, 'model_id', sheet, f'A{row_number}')
                for field, column in mapping.items():
                    transform = (lambda x: rounded(Decimal(str(x)) * 3600000)) if field.endswith('_ms') else str.lower if field == 'rarity' else lambda x: x
                    direct(species, field, sheet, f'{column}{row_number}', transform)
                check(f'species.{species_id}.source_sheet', species.get('source_sheet'), sheet, f'{sheet}!A{row_number}')
                check(f'species.{species_id}.source_row', species.get('source_row'), row_number, f'{sheet}!A{row_number}')
                currency = 'coins' if prefix == 'CF' else 'pearls' if prefix == 'PF' else str(cell(sheet, f'F{row_number}')).lower()
                check(f'species.{species_id}.currency', species.get('currency'), currency, f'{sheet}!F{row_number}' if prefix == 'LE' else f'{sheet}!purchase column heading')
                if prefix != 'CF':
                    check(f'species.{species_id}.rarity', species.get('rarity'), 'premium' if prefix == 'PF' else 'limited', sheet)
                for field, columns in [('sale_coins', coin_columns), ('sale_xp', xp_columns)]:
                    rewards = species.get(field)
                    if not isinstance(rewards, list) or len(rewards) != 5:
                        result['errors'].append({'field': f'species.{species_id}.{field}', 'reason': 'Expected actual snake_case array with five age entries'})
                        continue
                    for age, reward in enumerate(rewards):
                        if isinstance(reward, bool) or not isinstance(reward, int) or reward < 0:
                            result['errors'].append({'field': f'species.{species_id}.{field}[{age}]', 'reason': 'Reward must be a nonnegative integer'})
                    check(f'species.{species_id}.{field}[0]', rewards[0], 0, 'implementation: baby reward is zero', True)
                    for age, column in enumerate(columns, 1):
                        if column:
                            check(f'species.{species_id}.{field}[{age}]', rewards[age], cell(sheet, f'{column}{row_number}'), f'{sheet}!{column}{row_number}')
                        else:
                            adult = Decimal(str(cell(sheet, f'{columns[-1]}{row_number}')))
                            ratio = Decimal(str(cell('Inputs', f'B{21 + age}')))
                            price = Decimal(str(cell(sheet, f'G{row_number}')))
                            expected = rounded(price + (adult - price) * ratio) if field == 'sale_coins' and currency == 'coins' else rounded(adult * ratio)
                            check(f'species.{species_id}.{field}[{age}]', rewards[age], expected, f'{sheet}!{columns[-1]}{row_number}; Inputs!B{21 + age}', True)
                required_provenance = set(mapping) | {'id', 'model_id', 'currency', 'sale_coins', 'sale_xp'}
                missing = sorted(required_provenance - species.get('provenance', {}).keys())
                if missing:
                    result['unverified'].append({'species': species_id, 'missing_field_provenance': missing})
                if prefix != 'LE':
                    check(f'species.{species_id}.event_configured', species.get('event_configured'), True, 'Regular catalog has no seasonal availability gate')
                if prefix == 'LE':
                    availability = cell(sheet, f'D{row_number}')
                    check(f'species.{species_id}.annual', species.get('annual'), currency == 'gift', f'{sheet}!D{row_number}/F{row_number}')
                    check(f'species.{species_id}.non_resellable', species.get('non_resellable'), currency == 'gift', f'{sheet}!Y{row_number}/Z{row_number}')
                    if availability == '1 week, yearly':
                        start, end, configured = 0, 0, False
                        result['unverified'].append({'field': f'species.{species_id}.calendar_dates', 'reason': 'Workbook specifies one week yearly but no dates; correctly left unconfigured.', 'requires_configuration': True})
                    else:
                        # Independently tokenize the source rather than use importer.event_window.
                        tokens = re.findall(r'[A-Za-z]+|\d+', availability.replace(' only', ''))
                        months = {'Jan': 1, 'Feb': 2, 'Mar': 3, 'Apr': 4, 'May': 5, 'Jun': 6, 'Jul': 7, 'Aug': 8, 'Sep': 9, 'Oct': 10, 'Nov': 11, 'Dec': 12}
                        start = months[tokens[0]] * 100 + int(tokens[1])
                        end = (months[tokens[0]] * 100 + int(tokens[2])) if len(tokens) == 3 else months[tokens[2]] * 100 + int(tokens[3])
                        configured = True
                    for field, expected in [('event_start', start), ('event_end', end), ('event_configured', configured)]:
                        check(f'species.{species_id}.{field}', species.get(field), expected, f'{sheet}!D{row_number}', True)
                if species_id == 'bubbleEyeGoldfish':
                    notes = str(cell('Premium Fish', f'AA{row_number}'))
                    if not notes.startswith('Repeatable '):
                        result['errors'].append({'field': f'species.{species_id}.one_time', 'reason': 'Source policy changed and needs review'})
                    check(f'species.{species_id}.one_time', species.get('one_time'), False, f'Premium Fish!AA{row_number}')
                    result['source_conflicts'].append({'field': f'species.{species_id}.one_time', 'authoritative_cell': 'Premium Fish!AA2', 'conflicting_summary_cells': ['Unlock Plan!F5', 'Balance Review!B6', 'Inputs!F111', 'XP Curve!H5'], 'resolution': 'Detailed catalog explicitly says repeatable; summaries are stale.'})
        check('species.model_id_set', sorted(imported), sorted(seen), 'three catalog sheets')
        for level in range(1, 71):
            field, index = ('levels', level - 1) if level <= 40 else ('future_levels', level - 41)
            actual = data.get(field, [])
            check(f'{field}[{index}]', actual[index] if index < len(actual) else None, cell('XP Curve', f'G{level + 3}'), f'XP Curve!G{level + 3}')
        check('levels.count', len(data.get('levels', [])), 40, 'Unlock Plan!A45 launch cap')
        inputs = {
            'starting_coins': 'B6', 'starting_pearls': 'B7', 'starting_tank_capacity': 'B8',
            'starter_fish_count': 'B9', 'level_2_pearl_grant': 'B11',
            'growth_stages_to_adult': 'B19', 'feed_window_stage_multiplier': 'B20',
            'max_free_tank_capacity': 'B63', 'daily_quest_xp_share': 'B78',
            'weekly_quest_xp_share': 'B79', 'daily_quest_coin_share': 'B80',
        }
        for field, address in inputs.items():
            check(f'inputs.{field}', data.get('inputs', {}).get(field), cell('Inputs', address), f'Inputs!{address}')
        starter_names = [name.strip() for name in cell('Unlock Plan', 'C4').split(',')]
        check('starter_species', data.get('starter_species'), [source_names[name] for name in starter_names], 'Unlock Plan!C4', True)
        for tank in range(1, 6):
            source_row = tank + 3
            for field, column in [('tank_costs', 'H'), ('tank_tokens', 'G'), ('tank_friend_assists', 'F'), ('tank_capacities', 'E')]:
                text = str(cell('Tanks & Social', f'{column}{source_row}'))
                expected = [int(value) for value in re.findall(r'\d+', text.replace(',', ''))]
                values = data.get(field, [])
                actual = values[tank - 1] if tank <= len(values) else None
                check(f'{field}[{tank - 1}]', actual, expected, f'Tanks & Social!{column}{source_row}')
            levels = data.get('tank_levels', [])
            base_level = cell('Tanks & Social', f'B{source_row}')
            check(f'tank_levels[{tank - 1}]', levels[tank - 1] if tank <= len(levels) else None, base_level, f'Tanks & Social!B{source_row}')
            beats = [base_level]
            for label in ['Medium', 'Large']:
                matches = [row[0] for row in sheets['Unlock Plan'] if len(row) > 6 and row[6] == f'{label} Tank {tank}']
                beats.append(matches[0] if matches else None)
            beats.append(None)
            values = data.get('tank_expansion_levels', [])
            check(f'tank_expansion_levels[{tank - 1}]', values[tank - 1] if tank <= len(values) else None, beats, 'Unlock Plan!G4:G43 (label association only)', True)
        check('tank_expansion_mapping_confirmed', data.get('tank_expansion_mapping_confirmed'), False, 'Workbook has no explicit capacity-to-label mapping')
        result['unverified'].append({'field': 'tank_expansion_levels.capacity_mapping', 'reason': 'Workbook names Medium and Large beats but does not map all four slot steps to unlock levels.', 'requires_configuration': True})
        supplement = data.get('supplement', {})
        quest_definitions = {q['id']: q for q in supplement.get('quest_definitions', [])}
        if not quest_definitions:
            result['errors'].append({'field': 'supplement.quest_definitions', 'reason': 'Run import_supplement.py after the catalog import.'})
        else:
            quest_rows = {'daily-feed': 4, 'daily-sell': 5, 'daily-adult': 6,
                          'daily-decor': 7, 'weekly-collection': 8, 'weekly-helper': 9}
            check('quests.ids', sorted(quest_definitions), sorted(quest_rows), 'Quests & Live Ops!B4:B9')
            for qid, row in quest_rows.items():
                actual = quest_definitions.get(qid, {})
                check(f'quests.{qid}.label', actual.get('label'), cell('Quests & Live Ops', f'B{row}'), f'Quests & Live Ops!B{row}')
                target = int(re.findall(r'\d+', cell('Quests & Live Ops', f'C{row}'))[0])
                level = int(re.findall(r'\d+', cell('Quests & Live Ops', f'E{row}'))[0])
                check(f'quests.{qid}.target', actual.get('target'), target, f'Quests & Live Ops!C{row}', True)
                check(f'quests.{qid}.level', actual.get('level'), level, f'Quests & Live Ops!E{row}', True)
                check(f'quests.{qid}.weekly', actual.get('weekly'), cell('Quests & Live Ops', f'A{row}') == 'Weekly', f'Quests & Live Ops!A{row}', True)
            for level in range(2, 41):
                row = level + 2
                profile = supplement.get('quests_by_level', {}).get(str(level), {})
                for qid, column in [('daily-feed', 'D'), ('daily-sell', 'E'), ('daily-adult', 'F'), ('daily-decor', 'G'), ('weekly-collection', 'H')]:
                    reward = profile.get(qid, {})
                    check(f'quests.L{level}.{qid}.xp', reward.get('xp'), cell('Quest Scaling', f'{column}{row}'), f'Quest Scaling!{column}{row}')
                pools = supplement.get('quest_pools_by_level', {}).get(str(level), {})
                check(f'quests.L{level}.daily_xp_pool', pools.get('daily_xp'), cell('Quest Scaling', f'C{row}'), f'Quest Scaling!C{row}')
                check(f'quests.L{level}.daily_coin_pool', pools.get('daily_coins'), cell('Quest Scaling', f'I{row}'), f'Quest Scaling!I{row}')
            mastery_targets = [int(n) for n in re.findall(r'\d+', cell('Quests & Live Ops', 'C10'))]
            check('mastery.adult_targets', supplement.get('mastery', {}).get('adult_targets'), mastery_targets, 'Quests & Live Ops!C10', True)
            for area, missing in supplement.get('missing_specifications', {}).items():
                result['unverified'].append({'field': f'supplement.{area}', 'reason': '; '.join(missing), 'requires_configuration': True})
        workbook.close()
    except Exception as error:
        result['errors'].append({'error': str(error), 'type': type(error).__name__})
    result['structural_pass'] = not result['errors']
    missing_provenance = [item for item in result['unverified'] if not item.get('requires_configuration')]
    result['independent_workbook_parity_verified'] = not result['errors'] and not missing_provenance
    result['all_runtime_values_specified_by_workbook'] = not result['unverified']
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: value for key, value in result.items() if key != 'checked'}, indent=2))
    return 0 if result['independent_workbook_parity_verified'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
