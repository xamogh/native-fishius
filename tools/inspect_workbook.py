#!/usr/bin/env python3
"""Export every nonempty cell, its cached value and formula without changing the model."""
import argparse,json,hashlib
from pathlib import Path
import openpyxl
p=argparse.ArgumentParser();p.add_argument('workbook',type=Path);p.add_argument('--out',type=Path,default=Path('assets/workbook.json'));a=p.parse_args()
v=openpyxl.load_workbook(a.workbook,data_only=True,read_only=False)
f=openpyxl.load_workbook(a.workbook,data_only=False,read_only=False)
result={'sha256':hashlib.sha256(a.workbook.read_bytes()).hexdigest(),'sheets':{}}
for s in v:
 rows=[]
 for row in s:
  vals=[]
  for c in row:
   z=f[s.title][c.coordinate].value
   if c.value is not None or z is not None:
    vals.append({'cell':c.coordinate,'column':c.column,'value':c.value,'formula':z if isinstance(z,str) and z.startswith('=') else None})
  if vals:rows.append({'row':row[0].row,'cells':vals})
 result['sheets'][s.title]=rows
 a.out.parent.mkdir(parents=True,exist_ok=True)
a.out.write_text(json.dumps(result,indent=2,default=str),encoding='utf8')
print(f'Exported {len(result["sheets"])} sheets to {a.out}')
