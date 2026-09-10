#!/usr/bin/env python3
"""Validate packaged image bytes and catalog coverage, independently of the renderer."""
from pathlib import Path
import json, hashlib, sys
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
errors=[];rows=[]
try:
    content=json.loads((ROOT/'assets/content.json').read_text())
    species=content.get('species',content.get('catalog',[]))
    if isinstance(species,dict):species=list(species.values())
    if len(species)!=46:errors.append(f'Expected 46 definitions, found {len(species)}')
    for s in species:
        sid=s['id']
        for suffix in ['', '-dead', '-mask']:
            p=ROOT/'assets/fish'/(sid+suffix+'.png')
            if not p.exists():errors.append(f'Missing {p.relative_to(ROOT)}');continue
            with Image.open(p) as im:
                im.verify()
            with Image.open(p) as im:
                if 'A' not in im.getbands():errors.append(f'{sid}{suffix}: image has no alpha channel')
                rows.append({'path':str(p.relative_to(ROOT)),'width':im.width,'height':im.height,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
    ids=[s['id'] for s in species]
    if len(set(ids))!=len(ids):errors.append('Duplicate species IDs')
except Exception as ex:errors.append(str(ex))
(ROOT/'evidence/asset-validation.json').write_text(json.dumps({'passed':not errors,'errors':errors,'images':rows},indent=2))
print(json.dumps({'passed':not errors,'errors':errors,'images_checked':len(rows)},indent=2))
sys.exit(bool(errors))
