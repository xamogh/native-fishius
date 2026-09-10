#!/usr/bin/env python3
"""Create transparent review evidence without claiming visual parity.
Reference images are not used as the interactive application's background.
"""
from pathlib import Path
import json, shutil
from PIL import Image, ImageOps, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'evidence';E.mkdir(exist_ok=True)
refs=[]
for p in Path('/mnt/data').rglob('ChatGPT Image Sep 8, 2026, 04_25_18 PM (*).png'):
 if ROOT not in p.parents:refs.append(p)
refs=sorted(refs,key=lambda p:p.name)
seen=set();states=['tanks','shop','collection','settings'];items=[]
for p in refs:
 if p.name in seen:continue
 seen.add(p.name)
 idx=len(items)
 if idx>=4:break
 with Image.open(p) as im:
  im=im.convert('RGB');ref=E/('reference-'+states[idx]+'.png');im.save(ref)
  record={'state':states[idx],'source_filename':p.name,'width':im.width,'height':im.height,'reference':ref.name,'source_assignment':'Prior reference inspection identified Tanks, Shop, Collection, Settings in attachment order.','comparison_status':'Not visually certified','measured_geometry':'Only image dimensions are automatically measured here. Aquarium, panel, font and safe-area geometry require visual review.'}
  cap=E/(states[idx]+'.png')
  if cap.exists():
   with Image.open(cap) as native:
    native=native.convert('RGB')
    width=1000;height=650
    canvas=Image.new('RGB',(width*2, height+70),(245,248,251));d=ImageDraw.Draw(canvas)
    d.text((20,15),'SUPPLIED REFERENCE',(20,35,50));d.text((width+20,15),'NATIVE CAPTURE: REVIEW REQUIRED',(20,35,50))
    a=ImageOps.contain(im,(width-30,height));b=ImageOps.contain(native,(width-30,height))
    canvas.paste(a,((width-a.width)//2,60+(height-a.height)//2));canvas.paste(b,(width+(width-b.width)//2,60+(height-b.height)//2))
    dest=E/('comparison-'+states[idx]+'.png');canvas.save(dest);record['side_by_side']=dest.name
    # A full-image overlay is only a diagnostic. Device chrome and synthetic
    # fixture state make a global pixel score inappropriate as an acceptance test.
    normalized=ImageOps.contain(native,im.size)
    overlaybase=Image.new('RGB',im.size,(0,0,0));overlaybase.paste(normalized,((im.width-normalized.width)//2,(im.height-normalized.height)//2))
    Image.blend(im,overlaybase,.5).save(E/('overlay-'+states[idx]+'.png'))
    record['overlay']=f'overlay-{states[idx]}.png';record['pixel_parity_test']='Not evaluated: full-image alignment is not an acceptance metric.'
  else:record['native_capture']='Not produced'
  items.append(record)
(E/'reference-review.json').write_text(json.dumps({'references':items,'matched':False,'note':'Image dimensions and review composites are evidence of files, not proof of visual fidelity or animation parity.'},indent=2))
print(json.dumps(items,indent=2))
