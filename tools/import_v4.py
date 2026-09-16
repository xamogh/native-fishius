#!/usr/bin/env python3
"""Build runtime v4 content from cached workbook values, without editing Excel.

The conversation's feeding and six-second hatch rules are explicit overrides.
Future catalogs remain data; release gates and artwork determine availability.
"""
from pathlib import Path
from decimal import Decimal, ROUND_HALF_UP
import argparse
import hashlib
import json
import math
import re
import openpyxl
from decor_animation import animation_for

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_WORKBOOK = ROOT / 'design/aquarium_game_design_v4.xlsx'


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
                    preview_profit=int(r[10]), preview_xp=int(r[13]), companion=False,
                    sale_coins=[int(r[11]),int(r[14]),int(r[16]),int(r[18]),int(r[12])],
                    sale_xp=[0,int(r[15]),int(r[17]),int(r[19]),int(r[13])],
                    badge=r[6], description=r[26], length=38, event_configured=True,
                    annual=False, one_time=False, non_resellable=False,
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
            return rounded(Decimal(str(inputs['base_'+kind+'_day']))
                           * (1+Decimal(str(inputs[kind+'_level_slope']))*(item['level']-1))
                           * Decimal(schedule['duration_ms'])/86400000
                           * item[kind+'_factor_bps']/10000*item[kind+'_weight_bps']/10000)
        item['preview_profit']=payout('coin')
        item['preview_xp']=payout('xp')
        item['price']=max(int(inputs['min_egg_price']),rounded(Decimal(item['preview_profit'])*Decimal(str(inputs['working_capital_share']))))
        item['sale_coins'][0]=item['price']
        item['sale_coins'][4]=item['price']+item['preview_profit']
        item['sale_xp'][4]=item['preview_xp']
        for stage,share in [(1,10),(2,40),(3,70)]:
            item['sale_coins'][stage]=item['price']*95//100+item['preview_profit']*share//100
            item['sale_xp'][stage]=item['preview_xp']*share//100
        species.append(item)
    for row, r in enumerate(sheets['Fish Collectibles'][4:], 5):
        if not r[0]:
            continue
        if any(numeric(r[i],r[0]) != 0 for i in [9,10,11,12]):
            raise ValueError(f'{r[0]} companion produces value')
        species.append(dict(id=camel(r[1]),model_id=r[0],name=r[1],rarity=r[2].lower(),level=int(r[3]),
                            release_gate=r[4],schedule_id='',role='Companion',coin_weight_bps=0,xp_weight_bps=0,
                            currency='gift' if r[5]=='Gift' else 'pearls',price=int(r[6]),buy_xp=0,duration_ms=0,
                            feed_ms=43200000,coin_factor_bps=0,xp_factor_bps=0,preview_profit=0,preview_xp=0,
                            companion=True,sale_coins=[0]*5,sale_xp=[0]*5,badge='Companion',description=r[14],
                            length=58,event_configured=r[7]=='None',annual=False,one_time=r[5]=='Gift',
                            non_resellable=True,source={'sheet':'Fish Collectibles','row':row}))
    for s in species:
        s['asset']='species/'+s['id']+'.png'
        s['art_ready']=(assets/s['asset']).is_file()
    if len({s['id'] for s in species}) != 99:
        raise ValueError('Expected 72 coin species and 27 unique companions')
    if sum(s['release_gate']=='Launch' and not s['companion'] for s in species)!=40:
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
        decorations.append(dict(id=r[0],name=r[1],category=r[2],theme=r[3],edition=r[4],rarity=r[5],
                                 level=int(r[6]),currency=r[7].lower(),price=int(numeric(r[11],r[0])),
                                 buy_xp=0,score=int(r[13]),width=r[14],height=r[15],size=r[16],layer=r[17],
                                 event='' if r[18]=='None' else r[18],release_gate=r[19],subcategory=r[21],
                                 availability='Permanent ownership',asset='decor/catalog/'+r[0]+'.png',art=arts[r[0]],
                                 source={'sheet':'Decor Catalog','row':row}))
    if len(decorations)!=120 or len(arts)!=120:
        raise ValueError('Expected 120 decor items and matching art briefs')
    events=[dict(name=r[1],proposed_window=r[2],coin_id=r[3],pearl_id=r[4],configured=False,starts_at=0,ends_at=0)
            for r in sheets['Events'][4:] if r[0]]
    tanks=[dict(id=r[0],tank=int(r[1]),level=int(r[2]),added_slots=int(r[3]),coins=int(r[4]),pearls=int(r[5]),
                prerequisite='' if r[6]=='None' else r[6],slots=int(r[8])) for r in sheets['Tanks'][4:] if r[0]]
    if len(tanks)!=15 or sum(t['added_slots'] for t in tanks)!=100:
        raise ValueError('Capacity does not reconcile to 100 growing slots')
    levels=[r for r in sheets['XP & Unlocks'][4:] if isinstance(r[0],(int,float))]
    if [r[0] for r in levels]!=list(range(1,71)) or [r[7] for r in levels[:2]]!=[0,80]:
        raise ValueError('Invalid v4 XP curve')
    overrides=dict(hatch_ms=6000,egg_counts_toward_growth=True,hatch_hungry=True,
                   feed_duration_share=.5,feed_duration_cap_ms=43200000,hungry_pauses_growth=True,
                   sickness=False,death=False,purchase_xp=0,backend='local',legacy_save_migration=False,
                   baby_cancel='Return principal only; zero XP before Junior',
                   source='User decisions in this task, 14 September 2026')
    checksum=hashlib.sha256(path.read_bytes()).hexdigest()
    config_checksum=hashlib.sha256(json.dumps([checksum,overrides,'R25-exact-rounding-v1'],sort_keys=True).encode()).hexdigest()
    return dict(schema=4,config_version='v4-fed-1-'+config_checksum[:12],workbook_sha256=checksum,
                config_checksum=config_checksum,overrides=overrides,inputs=inputs,
                economy=dict(base_coin_day=int(inputs['base_coin_day']),base_xp_day=int(inputs['base_xp_day']),
                             coin_slope_bps=rounded(inputs['coin_level_slope']*10000),xp_slope_bps=rounded(inputs['xp_level_slope']*10000),
                             principal_share_bps=rounded(inputs['working_capital_share']*10000),minimum_price=int(inputs['min_egg_price']),
                             early_refund_bps=rounded(inputs['early_refund_share']*10000),
                             stage_bps=[0,rounded(inputs['junior_time']*10000),rounded(inputs['young_time']*10000),rounded(inputs['mature_time']*10000),10000],
                             reward_bps=[0,rounded(inputs['junior_yield']*10000),rounded(inputs['young_yield']*10000),rounded(inputs['mature_yield']*10000),10000]),
                schedules=list(schedules.values()),species=species,counts={'coin':72,'companions':27,'launch_coin':40},
                levels=[int(r[7]) for r in levels[:40]],future_levels=[int(r[7]) for r in levels[40:]],
                level_rewards=[dict(coins=int(r[8]),pearls=int(r[9])) for r in levels[:40]],tank_entitlements=tanks,
                starter_species=['neonTetra','neonTetra','guppy','platy'],
                decorations=dict(items=decorations,events=events,tuning=dict(placed_limit=32,animated_limit=6,emitter_limit=2,
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
