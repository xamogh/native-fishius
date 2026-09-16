#!/usr/bin/env python3
"""Reimport workbook content and validate the approved artwork without replacing it."""
from pathlib import Path
import argparse,subprocess,sys,json
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--workbook',type=Path,default=ROOT/'design/aquarium_game_design_v4.xlsx');args=p.parse_args()
subprocess.run([sys.executable,str(ROOT/'tools/inspect_workbook.py'),str(args.workbook),'--out',str(ROOT/'assets/workbook.json')],check=True)
# Individual tools retain their own argparse interface; --help documents optional asset search paths.
subprocess.run([sys.executable,str(ROOT/'tools/import_content.py'),str(args.workbook),'--out',str(ROOT/'assets/content.json')],check=True)
# Canonical species illustrations are approved source assets. The native
# renderer derives outlines and dead variants without modifying their bytes.
subprocess.run([sys.executable,str(ROOT/'tools/validate_assets.py')],check=True)
