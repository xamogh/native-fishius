#!/usr/bin/env python3
"""Obtain pinned native source dependencies, including SDL's Android Java host.
This is a build-machine operation. The shipped game never downloads resources.
"""
from pathlib import Path
import subprocess, json, sys, re
ROOT=Path(__file__).resolve().parents[1]
SOURCES=[('SDL','https://github.com/libsdl-org/SDL.git','release-3.2.20'),('SDL_image','https://github.com/libsdl-org/SDL_image.git','release-3.2.4'),('SDL_ttf','https://github.com/libsdl-org/SDL_ttf.git','release-3.2.2')]
manifest=[]
for name,url,tag in SOURCES:
 p=ROOT/'third_party'/name
 if p.exists() and not (p/'.git').is_dir():raise SystemExit(f'{p} already exists and is not a git checkout. It is preserved. Move it aside deliberately before bootstrapping.')
 if not p.exists():subprocess.run(['git','clone','--depth','1','--branch',tag,'--recurse-submodules','--shallow-submodules',url,str(p)],check=True)
 actual=subprocess.check_output(['git','-C',str(p),'describe','--tags','--exact-match'],text=True).strip()
 if actual!=tag:raise SystemExit(f'{name}: expected {tag}, found {actual}. Existing dependency preserved.')
 subprocess.run(['git','-C',str(p),'submodule','update','--init','--recursive','--depth','1'],check=True)
 sha=subprocess.check_output(['git','-C',str(p),'rev-parse','HEAD'],text=True).strip()
 manifest.append({'name':name,'version':tag,'commit':sha})
(ROOT/'third_party/mobile-dependency-lock.json').write_text(json.dumps(manifest,indent=2))
print('Pinned native source dependencies and SDL Android Java sources are installed.')
print('Native platform compilation and signing still require the corresponding SDKs.')
