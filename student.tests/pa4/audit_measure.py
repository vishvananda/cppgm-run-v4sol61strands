#!/usr/bin/env python3
"""Supplement the frozen PA4 protocol with scale controls and sampled ownership."""
import hashlib
import json
import os
from pathlib import Path
import re
import statistics
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
PERF=Path(sys.argv[1]).resolve()
A=Path(sys.argv[2]).resolve()
A.mkdir(exist_ok=False)
release=PERF/(sys.argv[3] if len(sys.argv)>3 else 'audit-certified')
base=PERF/'audit-base'
scale=A.parent/'scale-qualified'
inputs=json.loads((scale/'inputs.json').read_text())
fixture=ROOT/'pa4/tests/directives/600-repeated-argument-expansion.t'
inputs['slowest-fixture']={'path':str(fixture),'expected':64,
                          'bytes':fixture.stat().st_size,'sha256':hashlib.sha256(fixture.read_bytes()).hexdigest()}
cpu=min(os.sched_getaffinity(0))
commands={'cursor':[str(release/'cursor')], 'before_cursor':[str(base/'cursor')],
          'gcc':['g++','-std=c++11','-E','-P','-x','c++'],
          'clang':['clang++','-std=c++11','-E','-P','-x','c++'],
          'reference':[str(ROOT/'dev/preproc-ref'),'-o',str(A/'dump')]}
def run(cmd,**kw):
    return subprocess.run(list(map(str,cmd)),check=True,**kw)
def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()
manifest={'cpu':cpu,'parent_manifest':json.loads((release/'manifest.json').read_text()),
          'commands':commands, 'inputs':inputs, 'binaries':{'cursor':sha(release/'cursor'),
          'before_cursor':sha(base/'cursor')},'warmup':'one unmeasured invocation; interleaved three repeats',
          'executable_runtime_text_size':'N/A: preprocessing produces no native object'}
(A/'manifest.json').write_text(json.dumps(manifest,indent=2))
raw=[]
parity={}
for name,v in inputs.items():
    parity[name]={}
    for variant,cmd in commands.items():
        r=run(['taskset','-c',cpu,*cmd,v['path']],capture_output=True,text=True)
        if variant in ('cursor','before_cursor'):
            ok=int(r.stdout)==v['expected']
        elif variant in ('gcc','clang'):
            ok=r.stdout.split()==['done']*v['expected']
        else:
            ok=(A/'dump').read_text().splitlines()[2:-1]==['identifier done']*v['expected']
        assert ok,(name,variant)
        parity[name][variant]=ok
        (A/f'{name}-{variant}-warmup.stderr').write_text(r.stderr)
    for repeat in range(3):
        for variant,cmd in commands.items():
            stem=A/f'{name}-{variant}-{repeat}'
            measured=['perf','stat','-x',';','-o',str(stem)+'.perf','-e','{instructions:u,cycles:u}',
                      '--','/usr/bin/time','-f','%e %U %S %M','-o',str(stem)+'.time',
                      'taskset','-c',cpu,*cmd,v['path']]
            with open(str(stem)+'.stderr','w') as err:
                run(measured,stdout=subprocess.DEVNULL,stderr=err)
            events={}; running={}
            for line in Path(str(stem)+'.perf').read_text().splitlines():
                f=line.split(';')
                if len(f)>4 and f[2] in ('instructions:u','cycles:u'):
                    assert re.fullmatch('[0-9]+',f[0]),line
                    events[f[2]]=int(f[0]); running[f[2]]=float(f[4])
                    assert running[f[2]]>=99,line
            assert len(events)==2
            wall,user,system,rss=Path(str(stem)+'.time').read_text().split()
            raw.append({'workload':name,'variant':variant,'repeat':repeat,**events,
                        'ipc':events['instructions:u']/events['cycles:u'],'running_percent':running,
                        'wall':float(wall),'user':float(user),'sys':float(system),'rss_kib':int(rss)})
    (A/'raw.json').write_text(json.dumps(raw,indent=2))
summary={}
for name in inputs:
    summary[name]={}
    for variant in commands:
        rows=[r for r in raw if r['workload']==name and r['variant']==variant]
        summary[name][variant]={k:{'median':statistics.median(r[k] for r in rows),
                                  'min':min(r[k] for r in rows),'max':max(r[k] for r in rows)}
                                for k in ('instructions:u','cycles:u','ipc','wall','user','sys','rss_kib')}
(A/'summary.json').write_text(json.dumps(summary,indent=2))
(A/'parity.json').write_text(json.dumps(parity,indent=2))
# Every fixed family is sampled, including the slowest suite fixture. Small
# workloads are looped inside one task so the profile exceeds startup noise.
profiles={name:v['path'] for name,v in json.loads((release/'manifest.json').read_text())['inputs'].items()}
profiles['slowest-fixture']=str(fixture)
profiles['headers8000']=inputs['headers8000']['path']
for name,path in profiles.items():
    cmd=[release/'cursor',path]
    cmd=['sh','-c','for i in 1 2 3 4 5 6 7 8 9 10; do "$1" "$2" >/dev/null || exit; done','profile',*cmd]
    with open(A/f'{name}.profile.stderr','w') as err:
        run(['perf','record','-q','-e','cycles:u','-F','99','--call-graph','dwarf,8192','-o',A/f'{name}.perf.data',
             '--','taskset','-c',cpu,*cmd],stdout=subprocess.DEVNULL,stderr=err)
    with open(A/f'{name}.report','w') as out:
        run(['perf','report','--stdio','--percent-limit','0.5','-i',A/f'{name}.perf.data'],stdout=out)
for name in ('conditional','lookup','pastes','probes16000','includes'):
    for variant in ('cursor','before_cursor','gcc'):
        cmd=commands[variant]
        for group,events in [('branch','{branches:u,branch-misses:u}'),('cache','{cache-references:u,cache-misses:u}')]:
            for repeat in range(3):
                counts=A/f'{name}-{variant}-{group}-{repeat}.perf'
                with open(A/f'{name}-{variant}-{group}-{repeat}.stderr','w') as err:
                    run(['perf','stat','-x',';','-o',counts,
                         '-e',events,'--','taskset','-c',cpu,*cmd,profiles[name]],stdout=subprocess.DEVNULL,stderr=err)
                observed=0
                for line in counts.read_text().splitlines():
                    f=line.split(';')
                    if len(f)>4 and f[2].endswith(':u'):
                        assert re.fullmatch('[0-9]+',f[0]) and float(f[4])>=99,line
                        observed+=1
                assert observed==2,counts
for name,v in summary.items():
    print(name,', '.join(f'{key} {r["instructions:u"]["median"]/1e6:.1f}M/{r["wall"]["median"]:.2f}s/{r["rss_kib"]["median"]/1024:.1f}MiB' for key,r in v.items()),flush=True)
print(A,flush=True)
