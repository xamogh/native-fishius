#!/usr/bin/env python3
"""Run real build and validation steps, retaining all exit statuses and output.
Nothing in this script declares an unexecuted check successful.
"""
from pathlib import Path
import json, subprocess, time, shutil, os, sys, tarfile, zipfile, re, hashlib
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'evidence'; E.mkdir(exist_ok=True)
results=[]
def run(name, command, timeout=240, env=None, cwd=ROOT):
    start=time.time()
    try:
        r=subprocess.run(command,cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout,env=env)
        code=r.returncode; text=r.stdout
    except subprocess.TimeoutExpired as ex:
        code=124; text=(ex.stdout or b'').decode(errors='replace') if isinstance(ex.stdout,bytes) else (ex.stdout or '')
        text+='\nSTEP TIMED OUT. This is a failure, not a pass.\n'
    except Exception as ex:
        code=127; text=str(ex)
    (E/(name+'.log')).write_text(text,encoding='utf-8')
    results.append({'step':name,'command':command,'exit_code':code,'seconds':round(time.time()-start,3),'status':'passed' if code==0 else 'failed','log':name+'.log'})
    (E/'build-results.json').write_text(json.dumps(results,indent=2))
    return code,text

def prepare_deps():
    # Header-only JSON has an exact release pin. Local cached headers are preferred.
    dst=ROOT/'third_party/nlohmann/json.hpp'; dst.parent.mkdir(parents=True,exist_ok=True)
    if not dst.exists():
        found=[]
        for base in [Path('/usr/include'),Path('/usr/local/include'),Path('/mnt/data')]:
            if base.exists():
                found.extend(p for p in base.glob('**/nlohmann/json.hpp') if p!=dst)
        if found: shutil.copyfile(found[0],dst)
        else: run('fetch-json',['curl','--fail','--location','--retry','2','--max-time','60','https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp','-o',str(dst)],90)
    if dst.exists():
        (E/'json-header-sha256.txt').write_text(hashlib.sha256(dst.read_bytes()).hexdigest()+'\n')

def common_compile_fixes(log):
    """Only mechanically justified portable fixes. Every edit is recorded."""
    fixes=[]
    if 'TTF_GetError' in log:
        for p in (ROOT/'src').glob('*.cpp'):
            text=p.read_text()
            if 'TTF_GetError()' in text:
                p.write_text(text.replace('TTF_GetError()','SDL_GetError()')); fixes.append(str(p.relative_to(ROOT))+': SDL3_ttf error accessor')
    if 'SDL_GetAudioStreamQueued' in log and ('not declared' in log or 'no member' in log):
        # Do not guess an alternative API or remove queue bounds.
        pass
    # Missing standard includes are safe when the compiler names the corresponding symbol.
    standards={'std::clamp':'algorithm','std::sort':'algorithm','std::find_if':'algorithm','std::isfinite':'cmath','std::sqrt':'cmath','std::exchange':'utility','std::set':'set','std::unordered_set':'unordered_set','std::array':'array','std::span':'span','std::numeric_limits':'limits','std::ifstream':'fstream','std::ofstream':'fstream','std::function':'functional','std::memcpy':'cstring'}
    if 'error:' in log:
        for p in list((ROOT/'src').glob('*.cpp'))+list((ROOT/'include/aquarium').glob('*.hpp'))+list((ROOT/'tests').glob('*.cpp')):
            t=p.read_text()
            extra=[h for sym,h in standards.items() if sym in t and f'#include <{h}>' not in t and (sym.split('::')[-1] in log)]
            if extra:
                t=''.join(f'#include <{h}>\n' for h in sorted(set(extra)))+t
                p.write_text(t); fixes.append(str(p.relative_to(ROOT))+': explicit standard headers '+','.join(sorted(set(extra))))
    if fixes:
        with (E/'mechanical-fixes.txt').open('a') as f: f.write('\n'.join(fixes)+'\n')
    return bool(fixes)

def build_target(preset):
    code,log=run('configure-'+preset,['cmake','--preset',preset],420)
    if code:return False
    for attempt in range(3):
        code,log=run('build-'+preset+'-'+str(attempt+1),['cmake','--build','--preset',preset,'--parallel','4'],420)
        if code==0:return True
        if not common_compile_fixes(log):break
    return False

def main():
    prepare_deps()
    if shutil.which('clang-format'):
        files=list((ROOT/'src').glob('*.cpp'))+list((ROOT/'include/aquarium').glob('*.hpp'))+list((ROOT/'tests').glob('*.cpp'))
        run('format',['clang-format','-i',*[str(p) for p in files]],90)
    ok=build_target('domain')
    if ok:run('test-domain',['ctest','--preset','domain','--output-on-failure'],120)
    ok=build_target('desktop')
    if ok:
        candidates=[ROOT/'build/desktop/aquarium'] if (ROOT/'build/desktop/aquarium').is_file() else []
        if candidates:
            exe=candidates[0]
            env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
            for state in ['tanks','shop','tank-switcher','settings']:
                run('capture-'+state,[str(exe),'--software','--fixture',state,'--width','1088','--height','635','--frames','20','--capture',str(E/(state+'.png'))],90,env)
            for w,h in [(640,360),(667,375),(844,390),(932,430),(1024,768)]:
                run(f'layout-{w}x{h}',[str(exe),'--software','--fixture','shop','--width',str(w),'--height',str(h),'--frames','10','--capture',str(E/f'shop-{w}x{h}.png')],90,env)
            run('performance-40-fish',[str(exe),'--software','--fixture','performance','--width','1088','--height','635','--frames','600','--report',str(E/'performance-40-fish.json')],120,env)
            for state in ['aquarium','care']:
                dest=E/('motion-'+state);dest.mkdir(exist_ok=True)
                run('motion-'+state,[str(exe),'--software','--fixture',state,'--width','844','--height','490','--frames','120','--sequence',str(dest)],120,env)
                pngs=sorted(dest.glob('*.png'))
                if pngs and shutil.which('ffmpeg'):
                    # Frame rate here describes playback only. It is not a measured mobile frame rate.
                    with (dest/'frames.txt').open('w') as f:
                        for p in pngs:f.write("file '"+str(p)+"'\nduration 0.016666667\n")
                    run('encode-'+state,['ffmpeg','-y','-f','concat','-safe','0','-i',str(dest/'frames.txt'),'-vf','format=yuv420p','-movflags','+faststart',str(E/('motion-'+state+'.mp4'))],90)
    ok=build_target('sanitize')
    if ok:
        env=os.environ.copy();env['ASAN_OPTIONS']='detect_leaks=1:halt_on_error=1';env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
        run('test-sanitize',['ctest','--preset','sanitize','--output-on-failure'],120,env)
    # Structural checks are explicitly not execution or correctness tests.
    run('asset-integrity',[sys.executable,str(ROOT/'tools/validate_assets.py')],90)
    (E/'environment.json').write_text(json.dumps({'platform':sys.platform,'python':sys.version,'machine':os.uname().machine if hasattr(os,'uname') else 'unknown','display':os.getenv('DISPLAY'),'mobile_sdks_verified':False,'performance_note':'Only logs and measured native-host reports are evidence. No mobile device performance or visual parity is asserted.'},indent=2))
    return 0 if all(r['exit_code']==0 for r in results) else 1
if __name__=='__main__':sys.exit(main())
