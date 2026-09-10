#!/usr/bin/env python3
"""Recreate content and artwork with failures propagated to the caller."""
from pathlib import Path
import argparse,subprocess,sys,json
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--workbook',type=Path,default=ROOT/'design/aquarium_game_master_model.xlsx');args=p.parse_args()
subprocess.run([sys.executable,str(ROOT/'tools/inspect_workbook.py'),str(args.workbook),'--out',str(ROOT/'assets/workbook.json')],check=True)
# Individual tools retain their own argparse interface; --help documents optional asset search paths.
subprocess.run([sys.executable,str(ROOT/'tools/import_content.py'),str(args.workbook),'--out',str(ROOT/'assets/content.json')],check=True)
subprocess.run([sys.executable,str(ROOT/'tools/import_supplement.py')],check=True)
subprocess.run([sys.executable,str(ROOT/'tools/prepare_art.py')],check=True)
for file in (ROOT/'assets/fish').glob('*.png'):
 if file.stem.endswith(('-dead','-mask')):continue
 with Image.open(file).convert('RGBA') as image:
  mask=Image.new('RGBA',image.size,(255,255,255,0));mask.putalpha(image.getchannel('A'));mask.save(file.with_name(file.stem+'-mask.png'))
subprocess.run([sys.executable,str(ROOT/'tools/validate_assets.py')],check=True)
