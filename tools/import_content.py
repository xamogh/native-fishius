#!/usr/bin/env python3
"""Deterministic content import. Cached workbook results are authoritative.
The source workbook is opened read-only. The JSON audit retains every source cell.
Ambiguous required numeric fields fail rather than quietly substituting game prices.
"""
from __future__ import annotations
import argparse, hashlib, json, math, re, sys
from pathlib import Path
import openpyxl

def norm(x):return re.sub(r'[^a-z0-9]+',' ',str(x or '').lower()).strip()
def number(x):return isinstance(x,(int,float)) and not isinstance(x,bool) and math.isfinite(x)
def rnd(x):return int(math.floor(x+.5))
def camel(s):
 w=re.findall(r'[A-Za-z0-9]+',s);return w[0].lower()+''.join(x[0].upper()+x[1:] for x in w[1:])
def import_sheet(w,kind,prefix):
 s=w[kind]; rows=list(s.values)
 data=[(i,r) for i,r in enumerate(rows) if any(isinstance(v,str) and re.fullmatch(prefix+r'-?\d+',v.strip()) for v in r[:3])]
 if not data:raise ValueError(f'{kind}: no {prefix} catalog rows')
 start=data[0][0]
 # The nearest header row containing the most relevant vocabulary wins.
 candidates=[]
 for i,r in enumerate(rows[:start]):
  score=sum(bool(re.search(r'\b(id|name|fish|rarity|level|buy|cost|stage|sell|xp|role|price)\b',norm(v))) for v in r)
  candidates.append((score,i))
 _,hr=max(candidates);header=[norm(v) for v in rows[hr]]
 def col(aliases, exclude=(),required=True):
  scored=[]
  for j,h in enumerate(header):
   if not h or any(e in h for e in exclude):continue
   best=max((100+len(a) if h==a else 40+len(a) if a in h else 0 for a in aliases),default=0)
   if best:scored.append((best,j))
  if not scored:
   if required:raise ValueError(f'{kind} missing {aliases}, headers={header}')
   return None
  return max(scored)[1]
 def get(r,aliases,exclude=(),required=True,default=None):
  j=col(aliases,exclude,required);return default if j is None or j>=len(r) or r[j] is None else r[j]
 out=[]
 for i,r in data:
  model=next(v.strip() for v in r[:3] if isinstance(v,str) and re.fullmatch(prefix+r'-?\d+',v.strip()))
  name=get(r,['fish name','species name','name','fish','species'],['id','badge','role','style'])
  level=get(r,['unlock level','unlock lvl','min level','level'],['reward','xp'])
  stage=get(r,['stage time hrs','stage time h','stage hours','stage h','stage time','stage duration','hours per stage','growth h'],['adult','window','grace'])
  if not number(level) or not number(stage):raise ValueError(f'{model}: nonnumeric level/stage')
  rarity=str(get(r,['rarity'],required=False,default='premium' if prefix=='PF' else 'limited' if prefix=='LE' else 'common')).lower()
  role=str(get(r,['economic role','primary role','premium role','role'],required=False,default='Collection' if prefix=='LE' else 'Balanced'))
  price=get(r,['buy coins','buy cost coins','coin price','cost coins','price coins','buy price','cost','price'],['sell','xp','hour','action','pearl'],False)
  pearls=get(r,['buy pearls','pearl price','cost pearls','price pearls','pearls','premium cost'],['per','sell','xp','value'],False)
  if prefix=='PF' and pearls is None:pearls=get(r,['buy','cost','price'],['sell','xp','hour','action'],True)
  if prefix=='LE':
   currency=str(get(r,['currency','type'],required=False,default='')).lower()
   raw=get(r,['buy cost','price','cost'],['sell','xp','hour','action','per'],False)
   if 'pearl' in currency:pearls=raw;price=None
   elif 'coin' in currency:price=raw;pearls=None
   elif 'free' in currency or 'gift' in currency:price=None;pearls=None
   if isinstance(raw,str):
    q=re.search(r'([\d,.]+)',raw)
    if q and ('pearl' in raw.lower() or 'gem' in raw.lower()):pearls=float(q[1].replace(',',''));price=None
    elif q and 'coin' in raw.lower():price=float(q[1].replace(',',''));pearls=None
  currency='pearls' if number(pearls) else 'coins' if number(price) else 'gift'
  amount=pearls if currency=='pearls' else price if currency=='coins' else 0
  buy=get(r,['buy xp','purchase xp'],['per'],True)
  adultcoins=get(r,['adult sell coins','adult coins','sell coins','adult sell','sell value'],['junior','young','mature','hour','per'],True)
  adultxp=get(r,['adult sell xp','adult xp','sell xp'],['junior','young','mature','hour','per'],True)
  if not all(number(z) for z in [amount,buy,adultcoins,adultxp]):raise ValueError(f'{model}: missing cached economy results')
  agecoins=[0];agexp=[0]
  for age,ratio in [('junior',.25),('young',.55),('mature',.8),('adult',1)]:
   cv=get(r,[age+' coins',age+' sell coins',age+' coin'],required=False)
   xv=get(r,[age+' xp',age+' sell xp'],required=False)
   if cv is None:cv=rnd(amount+(adultcoins-amount)*ratio) if currency=='coins' else rnd(adultcoins*ratio)
   if xv is None:xv=rnd(adultxp*ratio)
   if not all(number(z) for z in [cv,xv]):raise ValueError(f'{model}: missing stage cache')
   agecoins.append(rnd(cv));agexp.append(rnd(xv))
  feed=get(r,['feed window h','feeding window h','feed window','feed h'],['grace'],False,stage*2)
  grace=get(r,['sick grace h','sickness grace h','sick grace','grace h'],required=False,default=min(24,max(.5,float(feed)*.25)))
  id=camel(str(name));id={'jackOLanternPuffer':'jackOLanternPuffer'}.get(id,id)
  badge=get(r,['store badge','badge'],required=False,default='Fast Fish' if stage<1 else 'Overnight' if stage>=24 else '')
  row={'id':id,'model_id':model,'name':str(name),'rarity':rarity,'role':role,'level':int(level),'currency':currency,'price':rnd(amount),'buy_xp':rnd(buy),'stage_ms':rnd(stage*3600000),'feed_ms':rnd(float(feed)*3600000),'grace_ms':rnd(float(grace)*3600000),'sale_coins':agecoins,'sale_xp':agexp,'badge':str(badge),'description':str(get(r,['secondary trait','visual hook','description','notes'],required=False,default=role)),'length':58 if prefix=='PF' else 38,'event_start':0,'event_end':0,'annual':False,'one_time':False,'non_resellable':False,'source_row':i+1,'source_sheet':kind}
  # These event windows are configuration assumptions unless the row has explicit dates.
  events={'heartfinTetra':(207,221),'emeraldCod':(310,324),'sakuraGoldfish':(401,421),'fireworkFish':(701,714),'jackOLanternPuffer':(1017,1102),'midnightVampireSquid':(1024,1031),'santaClownfish':(1210,1231),'frostAngelfish':(1210,106),'goldenKoi':(125,208),'anniversaryRainbowfish':(901,907)}
  if prefix=='LE':
   row['event_start'],row['event_end']=events.get(id,(0,0))
   if id=='anniversaryRainbowfish' or currency=='gift':row.update(annual=True,non_resellable=True)
   row['event_copy']=str(get(r,['availability','event window','event','available'],required=False,default=''))
  out.append(row)
 return out,header

