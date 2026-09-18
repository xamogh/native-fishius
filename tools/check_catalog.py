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
    errors=[];corrections=[];checks=0
    def equal(field,actual,expected):
        nonlocal checks
        checks+=1
        if actual!=expected: errors.append(dict(field=field,actual=actual,expected=expected))
    equal('schema',content['schema'],4)
    equal('minimum sell age',content['overrides']['minimum_sell_age'],1)
    equal('workbook_sha256',content['workbook_sha256'],hashlib.sha256(workbook.read_bytes()).hexdigest())
    equal('species count',len(content['species']),99)
    equal('launch coin count',sum(s['release_gate']=='Launch' and not s['companion'] for s in content['species']),40)
    for s in content['species']:
        origin=s['source'];r=sheets[origin['sheet']][origin['row']-1]
        for key,index in [('model_id',0),('name',1),('level',3),('release_gate',4)]:equal(s['id']+'.'+key,s[key],r[index])
        equal(s['id']+'.buy_xp',s['buy_xp'],0)
        if s['companion']:
            equal(s['id']+'.price',s['price'],r[6]);equal(s['id']+'.rewards',s['sale_coins']+s['sale_xp'],[0]*10)
            continue
        equal(s['id']+'.baby sale coins',s['sale_coins'][0],0)
        equal(s['id']+'.baby sale XP',s['sale_xp'][0],0)
        # Separate rational arithmetic, including each rounding boundary.
        duration=Fraction(str(r[9]))/24
        schedule=next(x for x in sheets['Schedules'][4:] if x[0]==r[5])
        profit=halfup(360*(1+Fraction(45,1000)*(r[3]-1))*duration*Fraction(str(schedule[3]))*Fraction(str(r[7])))
        xp=halfup(96*(1+Fraction(1,100)*(r[3]-1))*duration*Fraction(str(schedule[4]))*Fraction(str(r[8])))
        price=max(5,halfup(Fraction(profit,4)))
        for key,expected,cached in [('preview_profit',profit,r[10]),('preview_xp',xp,r[13]),('price',price,r[11])]:
            equal(s['id']+'.'+key,s[key],expected)
            if expected!=cached:corrections.append(dict(species=s['id'],field=key,cached=cached,exact=expected,source=origin))
        equal(s['id']+'.duration',s['duration_ms'],halfup(duration*86400000))
        equal(s['id']+'.meal',s['feed_ms'],min(s['duration_ms']//2,43200000))
        for stage,share in [(1,10),(2,40),(3,70),(4,100)]:
            coins=price+profit if stage==4 else price*95//100+profit*share//100
            earned=xp*share//100
            for key,expected in [('sale_coins',coins),('sale_xp',earned)]:
                equal(s['id']+'.'+key+str(stage),s[key][stage],expected)
                cached=s['cached_'+key][stage]
                if expected!=cached:corrections.append(dict(species=s['id'],field=key+'['+str(stage)+']',cached=cached,exact=expected,source=origin))
    for d in content['decorations']['items']:
        r=sheets['Decor Catalog'][d['source']['row']-1]
        for key,index in [('id',0),('name',1),('price',11),('buy_xp',12),('release_gate',19)]:equal(d['id']+'.'+key,d[key],r[index])
    equal('decor count',len(content['decorations']['items']),120)
    equal('XP curve',content['levels'],[r[7] for r in sheets['XP & Unlocks'][4:44]])
    equal('level rewards',content['level_rewards'],[dict(coins=r[8],pearls=r[9]) for r in sheets['XP & Unlocks'][4:44]])
    equal('tank entitlements',content['tank_entitlements'],[dict(id=r[0],tank=r[1],level=r[2],added_slots=r[3],coins=r[4],pearls=r[5],prerequisite='' if r[6]=='None' else r[6],slots=r[8]) for r in sheets['Tanks'][4:] if r[0]])
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
    return dict(checks=checks,errors=errors,rounding_corrections=corrections)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--workbook',type=Path,default=ROOT/'design/aquarium_game_design_v4.xlsx');p.add_argument('--content',type=Path,default=ROOT/'assets/content.json');p.add_argument('--out',type=Path,default=ROOT/'evidence/v4-implementation/catalog-audit.json');args=p.parse_args()
    report=audit(args.workbook,json.loads(args.content.read_text()));args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(report,indent=2)+'\n');print(f"{report['checks']} checks; {len(report['errors'])} errors; {len(report['rounding_corrections'])} documented rounding corrections");return bool(report['errors'])
if __name__=='__main__':raise SystemExit(main())
