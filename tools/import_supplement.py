#!/usr/bin/env python3
"""Import recognizable supplemental tables with cell provenance.
Unresolved reward fields remain unresolved. No invented currency replaces them.
"""
import json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];p=ROOT/'assets/content.json'
if not p.exists():raise SystemExit('Import catalog first')
c=json.loads(p.read_text());raw=c['raw_sheets'];audit=[]
def n(v):return re.sub('[^a-z0-9]+',' ',str(v or '').lower()).strip()
def numeric(v):return isinstance(v,(int,float)) and not isinstance(v,bool)
quests=[('daily-feed','feed',False),('daily-sell','sell',False),('daily-adult','adult',False),('daily-decor','decor',False),('daily-gift','gift',False),('weekly-collection','collection',True),('weekly-care','feed',True)]
by_level={str(i):{} for i in range(1,41)}
rows=raw.get('Quest Scaling',[])
# Handle common wide tables: one level per row, rewards in named columns.
header_row=None;best=-1
for index,row in enumerate(rows):
 h=[n(x) for x in row];score=sum(any(word in x for word in ['coin','xp','reward','target']) for x in h)
 if any(x in ['level','player level','level min','min level','lv'] for x in h) and score>best:header_row=index;best=score
if header_row is not None:
 headers=[n(x) for x in rows[header_row]];lc=next(i for i,x in enumerate(headers) if x in ['level','player level','level min','min level','lv'])
 for rindex,row in enumerate(rows[header_row+1:],header_row+2):
  if lc>=len(row) or not numeric(row[lc]) or not 1<=row[lc]<=70:continue
  level=int(row[lc])
  if level>40:continue
  for qid,event,weekly in quests:
   result={};sources={}
   for field,aliases in [('coins',['coin']),('xp',['xp']),('tokens',['token']),('pearls',['pearl']),('target',['target','count','require','goal'])]:
    choices=[]
    for i,h in enumerate(headers):
     if i>=len(row) or not numeric(row[i]) or not any(a in h for a in aliases):continue
     if any(x in h for x in ['per hour','rate','frontier','fraction','factor','multiplier','total budget']):continue
     period= 'weekly' if weekly else 'daily'
     opposite='daily' if weekly else 'weekly'
     if opposite in h:continue
     tags=[event]
     if event=='sell':tags+=['junior','harvest']
     if event=='collection':tags+=['collect','species']
     if event=='decor':tags+=['plant','decorate']
     score=10 if period in h else 0
     if any(t in h for t in tags):score+=20
     if 'reward' in h:score+=3
     if score>=10:choices.append((score,i))
    if choices:
     _,i=max(choices);result[field]=int(round(row[i]));sources[field]={'sheet':'Quest Scaling','row':rindex,'column':i+1,'heading':headers[i]}
   if any(k in result for k in ['coins','xp','tokens','pearls']):
    result['source']=sources;by_level[str(level)][qid]=result
# Interpret long tables only where an objective and explicit reward columns coexist.
for sheet in ['Quest Scaling','Quests & Live Ops']:
 rows=raw.get(sheet,[])
 for hi,row in enumerate(rows):
  headers=[n(v) for v in row]
  coincols=[i for i,h in enumerate(headers) if 'coin' in h and not any(x in h for x in ['rate','budget','cost'])]
  xpcols=[i for i,h in enumerate(headers) if 'xp' in h and not any(x in h for x in ['cumulative','rate','total'])]
  if not coincols and not xpcols:continue
  for ri,data in enumerate(rows[hi+1:],hi+2):
   text=' '.join(n(v) for v in data if isinstance(v,str))
   if not text:continue
   for qid,event,weekly in quests:
    if event not in text and not(event=='sell' and 'junior' in text) and not(event=='collection' and 'collect' in text):continue
    if weekly!=('weekly' in text):continue
    # Only use an explicit target/reward row, not descriptive paragraphs.
    vals={};provenance={}
    for field,cols in [('coins',coincols),('xp',xpcols)]:
     if len(cols)==1 and cols[0]<len(data) and numeric(data[cols[0]]):vals[field]=int(round(data[cols[0]]));provenance[field]={'sheet':sheet,'row':ri,'column':cols[0]+1,'heading':headers[cols[0]]}
    if vals:
     vals['source']=provenance
     for level in range(1,41):
      if qid not in by_level[str(level)]:by_level[str(level)][qid]=vals
onboarding={};rows=raw.get('Onboarding',[])
for hi,row in enumerate(rows):
 h=[n(v) for v in row];xp=[i for i,s in enumerate(h) if 'xp' in s and not any(z in s for z in ['total','cumulative','running','threshold'])]
 if len(xp)!=1:continue
 col=xp[0]
 for ri,data in enumerate(rows[hi+1:],hi+2):
  if col>=len(data) or not numeric(data[col]) or data[col]<0:continue
  text=' '.join(n(v) for v in data if isinstance(v,str))
  for action,terms in [('feed',['feed']),('place',['place','placement']),('decor',['decor','plant']),('gift',['send a gift','send gift'])]:
   if not any(term in text for term in terms):continue
   if action=='place' and 'buy' in text:continue
   if any(term in text for term in ['total','level 2 reward','quest reward']):continue
   if action not in onboarding:onboarding[action]={'xp':int(round(data[col])),'source':{'sheet':'Onboarding','row':ri,'column':col+1,'heading':h[col]}}
 break
c['supplement']={'quests_by_level':by_level,'onboarding_rewards':onboarding,'import_status':{'quest_profiles':sum(bool(v) for v in by_level.values()),'onboarding_bonus_actions':list(onboarding)}}
p.write_text(json.dumps(c,indent=2));(ROOT/'evidence/supplement-import.json').write_text(json.dumps(c['supplement'],indent=2))
print(json.dumps(c['supplement']['import_status']))