def main():
 p=argparse.ArgumentParser();p.add_argument('workbook',type=Path);p.add_argument('--out',type=Path,default=Path('assets/content.json'));a=p.parse_args()
 w=openpyxl.load_workbook(a.workbook,data_only=True)
 species=[];headers={}
 for sheet,prefix in [('Coin Fish','CF'),('Premium Fish','PF'),('Limited Edition','LE')]:
  items,h=import_sheet(w,sheet,prefix);species.extend(items);headers[sheet]=h
 counts={k:sum(s['model_id'].startswith(v) for s in species) for k,v in [('coin','CF'),('premium','PF'),('limited','LE')]}
 if counts!={'coin':26,'premium':10,'limited':10}:raise ValueError(f'Catalog count changed: {counts}; review launch scope')
 curves=[];x=w['XP Curve'];rows=list(x.values)
 hi=max(range(min(15,len(rows))),key=lambda i:sum('cumulative' in norm(c) for c in rows[i]));h=[norm(c) for c in rows[hi]]
 c=next((i for i,v in enumerate(h) if 'proposed' in v and 'cumulative' in v),None)
 if c is None:c=next(i for i,v in enumerate(h) if 'cumulative' in v)
 lc=next((i for i,v in enumerate(h) if v=='level'),0)
 for r in rows[hi+1:]:
  if lc<len(r) and c<len(r) and number(r[lc]) and number(r[c]) and 1<=r[lc]<=70:curves.append((int(r[lc]),int(r[c])))
 levels=[v for k,v in sorted(curves)]
 if len(levels)<40 or levels[:2]!=[0,80]:raise ValueError('XP curve did not import exactly')
 raw={s.title:[[c for c in r] for r in s.values] for s in w}
 result={'schema':1,'workbook_sha256':hashlib.sha256(a.workbook.read_bytes()).hexdigest(),'counts':counts,'species':species,'levels':levels[:40],'future_levels':levels[40:],'raw_sheets':raw,'headers':headers,'tank_costs':[[0,6000,12000,22000],[12000,28000,52000,85000],[40000,80000,140000,220000],[110000,220000,380000,600000],[250000,500000,850000,1300000]],'tank_levels':[1,7,16,25,34]}
 a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(result,indent=2,default=str))
 print(json.dumps({'output':str(a.out),'species':len(species),'levels':len(levels)},indent=2))
if __name__=='__main__':main()
