#!/usr/bin/env python3
"""Build runtime v4 content from cached workbook values, without editing Excel.

Care, active-play economy, shared fish, decor capacity and decor purchase XP are overrides.
Future catalogs remain data; release gates and artwork determine availability.
"""
from pathlib import Path
from decimal import Decimal, ROUND_HALF_UP
from fractions import Fraction
import argparse
import hashlib
import json
import math
import re
import openpyxl
from decor_animation import animation_for

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WORKBOOK = ROOT / 'design/aquarium_game_design_v4.xlsx'
PEARL_FISH_SCHEDULE = 'SC-2H'
BUBBLE_EYE_PEARLS = 1
ACTIVE_PLAY = dict(base_xp_day=576, xp_slope_bps=1500, pearl_price_divisor=6,
                   adult_coin_sales_per_pearl=20, pearls_per_milestone=1,
                   new_tank_coins=[0,1500,6000,15000,30000],
                   new_tank_pearls=[0,5,10,15,20],
                   upgrade_coins=[300,900,1800,3000,4500,6300],
                   upgrade_pearls=[2,3,4,5,6,7], tank_price_step_bps=5000)
DECOR_PLACED_LIMIT = 64
DECOR_PURCHASE_XP = dict(coins_per_xp=10, xp_per_pearl=10, minimum_xp=1,
                        rounding='half_up', first_purchase_only=True)


def decor_purchase_xp(price, currency):
    if not isinstance(price, int) or isinstance(price, bool) or price <= 0:
        raise ValueError('Decor price must be a positive whole number')
    if currency == 'coins':
        divisor = DECOR_PURCHASE_XP['coins_per_xp']
        return max(DECOR_PURCHASE_XP['minimum_xp'], (price*2+divisor)//(divisor*2))
    if currency == 'pearls':
        return price*DECOR_PURCHASE_XP['xp_per_pearl']
    raise ValueError('Unknown decor currency')


def rounded(value):
    return int(Decimal(str(value)).quantize(Decimal('1'), rounding=ROUND_HALF_UP))


def camel(name):
    words = re.findall(r'[A-Za-z0-9]+', name)
    return words[0].lower() + ''.join(w[0].upper() + w[1:] for w in words[1:])


def numeric(value, name):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f'Missing cached number: {name}')
    return value


