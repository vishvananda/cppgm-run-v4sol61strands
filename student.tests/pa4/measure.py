#!/usr/bin/env python3
"""Identical PA4 source, warm pinned interleaved three-run PMU/time protocol."""
import argparse
import hashlib
import json
import os
import pathlib
import re
import shutil
import statistics
import subprocess
ROOT=pathlib.Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(); p.add_argument('artifact',type=pathlib.Path);p.add_argument('--label',default='initial');p.add_argument('--against');p.add_argument('--only', nargs='+', help='measure named workload subset, retaining all earlier artifacts');a=p.parse_args()
A=a.artifact.resolve()/a.label; A.mkdir(parents=True,exist_ok=False)
cpu=min(os.sched_getaffinity(0))
def run(cmd,**kw): return subprocess.run(list(map(str,cmd)),check=True,**kw)
def sha(path): return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()
sources=list((ROOT/'dev/src/preprocess/lex').glob('*.cpp'))+list((ROOT/'dev/src/preprocess/post').glob('*.cpp'))+list((ROOT/'dev/src/preprocess/expr').glob('*.cpp'))+list((ROOT/'dev/src/preprocess/engine').glob('*.cpp'))
flags=['-std=c++11','-O3','-g','-I'+str(ROOT/'dev/src')]
for variant,entry in [('cursor',ROOT/'student.tests/pa4/bench.cpp'),('preproc',ROOT/'dev/preproc.cpp')]:
    run(['g++',*flags,entry,*sources,'-o',A/variant])
variants={
    'cursor':[A/'cursor'], 'preproc':[A/'preproc','-o',A/'dump'],
    'gcc':['g++','-std=c++11','-E','-P','-x','c++'],
    'clang':['clang++','-std=c++11','-E','-P','-x','c++'],
    'reference':[ROOT/'dev/preproc-ref','-o',A/'refdump'],
}
if a.against:
    before=a.artifact.resolve()/a.against
    variants['before_cursor']=[before/'cursor']
    variants['before_preproc']=[before/'preproc','-o',A/'before_dump']
inputs={}
workloads=['text','lookup','arguments','pastes','deep','conditional','includes','aliases','nested','probes1000','probes4000','probes16000']
if a.only:
    assert set(a.only)<=set(workloads), 'unknown workload'
    workloads=[name for name in workloads if name in a.only]
for name in workloads:
    path=A/(name+'.cc')
    if name=='text': text='done\n'*300000; expected=300000
    elif name=='lookup':
        text=''.join(f'#define D{i} {i}\n' for i in range(120000))+'done\n'*100000;expected=100000
    elif name=='arguments':
        text='#define ID(x) x\n#define REP(x) x x x x x x x x\n'+'REP(ID(done))\n'*70000;expected=560000
    elif name=='pastes':
        text='#define CAT(a,b) a##b\n'+'CAT(do,ne)\n'*200000; expected=200000
    elif name=='deep':
        text='#define F0() done\n'+''.join(f'#define F{i}() F{i-1}()\n' for i in range(1,30001))+'#define REP(x) x x x x\nREP(F30000())\n';expected=4
    elif name=='conditional':
        text='#define FLAG 7\n'+ '#if defined(FLAG) && FLAG==7 && (1 || 1/0)\ndone\n#else\n#error fail\n#endif\n'*85000;expected=85000
    elif name.startswith('probes'):
        count=int(name[6:])
        expression=' && '.join(['(1 + __has_cpp_attribute(pa4_unknown_attribute) == 1)']*count)
        text='#if '+expression+'\ndone\n#else\n#error probe compaction\n#endif\n';expected=1
    elif name=='nested':
        text='#define F(x) x\n'+('F('*1000+'done'+')'*1000+'\n')*60;expected=60
    elif name=='aliases':
        text='#define A B\n#define B C\n#define C done\n'+'A\n'*300000;expected=300000
    else:
        header=A/'header.hh';header.write_text('#pragma once\n#define VAL done\n')
        text='#include "header.hh"\n'*60000+'VAL\n'*60000;expected=60000
    path.write_text(text);inputs[name]={'path':str(path),'bytes':path.stat().st_size,'sha256':sha(path),'expected':expected}
