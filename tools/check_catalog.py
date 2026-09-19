#!/usr/bin/env python3
"""Independently audit v4 source rows and exact integer reward calculations."""
import argparse
import hashlib
import json
from fractions import Fraction
from pathlib import Path
import openpyxl
ROOT=Path(__file__).resolve().parents[1]
def halfup(value):
    return (value.numerator*2+value.denominator)//(2*value.denominator)
def audit(workbook, content):
    book=openpyxl.load_workbook(workbook,read_only=True,data_only=True)
    try:
        sheets={s.title:list(s.values) for s in book}
    finally:
        book.close()
    errors=[];corrections=[];tuning_changes=[];checks=0
    def equal(field,actual,expected):
        nonlocal checks
        checks+=1
        if actual!=expected: errors.append(dict(field=field,actual=actual,expected=expected))
    equal('schema',content['schema'],4)
    equal('minimum sell age',content['overrides']['minimum_sell_age'],1)
    equal('Keep action retired',content['overrides']['keep_action'],False)
    equal('shared fish capacity',content['overrides']['fish_capacity'],'shared')
    equal('fish purchase XP',content['overrides']['fish_purchase_xp'],0)
    equal('decor purchase XP policy',content['overrides']['decor_purchase_xp'],
          dict(coins_per_xp=10,xp_per_pearl=10,minimum_xp=1,rounding='half_up',first_purchase_only=True))
    equal('placed decor override',content['overrides']['decor_placed_limit'],64)
    equal('placed decor per tank',content['decorations']['tuning']['placed_limit'],64)
    equal('workbook_sha256',content['workbook_sha256'],hashlib.sha256(workbook.read_bytes()).hexdigest())
    equal('species count',len(content['species']),99)
    equal('shared fish lifecycle',content['overrides']['fish_lifecycle'],'shared')
    equal('pearl fish schedule',content['overrides']['pearl_fish_schedule'],'SC-2H')
    equal('Bubble Eye pearl price',content['overrides']['bubble_eye_pearls'],1)
    equal('repeat pearl purchases',content['overrides']['pearl_fish_repeatable'],True)
    equal('active XP base',content['economy']['base_xp_day'],576)
    equal('active XP slope',content['economy']['xp_slope_bps'],1500)
    equal('renewable pearl reward',content['pearl_earning'],dict(sales_target=20,reward=1))
    equal('launch coin count',sum(s['release_gate']=='Launch' and s['currency']=='coins' for s in content['species']),40)
    equal('pearl species count',sum(s['currency']=='pearls' for s in content['species']),27)
    for s in content['species']:
        origin=s['source'];r=sheets[origin['sheet']][origin['row']-1]
        for key,index in [('model_id',0),('name',1),('level',3),('release_gate',4)]:equal(s['id']+'.'+key,s[key],r[index])
        equal(s['id']+'.buy_xp',s['buy_xp'],0)
        pearl=origin['sheet']=='Fish Collectibles'
        equal(s['id']+'.currency',s['currency'],'pearls' if pearl else 'coins')
        equal(s['id']+'.repeatable',s['one_time'],False)
        equal(s['id']+'.baby sale coins',s['sale_coins'][0],0)
        equal(s['id']+'.baby sale XP',s['sale_xp'][0],0)
        # Separate rational arithmetic, including each rounding boundary.
        duration=Fraction(2,24) if pearl else Fraction(halfup(Fraction(str(r[9]))*3600000),86400000)
        schedule=next(x for x in sheets['Schedules'][4:] if x[0]==('SC-2H' if pearl else r[5]))
        equal(s['id']+'.schedule',s['schedule_id'],schedule[0])
        profit=halfup(360*(1+Fraction(45,1000)*(r[3]-1))*duration*Fraction(str(schedule[3]))*(1 if pearl else Fraction(str(r[7]))))
        xp=halfup(576*(1+Fraction(15,100)*(r[3]-1))*duration*Fraction(str(schedule[4]))*(1 if pearl else Fraction(str(r[8]))))
        principal=0 if pearl else max(5,halfup(Fraction(profit,4)))
        price=(1 if r[0]=='PF-01' else (r[6]+5)//6) if pearl else principal
        for key,expected,cached in [('preview_profit',profit,r[10]),('preview_xp',xp,r[13]),('price',price,r[6] if pearl else r[11])]:
            equal(s['id']+'.'+key,s[key],expected)
            if expected!=cached and (not pearl or key=='price'):
                (tuning_changes if pearl or key=='preview_xp' else corrections).append(dict(species=s['id'],field=key,cached=cached,exact=expected,source=origin))
        equal(s['id']+'.duration',s['duration_ms'],halfup(duration*86400000))
        equal(s['id']+'.meal',s['feed_ms'],min(s['duration_ms']//2,43200000))
        for stage,share in [(1,10),(2,40),(3,70),(4,100)]:
            coins=principal+profit if stage==4 else principal*95//100+profit*share//100
            earned=xp*share//100
            for key,expected in [('sale_coins',coins),('sale_xp',earned)]:
                equal(s['id']+'.'+key+str(stage),s[key][stage],expected)
                if not pearl:
                    cached=s['cached_'+key][stage]
                    if expected!=cached:(tuning_changes if key=='sale_xp' else corrections).append(dict(species=s['id'],field=key+'['+str(stage)+']',cached=cached,exact=expected,source=origin))
    for d in content['decorations']['items']:
        r=sheets['Decor Catalog'][d['source']['row']-1]
        for key,index in [('id',0),('name',1),('price',11),('release_gate',19)]:equal(d['id']+'.'+key,d[key],r[index])
        expected_xp=max(1,halfup(Fraction(r[11],10))) if r[7]=='Coins' else r[11]*10
        equal(d['id']+'.buy_xp',d['buy_xp'],expected_xp)
    equal('decor count',len(content['decorations']['items']),120)
    equal('XP curve',content['levels'],[r[7] for r in sheets['XP & Unlocks'][4:44]])
    equal('level rewards',content['level_rewards'],[dict(coins=r[8],pearls=r[9]) for r in sheets['XP & Unlocks'][4:44]])
    source_tanks=[r for r in sheets['Tanks'][4:] if r[0]]
    expected_tanks=[dict(id=r[0],tank=r[1],level=r[2] if r[8]==10 else 0,added_slots=r[3],coins=r[4],pearls=r[5],prerequisite='' if r[6]=='None' else r[6],slots=r[8]) for r in source_tanks]
    for tank in range(1,6):
        base=next(r for r in source_tanks if r[1]==tank and r[8]==20)
        for capacity in (25,30,35,40):
            multiple=Fraction(capacity-10,10)
            expected_tanks.append(dict(id=f'TK-{tank:02d}-{capacity}',tank=tank,level=0,added_slots=5,
                                       coins=halfup(base[4]*multiple),pearls=halfup(base[5]*multiple),
                                       prerequisite=f'TK-{tank:02d}-{capacity-5}',slots=capacity))
    equal('owned tank upgrades have no level gate',content['overrides']['tank_upgrade_level_gate'],False)
    equal('tank capacity steps',content['overrides']['tank_capacity_steps'],[10,15,20,25,30,35,40])
    for row in expected_tanks:
        index=row['tank']-1
        if row['slots']==10:
            row['coins']=[0,1500,6000,15000,30000][index]
            row['pearls']=[0,5,10,15,20][index]
        else:
            step=(row['slots']-15)//5
            row['coins']=halfup([300,900,1800,3000,4500,6300][step]*Fraction(2+index,2))
            row['pearls']=halfup([2,3,4,5,6,7][step]*Fraction(2+index,2))
    equal('tank entitlements',content['tank_entitlements'],expected_tanks)
    equal('Treasure reference capacity',content['treasure']['reference_slots_by_level'],
          [sum(r[3] for r in source_tanks if r[2]<=level) for level in range(1,41)])
    offers=content['treasure']['offers']
    for offer in offers:
        row=sheets['Shop'][offer['source']['row']-1]
        equal(offer['id']+'.unlock level',offer['level'],0)
        equal(offer['id']+'.eligibility',offer['eligibility'],row[7])
        equal(offer['id']+'.available from start',row[7],'Level 0; available from start')
    for kind in ('coins','pearls'):
        group=[offer for offer in offers if offer['kind']==kind]
        equal(kind+' pack count',len(group),5)
        previous=None
        for offer in group:
            row=sheets['Shop'][offer['source']['row']-1]
            for key,index in [('id',0),('name',2),('pearls',4)]:equal(offer['id']+'.'+key,offer[key],row[index])
            equal(offer['id']+'.price cents',offer['price_usd_cents'],halfup(Fraction(str(row[3]))*100))
            equal(offer['id']+'.coin days',offer['coin_days_bps'],halfup(Fraction(str(row[5]))*10000))
            value=Fraction(offer['pearls'] if kind=='pearls' else offer['coin_days_bps'],offer['price_usd_cents'])
            if previous is not None:equal(offer['id']+'.better value',value>previous,True)
            previous=value
    return dict(checks=checks,errors=errors,rounding_corrections=corrections,active_play_changes=tuning_changes)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--workbook',type=Path,default=ROOT/'design/aquarium_game_design_v4.xlsx');p.add_argument('--content',type=Path,default=ROOT/'assets/content.json');p.add_argument('--out',type=Path,default=ROOT/'evidence/v4-implementation/catalog-audit.json');args=p.parse_args()
    report=audit(args.workbook,json.loads(args.content.read_text()));args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['checks']} checks; {len(report['errors'])} errors; {len(report['rounding_corrections'])} documented rounding corrections");return bool(report['errors'])
if __name__=='__main__':raise SystemExit(main())
