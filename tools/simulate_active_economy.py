#!/usr/bin/env python3
"""Estimate active fish-selling progression from generated runtime content.

This deterministic cash-flow model assumes continuous twenty-minute Neon Tetra
batches, feeding at hatch and halfway, no missed visits, and no decor purchases.
It buys actual capacity with coins and preserves each fish's purchase quote.
Hours are accumulated active-cycle time, not a forecast of calendar days.
The C++ economy_balance test separately checks the real first forty minutes.
"""
import argparse
import bisect
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def half_up(numerator, denominator):
    return (2*numerator+denominator)//(2*denominator)


def quote(content, fish, level):
    economy = content['economy']
    values = {}
    for kind in ('coin', 'xp'):
        values[kind] = half_up(
            economy['base_'+kind+'_day']
            * (10000+economy[kind+'_slope_bps']*(level-1))
            * fish['duration_ms'] * fish[kind+'_factor_bps'] * fish[kind+'_weight_bps'],
            86400000*10000**3)
    price = max(economy['minimum_price'], half_up(values['coin']*economy['principal_share_bps'],10000))
    return price, values['coin'], values['xp']


def simulate(content, tank_limit, favorites, max_hours=5000):
    fish = next(s for s in content['species'] if s['id']=='neonTetra')
    levels = content['levels']
    coins = content['inputs']['starting_coins']
    pearls = content['inputs']['starting_pearls']
    xp = elapsed = sales = upgrades_spent = 0
    tanks = {1: content['inputs']['starting_slots']}
    # Keep the gifted Guppy and Platy. The two gifted Tetras can be sold.
    retained = 2
    initial = quote(content, fish, 1)
    growing = [(0, initial[1], initial[2])]*2
    milestones = {}
    minimum_coins = coins
    capacity_at = {}
    while xp < levels[-1] and elapsed < max_hours*3600000:
        level = bisect.bisect_right(levels, xp)
        current = quote(content, fish, level)
        while True:
            choices = []
            for offer in content['tank_entitlements']:
                tank = offer['tank']
                if tank > tank_limit or offer['slots'] != tanks.get(tank,5)+5:
                    continue
                if tank not in tanks and (level < offer['level'] or tank-1 not in tanks):
                    continue
                # Keep enough working coins to restock every open fish space.
                new_slots = sum(tanks.values())+offer['added_slots']
                reserve = (new_slots-retained-len(growing))*current[0]
                if coins >= offer['coins']+reserve:
                    choices.append(offer)
            if not choices:
                break
            offer = min(choices, key=lambda row:(row['coins']/row['added_slots'],row['tank']))
            coins -= offer['coins']
            upgrades_spent += offer['coins']
            tanks[offer['tank']] = offer['slots']
            capacity_at.setdefault(sum(tanks.values()), round(elapsed/3600000,2))
        capacity = sum(tanks.values())
        # Add favorites as room opens, preserving at least ten selling spaces.
        desired = max(2,min(favorites,capacity-10))
        while retained < desired and coins >= current[0] and retained+len(growing)<capacity:
            coins -= current[0]
            retained += 1
        while retained+len(growing)<capacity and coins>=current[0]:
            coins -= current[0]
            growing.append(current)
        minimum_coins = min(minimum_coins,coins)
        if not growing:
            break
        elapsed += fish['duration_ms']
        for principal, profit, reward_xp in growing:
            before = bisect.bisect_right(levels, xp)
            coins += principal+profit
            xp = min(levels[-1],xp+reward_xp)
            sales += 1
            earning = content.get('pearl_earning')
            if earning and sales % earning['sales_target'] == 0:
                pearls += earning['reward']
            after = bisect.bisect_right(levels,xp)
            for reached in range(before+1,after+1):
                reward = content['level_rewards'][reached-1]
                coins += reward['coins']
                pearls += reward['pearls']
                milestones[str(reached)] = round(elapsed/3600000,2)
        growing = []
    return dict(tank_limit=tank_limit,favorites=retained,level=bisect.bisect_right(levels,xp),
                active_hours_by_level=milestones,capacity_hours=capacity_at,
                final_capacity=sum(tanks.values()),adult_sales=sales,
                capacity_coins_spent=upgrades_spent,minimum_coins=minimum_coins,
                final_coins=coins,earned_pearls=pearls)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--content',type=Path,default=ROOT/'assets/content.json')
    parser.add_argument('--out',type=Path)
    args=parser.parse_args()
    content=json.loads(args.content.read_text())
    result=dict(config=content['config_version'],assumptions=__doc__.strip(),
                scenarios=[simulate(content,tanks,favorites) for tanks,favorites in ((1,2),(1,10),(5,10))])
    output=json.dumps(result,indent=2)+'\n'
    if args.out:
        args.out.parent.mkdir(parents=True,exist_ok=True)
        args.out.write_text(output)
    else:
        print(output,end='')


if __name__=='__main__':
    main()