manifest={'cpu':cpu,'cpu_model':next(s for s in pathlib.Path('/proc/cpuinfo').read_text().splitlines() if s.startswith('model name')),
          'flags':flags,'commands':{k:list(map(str,v)) for k,v in variants.items()},'inputs':inputs,
          'hashes':{k:sha(shutil.which(str(v[0]))) for k,v in variants.items()},
          'source_hashes':{str(x.relative_to(ROOT)):sha(x) for x in [ROOT/'dev/preproc.cpp',ROOT/'student.tests/pa4/bench.cpp',*sources,*sorted((ROOT/'dev/src/preprocess').rglob('*.h'))]},
          'gcc_cc1plus_hash':sha(subprocess.check_output(['g++','-print-prog-name=cc1plus'],text=True).strip()),
          'reference_binary_hash':sha(ROOT/'reference-binaries/preproc'),
          'hosts':{c:subprocess.check_output([c,'--version'],text=True).splitlines()[0] for c in ['g++','clang++']},
          'cache':'one warmup per variant; warm-cache interleaved repeats; taskset fixed CPU',
          'executable_runtime_text_size':'N/A: PA4 emits tokens, not native programs'}
(A/'manifest.json').write_text(json.dumps(manifest,indent=2))
observations=[]; parity={}
for name,v in inputs.items():
    parity[name]={}
    for variant,cmd in variants.items():
        output=run([*cmd,v['path']],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        if variant in ['cursor','before_cursor']: ok=int(output.stdout)==v['expected']
        elif variant in ['gcc','clang']: ok=output.stdout.split()==['done']*v['expected']
        else:
            dump=(A/('dump' if variant=='preproc' else 'before_dump' if variant=='before_preproc' else 'refdump')).read_text().splitlines()
            ok=dump[2:-1]==['identifier done']*v['expected']
        parity[name][variant]=ok;assert ok,(name,variant)
        (A/(name+'-'+variant+'-warmup.stderr')).write_text(output.stderr)
    for repeat in range(3):
        for variant,cmd in variants.items():
            stem=A/f'{name}-{variant}-{repeat}'
            measured=['perf','stat','-x',';','-o',str(stem)+'.perf','-e','{instructions:u,cycles:u}','--',
                      '/usr/bin/time','-f','%e %U %S %M','-o',str(stem)+'.time','taskset','-c',str(cpu),*cmd,v['path']]
            with open(str(stem)+'.stderr','w') as err:run(measured,stdout=subprocess.DEVNULL,stderr=err)
            events={};running={}
            for line in pathlib.Path(str(stem)+'.perf').read_text().splitlines():
                f=line.split(';')
                if len(f)>4 and f[2] in ['instructions:u','cycles:u']:
                    assert re.fullmatch(r'[0-9]+',f[0]),line
                    events[f[2]]=int(f[0]);running[f[2]]=float(f[4]);assert float(f[4])>=99
            assert len(events)==2
            wall,user,system,rss=pathlib.Path(str(stem)+'.time').read_text().split()
            observations.append({'workload':name,'variant':variant,'repeat':repeat,**events,'ipc':events['instructions:u']/events['cycles:u'],
                                 'running_percent':running,'wall':float(wall),'user':float(user),'sys':float(system),'rss_kib':int(rss)})
    (A/'raw.json').write_text(json.dumps(observations,indent=2))
summary={}
for name in inputs:
    summary[name]={}
    for variant in variants:
        runs=[r for r in observations if r['workload']==name and r['variant']==variant]
        summary[name][variant]={k:{'median':statistics.median(r[k] for r in runs),'min':min(r[k] for r in runs),'max':max(r[k] for r in runs)}
                                for k in ['instructions:u','cycles:u','ipc','wall','user','sys','rss_kib']}
    for variant in variants:
        summary[name][variant]['vs_gcc']={k:summary[name][variant][k]['median']/summary[name]['gcc'][k]['median']
                                         for k in ['instructions:u','cycles:u','wall','rss_kib'] if summary[name]['gcc'][k]['median']}
(A/'summary.json').write_text(json.dumps(summary,indent=2));(A/'parity.json').write_text(json.dumps(parity,indent=2))
for name in summary:
    print(name,':',', '.join(f'{v} {summary[name][v]["instructions:u"]["median"]/1e6:.1f}M instructions, {summary[name][v]["wall"]["median"]:.3f}s, {summary[name][v]["rss_kib"]["median"]/1024:.1f}MiB' for v in variants))
print(A)