def build_content(path=DEFAULT_WORKBOOK, assets=ROOT / 'assets'):
    workbook = openpyxl.load_workbook(path, read_only=True, data_only=True)
    try:
        sheets = {s.title: [list(row) for row in s.values] for s in workbook}
    finally:
        workbook.close()
    inputs = {r[0]: numeric(r[1], str(r[0])) for r in sheets['Inputs'][4:] if r[0]}
    # Version the active-play overrides without rewriting source inputs or old
    # fish. Each owned fish retains its saved purchase quote.
    production = dict(inputs, base_xp_day=ACTIVE_PLAY['base_xp_day'],
                      xp_level_slope=Decimal(ACTIVE_PLAY['xp_slope_bps'])/10000)
    schedules = {}
    for r in sheets['Schedules'][4:]:
        schedules[r[0]] = dict(id=r[0], duration_ms=rounded(numeric(r[2], r[0])*3600000),
                              coin_factor_bps=rounded(r[3]*10000), xp_factor_bps=rounded(r[4]*10000))
    species = []
    for row, r in enumerate(sheets['Coin Fish'][4:], 5):
        if not r[0]:
            continue
        schedule = schedules[r[5]]
        item = dict(id=camel(r[1]), model_id=r[0], name=r[1], rarity=r[2].lower(), level=int(r[3]),
                    release_gate=r[4], schedule_id=r[5], role=r[6], coin_weight_bps=rounded(r[7]*10000),
                    xp_weight_bps=rounded(r[8]*10000), currency='coins', price=int(numeric(r[11], r[0])),
                    buy_xp=0, duration_ms=schedule['duration_ms'], feed_ms=min(schedule['duration_ms']//2,43200000),
                    coin_factor_bps=schedule['coin_factor_bps'], xp_factor_bps=schedule['xp_factor_bps'],
                    preview_profit=int(r[10]), preview_xp=int(r[13]),
                    sale_coins=[int(r[11]),int(r[14]),int(r[16]),int(r[18]),int(r[12])],
                    sale_xp=[0,int(r[15]),int(r[17]),int(r[19]),int(r[13])],
                    badge=r[6], description=r[26], length=38, event_configured=True,
                    annual=False, one_time=False,
                    source={'sheet':'Coin Fish','row':row})
        if r[20] != 0:
            raise ValueError(f'{r[0]} must have zero purchase XP')
        # Preserve frozen workbook values, then apply Rules R25: exact half-up
        # ROUND and integer FLOOR. Some cached cells used binary float/banker's
        # rounding and differ by one. Keep the correction visible in the data.
        item['cached_price']=item['price']
        item['cached_preview_profit']=item['preview_profit']
        item['cached_preview_xp']=item['preview_xp']
        item['cached_sale_coins']=item['sale_coins'][:]
        item['cached_sale_xp']=item['sale_xp'][:]
        def payout(kind):
            return rounded(Decimal(str(production['base_'+kind+'_day']))
                           * (1+Decimal(str(production[kind+'_level_slope']))*(item['level']-1))
                           * Decimal(schedule['duration_ms'])/86400000
                           * item[kind+'_factor_bps']/10000*item[kind+'_weight_bps']/10000)
        item['preview_profit']=payout('coin')
        item['preview_xp']=payout('xp')
        item['price']=max(int(inputs['min_egg_price']),rounded(Decimal(item['preview_profit'])*Decimal(str(inputs['working_capital_share']))))
        item['sale_coins'][0]=0  # Eggs and Babies are not eligible for settlement.
        item['sale_coins'][4]=item['price']+item['preview_profit']
        item['sale_xp'][4]=item['preview_xp']
        for stage,share in [(1,10),(2,40),(3,70)]:
            item['sale_coins'][stage]=item['price']*95//100+item['preview_profit']*share//100
            item['sale_xp'][stage]=item['preview_xp']*share//100
        species.append(item)
    for row, r in enumerate(sheets['Fish Collectibles'][4:], 5):
        if not r[0]:
            continue
        # User decision: every fish uses the ordinary egg, care and sale cycle.
        # Use the existing balanced two-hour schedule until species are retuned.
        # Pearl purchases have no refundable coin principal.
        schedule=schedules[PEARL_FISH_SCHEDULE]
        level=int(r[3])
        def payout(kind):
            return rounded(Decimal(str(production['base_'+kind+'_day']))
                           * (1+Decimal(str(production[kind+'_level_slope']))*(level-1))
                           * Decimal(schedule['duration_ms'])/86400000
                           * schedule[kind+'_factor_bps']/10000)
        profit,xp=payout('coin'),payout('xp')
        species.append(dict(id=camel(r[1]),model_id=r[0],name=r[1],rarity=r[2].lower(),level=level,
                            release_gate=r[4],schedule_id=schedule['id'],role='Balanced',coin_weight_bps=10000,xp_weight_bps=10000,
                            currency='pearls',price=BUBBLE_EYE_PEARLS if r[0]=='PF-01' else
                            (int(r[6])+ACTIVE_PLAY['pearl_price_divisor']-1)//ACTIVE_PLAY['pearl_price_divisor'],
                            cached_price=int(r[6]),buy_xp=0,
                            duration_ms=schedule['duration_ms'],feed_ms=min(schedule['duration_ms']//2,43200000),
                            coin_factor_bps=schedule['coin_factor_bps'],xp_factor_bps=schedule['xp_factor_bps'],
                            preview_profit=profit,preview_xp=xp,
                            sale_coins=[profit*share//100 for share in (0,10,40,70,100)],
                            sale_xp=[xp*share//100 for share in (0,10,40,70,100)],badge=r[2],description=r[14],
                            length=58,event_configured=r[7]=='None',annual=False,one_time=False,
                            source={'sheet':'Fish Collectibles','row':row}))
    for s in species:
        s['asset']='species/'+s['id']+'.png'
        s['art_ready']=(assets/s['asset']).is_file()
    if len({s['id'] for s in species}) != 99:
        raise ValueError('Expected 72 coin species and 27 pearl species')
    if sum(s['release_gate']=='Launch' and s['currency']=='coins' for s in species)!=40:
        raise ValueError('Expected 40 launch coin species')
    arts={}
    art_fields=['id','name','silhouette','palette','materials','footprint','placement','animation','motion_class','still','hook','construction']
    for row,r in enumerate(sheets['Decor Art Briefs'][4:],5):
        if r[0]:
            arts[r[0]]=dict(zip(art_fields,r[:12]))
            arts[r[0]]['source']={'sheet':'Decor Art Briefs','row':row}
            arts[r[0]]['motion']=animation_for(r[0])
    decorations=[]
    for row,r in enumerate(sheets['Decor Catalog'][4:],5):
        if not r[0]:
            continue
        if r[12]!=0 or r[0] not in arts or arts[r[0]]['name']!=r[1]:
            raise ValueError(f'{r[0]}: invalid decor XP or art brief')
        price=int(numeric(r[11],r[0]));currency=r[7].lower()
        decorations.append(dict(id=r[0],name=r[1],category=r[2],theme=r[3],edition=r[4],rarity=r[5],
                                 level=int(r[6]),currency=currency,price=price,
                                 buy_xp=decor_purchase_xp(price,currency),score=int(r[13]),width=r[14],height=r[15],size=r[16],layer=r[17],
                                 event='' if r[18]=='None' else r[18],release_gate=r[19],subcategory=r[21],
                                 availability='Permanent ownership',asset='decor/catalog/'+r[0]+'.png',art=arts[r[0]],
                                 source={'sheet':'Decor Catalog','row':row}))
    if len(decorations)!=120 or len(arts)!=120:
        raise ValueError('Expected 120 decor items and matching art briefs')
    events=[dict(name=r[1],proposed_window=r[2],coin_id=r[3],pearl_id=r[4],configured=False,starts_at=0,ends_at=0)
            for r in sheets['Events'][4:] if r[0]]
    source_tanks=[dict(id=r[0],tank=int(r[1]),level=int(r[2]),added_slots=int(r[3]),coins=int(r[4]),pearls=int(r[5]),
                prerequisite='' if r[6]=='None' else r[6],slots=int(r[8])) for r in sheets['Tanks'][4:] if r[0]]
    if len(source_tanks)!=15 or sum(t['added_slots'] for t in source_tanks)!=100:
        raise ValueError('Workbook capacity does not reconcile to 100 fish spaces')
    # Preserve the workbook's reference income curve for Treasure quotes.
    # Only new tanks require a level; owned tanks can buy every five-slot step.
    reference_slots=[sum(t['added_slots'] for t in source_tanks if t['level']<=level)
                     for level in range(1,41)]
    tanks=[dict(t,level=t['level'] if t['slots']==10 else 0) for t in source_tanks]
    for tank in range(1,6):
        for slots in range(25,41,5):
            tanks.append(dict(id=f'TK-{tank:02d}-{slots}',tank=tank,level=0,added_slots=5,
                              coins=0,pearls=0,
                              prerequisite=f'TK-{tank:02d}-{slots-5}',slots=slots))
    for row in tanks:
        tank=row['tank']
        if row['slots']==10:
            row['coins']=ACTIVE_PLAY['new_tank_coins'][tank-1]
            row['pearls']=ACTIVE_PLAY['new_tank_pearls'][tank-1]
        else:
            step=(row['slots']-15)//5
            multiplier=10000+(tank-1)*ACTIVE_PLAY['tank_price_step_bps']
            for currency in ('coins','pearls'):
                row[currency]=(ACTIVE_PLAY['upgrade_'+currency][step]*multiplier+5000)//10000
    levels=[r for r in sheets['XP & Unlocks'][4:] if isinstance(r[0],(int,float))]
    if [r[0] for r in levels]!=list(range(1,71)) or [r[7] for r in levels[:2]]!=[0,80]:
        raise ValueError('Invalid v4 XP curve')
    treasure = []
    treasure_assets = {
        f'{currency}-{suffix}': f'treasure/{prefix}-{tier}-v1.png'
        for currency, prefix in [('COIN', 'coin'), ('PEARL', 'pearl')]
        for suffix, tier in zip(('S', 'M', 'L', 'XL', 'XXL'), ('pocket', 'pile', 'bag', 'box', 'chest'))
    }
    treasure_assets['STARTER'] = 'treasure/starter-bundle-v1.png'
    for row, r in enumerate(sheets['Shop'][4:], 5):
        if r[1] not in ('Coins', 'Pearls', 'Bundle'):
            continue
        unlock = re.match(r'^Level (\d+)(?:;|$)', str(r[7]))
        if not unlock or not 0 <= int(unlock[1]) <= 40:
            raise ValueError(f'Invalid Treasure unlock level: {r[0]}')
        treasure.append(dict(id=r[0], name=r[2], kind=r[1].lower(),
                             asset=treasure_assets[r[0]],
                             price_usd_cents=rounded(Decimal(str(numeric(r[3], r[0])))*100),
                             pearls=int(numeric(r[4], r[0])),
                             coin_days_bps=rounded(Decimal(str(numeric(r[5], r[0])))*10000),
                             level=int(unlock[1]),
                             once_per_account=r[8]=='Lifetime once/account',
                             permanent_frame=bool(r[9]), eligibility=r[7],
                             source={'sheet':'Shop', 'row':row}))
    if any(sum(o['kind']==kind for o in treasure)!=5 for kind in ('coins', 'pearls')):
        raise ValueError('Expected five coin packs and five pearl packs')
    utilization = Fraction(str(inputs['reference_utilization'])).limit_denominator(86400)
    overrides=dict(hatch_ms=6000,egg_counts_toward_growth=True,hatch_hungry=True,
                   feed_duration_share=.5,feed_duration_cap_ms=43200000,hungry_pauses_growth=True,
                   sickness=False,death=False,fish_purchase_xp=0,decor_purchase_xp=DECOR_PURCHASE_XP,
                   decor_placed_limit=DECOR_PLACED_LIMIT,
                   backend='local',legacy_save_migration=False,
                   minimum_sell_age=1,selling_rule='Eggs and Babies cannot be sold; selling starts at Junior',
                   keep_action=False,fish_capacity='shared',fish_lifecycle='shared',
                   pearl_fish_schedule=PEARL_FISH_SCHEDULE,bubble_eye_pearls=BUBBLE_EYE_PEARLS,
                   pearl_fish_repeatable=True,
                   tank_upgrade_level_gate=False,tank_capacity_steps=list(range(10,41,5)),
                   active_play=ACTIVE_PLAY,
                   source='User decisions in this task, 14, 17 and 18 September 2026')
    checksum=hashlib.sha256(path.read_bytes()).hexdigest()
    config_checksum=hashlib.sha256(json.dumps([checksum,overrides,'R25-exact-rounding-v1'],sort_keys=True).encode()).hexdigest()
    return dict(schema=4,config_version='v4-fed-1-'+config_checksum[:12],workbook_sha256=checksum,
                config_checksum=config_checksum,overrides=overrides,inputs=inputs,
                economy=dict(base_coin_day=int(inputs['base_coin_day']),base_xp_day=ACTIVE_PLAY['base_xp_day'],
                             coin_slope_bps=rounded(inputs['coin_level_slope']*10000),xp_slope_bps=ACTIVE_PLAY['xp_slope_bps'],
                             principal_share_bps=rounded(inputs['working_capital_share']*10000),minimum_price=int(inputs['min_egg_price']),
                             early_refund_bps=rounded(inputs['early_refund_share']*10000),
                             stage_bps=[0,rounded(inputs['junior_time']*10000),rounded(inputs['young_time']*10000),rounded(inputs['mature_time']*10000),10000],
                             reward_bps=[0,rounded(inputs['junior_yield']*10000),rounded(inputs['young_yield']*10000),rounded(inputs['mature_yield']*10000),10000]),
                schedules=list(schedules.values()),species=species,counts={'coin':72,'pearl':27,'launch_coin':40},
                levels=[int(r[7]) for r in levels[:40]],future_levels=[int(r[7]) for r in levels[40:]],
                level_rewards=[dict(coins=int(r[8]),pearls=int(r[9])) for r in levels[:40]],tank_entitlements=tanks,
                pearl_earning=dict(sales_target=ACTIVE_PLAY['adult_coin_sales_per_pearl'],
                                   reward=ACTIVE_PLAY['pearls_per_milestone']),
                treasure=dict(offers=treasure, reference_utilization_numerator=utilization.numerator,
                              reference_utilization_denominator=utilization.denominator,
                              reference_slots_by_level=reference_slots),
                starter_species=['neonTetra','neonTetra','guppy','platy'],
                decorations=dict(items=decorations,events=events,tuning=dict(placed_limit=DECOR_PLACED_LIMIT,animated_limit=6,emitter_limit=2,
                    particle_limit=6,launch_level_cap=40,tutorial_total_xp=0,tutorial_item='CP-01')),
                feature_flags=dict(quests=False,mastery_rewards=False,projects=False,events=False,iap=False,ads=False),
                supplement={},raw_sheets=sheets)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('workbook',type=Path,nargs='?',default=DEFAULT_WORKBOOK)
    p.add_argument('--out',type=Path,default=ROOT/'assets/content.json')
    args=p.parse_args()
    result=build_content(args.workbook)
    args.out.parent.mkdir(parents=True,exist_ok=True)
    temp=args.out.with_suffix(args.out.suffix+'.tmp')
    temp.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
    temp.replace(args.out)
    print(json.dumps({'config':result['config_version'],'counts':result['counts'],'output':str(args.out)}))


if __name__=='__main__':
    main()
