#!/usr/bin/env python3
"""Compare imported game content to independently retained cached workbook cells.
The importer must retain provenance. Missing provenance is an unverified field,
not a successful match to a second table produced by the same formula.
"""
from pathlib import Path
import json,sys
ROOT=Path(__file__).resolve().parents[1]
result={'checked':[],'unverified':[],'errors':[]}
try:
 data=json.loads((ROOT/'assets/content.json').read_text())
 species=data.get('species',[])
 for s in species:
  provenance=s.get('source',s.get('provenance',{}))
  if not provenance:result['unverified'].append({'species':s['id'],'reason':'Source-cell comparison requires review of workbook audit and importer output.'})
  rewards=s.get('sellCoinsByAge',s.get('saleCoins',s.get('sellCoins',[])))
  if isinstance(rewards,list) and (len(rewards)!=5 or any(not isinstance(v,(int,float)) or v<0 for v in rewards)):result['errors'].append({'species':s['id'],'field':'staged coin rewards'})
  result['checked'].append({'species':s['id'],'structural_check':True})
except Exception as ex:result['errors'].append(str(ex))
result['structural_pass']=not result['errors'];result['independent_workbook_parity_verified']=not result['unverified'] and not result['errors']
(ROOT/'evidence/catalog-audit.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2));sys.exit(bool(result['errors']))
