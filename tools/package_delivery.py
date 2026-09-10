#!/usr/bin/env python3
"""Package actual files and generate an honest, machine-derived handoff report."""
from pathlib import Path
import json,zipfile,os,shutil,subprocess,hashlib,platform,datetime,re
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT.parent
E=ROOT/'evidence';E.mkdir(exist_ok=True)
font_ext={'.ttf','.otf','.woff','.woff2','.eot','.ttc','.dfont','.pfb','.pfa','.pcf','.bdf'}
private_files={'spreadsheet-skill-read.txt','previous-work-inventory.json'}
excluded_dirs={'build','.git','node_modules','__pycache__','.gradle','motion-aquarium','motion-care'}
def allowed(path):
 rel=path.relative_to(ROOT)
 if any(part in excluded_dirs for part in rel.parts):return False
 if path.suffix.lower() in font_ext or path.name in private_files:return False
 if rel.parts[0]=='third_party' and len(rel.parts)>1 and rel.parts[1] not in {'nlohmann','mobile-dependency-lock.json'}:return False
 if path.name.startswith('.env') or path.suffix.lower() in {'.keystore','.jks','.p12','.mobileprovision'}:return False
 return path.is_file()
results=[]
p=E/'build-results.json'
if p.exists():
 try:results=json.loads(p.read_text())
 except Exception:results=[]
# Record expected-but-absent stages as not run instead of omitting their absence.
expected=['configure-domain','test-domain','configure-desktop','capture-tanks','capture-shop','capture-collection','capture-settings','performance-40-fish','configure-sanitize','test-sanitize','asset-integrity']
for name in expected:
 if not any(x.get('step')==name for x in results):results.append({'step':name,'status':'not run or no result recorded','exit_code':None})
source_files=[p for p in ROOT.rglob('*') if allowed(p)]
cpp=[p for p in source_files if p.suffix in {'.cpp','.hpp','.h','.c'} and 'third_party' not in p.parts]
report={'generated_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'project_source_files':len(cpp),'cpp_source_lines':sum(len(p.read_text(errors='replace').splitlines()) for p in cpp),'packaged_file_count_before_report':len(source_files),'checks':results,'font_binaries_in_delivery':False,'android_device_verified':False,'ios_device_verified':False,'reference_fidelity_certified':False,'complete_requested_scope':False,'remaining_scope':'See docs/feature-matrix.md. Source implementation is not equivalent to verified completion.'}
# Keep all command logs, but remove accidental local access tokens if any.
for log in E.glob('*.log'):
 text=log.read_text(errors='replace')
 text=re.sub(r'([?&](?:token|access_token|sig)=)[^\s&]+',r'\1[REDACTED]',text,flags=re.I)
 log.write_text(text)
lines=['# Verification report','','This report is generated from files and recorded command results. It does not infer that a check passed from the presence of source code.','','## Recorded checks','','| Check | Status | Exit code |','|---|---|---|']
for r in results:lines.append(f"| {r.get('step','unknown')} | {r.get('status','unknown')} | {r.get('exit_code','not recorded')} |")
lines+=['','## Scope and limitations','','The full requested scope is not certified complete. The feature matrix identifies remaining systems and verification gaps. Reference fixtures use synthetic review state unless specifically measured from the supplied images. No comparison to an unavailable running original application is claimed.','','The available build host is '+platform.platform()+'. No Android or iOS device testing, signing, installation, native safe-area validation, or mobile frame-rate certification is asserted.','','Any performance JSON describes only the named native-host scenario and its recorded measurement method. A software or dummy SDL driver is not a representative mobile GPU.','','A capture file proves that a rendering command produced an image. It does not prove accurate visual composition, correct interaction, or a complete game.','','Font binaries are excluded from the delivered archive. The setup script installs fonts on the developer\'s own machine.','','Raw logs and JSON reports are retained under `evidence/`.']
(ROOT/'docs/verification.md').write_text('\n'.join(lines)+'\n')
(E/'delivery-status.json').write_text(json.dumps(report,indent=2))
archive=OUT/'aquarium-native-source.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in sorted(ROOT.rglob('*')):
  if allowed(p):z.write(p,Path('aquarium-native')/p.relative_to(ROOT))
# A binary package is produced only if an executable actually exists and answers --help.
binaries=[]
for p in (ROOT/'build').rglob('aquarium') if (ROOT/'build').exists() else []:
 if p.is_file() and os.access(p,os.X_OK):binaries.append(p)
bundle_path=None
for exe in binaries:
 try:
  test=subprocess.run([str(exe),'--help'],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
 except Exception:continue
 if test.returncode!=0:continue
 bundle=OUT/'aquarium-native-linux';bundle.mkdir(exist_ok=True)
 shutil.copy2(exe,bundle/'aquarium');(bundle/'aquarium').chmod(0o755)
 shutil.copytree(ROOT/'assets',bundle/'assets',dirs_exist_ok=True,ignore=shutil.ignore_patterns('*.ttf','*.otf','*.woff','*.woff2','*.ttc','*.eot','*.dfont'))
 libs=bundle/'lib';libs.mkdir(exist_ok=True)
 ldd=subprocess.run(['ldd',str(exe)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 for line in ldd.stdout.splitlines():
  match=re.search(r'=> (/\S+)',line)
  if not match:continue
  path=Path(match.group(1))
  # Do not redistribute the system C runtime or dynamic loader.
  if path.name.startswith(('libc.so','libm.so','libpthread.so','libdl.so','librt.so','ld-linux')):continue
  if path.is_file():shutil.copy2(path,libs/path.name)
 (bundle/'run.sh').write_text('#!/bin/sh\nset -eu\nHERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexport LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$HERE/aquarium" --assets "$HERE/assets" "$@"\n');(bundle/'run.sh').chmod(0o755)
 shutil.copy2(ROOT/'docs/verification.md',bundle/'VERIFICATION.md')
 shutil.copytree(ROOT/'licenses',bundle/'licenses',dirs_exist_ok=True)
 (bundle/'README.txt').write_text('Linux native executable. Run ./run.sh. System fonts are required unless you install the intended fonts using the source project setup script. This package is not a macOS, Android, or iOS application. A successful --help check is not a gameplay verification. Read VERIFICATION.md.\n')
 bundle_path=OUT/'aquarium-native-linux.zip'
 with zipfile.ZipFile(bundle_path,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in bundle.rglob('*'):
   if p.is_file() and p.suffix.lower() not in font_ext:z.write(p,Path('aquarium-native-linux')/p.relative_to(bundle))
 break
handoff={'source_archive':str(archive),'source_archive_bytes':archive.stat().st_size,'source_archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'linux_archive':str(bundle_path) if bundle_path else None,'verification':str(ROOT/'docs/verification.md'),'status':str(E/'delivery-status.json'),'source_file_count':len(cpp),'complete_requested_scope':False}
(OUT/'aquarium-delivery-manifest.json').write_text(json.dumps(handoff,indent=2))
print(json.dumps(handoff,indent=2))
