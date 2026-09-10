#!/usr/bin/env python3
"""Install the two intended open-source font families on the developer's machine.
Font binaries are deliberately not distributed in this project archive.
Run once before packaging your own native release. No runtime download is used.
"""
from pathlib import Path
import urllib.request, hashlib, json, shutil
ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'assets/fonts'; DEST.mkdir(parents=True,exist_ok=True)
FILES={
 'LuckiestGuy-Regular.ttf':'https://raw.githubusercontent.com/google/fonts/main/apache/luckiestguy/LuckiestGuy-Regular.ttf',
 'Baloo2-Regular.ttf':'https://raw.githubusercontent.com/google/fonts/main/ofl/baloo2/Baloo2%5Bwght%5D.ttf',
 'Baloo2-OFL.txt':'https://raw.githubusercontent.com/google/fonts/main/ofl/baloo2/OFL.txt',
 'LuckiestGuy-LICENSE.txt':'https://raw.githubusercontent.com/google/fonts/main/apache/luckiestguy/LICENSE.txt',
}
report=[]
for name,url in FILES.items():
 target=DEST/name
 try:
  with urllib.request.urlopen(url,timeout=45) as response:data=response.read()
  if name.endswith('.ttf') and not (data.startswith(b'\x00\x01\x00\x00') or data.startswith(b'OTTO')):raise ValueError('Response is not a TrueType/OpenType font')
  temp=target.with_suffix(target.suffix+'.tmp');temp.write_bytes(data);temp.replace(target)
  report.append({'file':name,'source':url,'sha256':hashlib.sha256(data).hexdigest()})
 except Exception as ex:
  raise SystemExit(f'Could not install {name}: {ex}. An existing installation is not deleted.')
# Compatibility aliases for the renderer and common build layouts.
for alias,base in {'body.ttf':'Baloo2-Regular.ttf','display.ttf':'LuckiestGuy-Regular.ttf','baloo2.ttf':'Baloo2-Regular.ttf','luckiest-guy.ttf':'LuckiestGuy-Regular.ttf','Baloo2.ttf':'Baloo2-Regular.ttf','LuckiestGuy.ttf':'LuckiestGuy-Regular.ttf'}.items():
 shutil.copyfile(DEST/base,DEST/alias)
(DEST/'installed-fonts.json').write_text(json.dumps(report,indent=2)+'\n')
print('Fonts and their license records are installed locally. Rebuild to copy them into your own application package.')
