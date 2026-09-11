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
    hashes={}
    for s in species:
        sid=s['id']
        p=ROOT/'assets/species'/(sid+'.png')
        if not p.exists():errors.append(f'Missing {p.relative_to(ROOT)}');continue
        with Image.open(p) as im:im.verify()
        with Image.open(p) as im:
            if 'A' not in im.getbands():errors.append(f'{sid}: image has no alpha channel');continue
            alpha=im.getchannel('A');hist=alpha.histogram();pixels=im.width*im.height
            if hist[0]<pixels*.05:errors.append(f'{sid}: no meaningful transparent background')
            if sum(hist[128:])<pixels*.03:errors.append(f'{sid}: no meaningful visible silhouette')
            digest=hashlib.sha256(p.read_bytes()).hexdigest()
            if digest in hashes:errors.append(f'{sid}: duplicate artwork of {hashes[digest]}')
            hashes[digest]=sid
            provenance=ROOT/'assets/species/provenance'/(sid+'.json')
            if not provenance.exists():errors.append(f'{sid}: missing provenance')
            else:
                record=json.loads(provenance.read_text())
                if record.get('sha256') and record['sha256']!=digest:errors.append(f'{sid}: provenance hash mismatch')
            rows.append({'id':sid,'path':str(p.relative_to(ROOT)),'width':im.width,'height':im.height,'transparent_fraction':hist[0]/pixels,'sha256':digest})
    ids=[s['id'] for s in species]
    if len(set(ids))!=len(ids):errors.append('Duplicate species IDs')
except Exception as ex:errors.append(str(ex))
(ROOT/'evidence/asset-validation.json').write_text(json.dumps({'passed':not errors,'errors':errors,'runtime_derivatives':'Canvas derives matching dead and mask textures from each canonical source. Native UI tests verify them.','images':rows},indent=2))
print(json.dumps({'passed':not errors,'errors':errors,'images_checked':len(rows)},indent=2))
sys.exit(bool(errors))
