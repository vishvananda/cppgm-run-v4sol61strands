#!/usr/bin/env python3
"""Freeze/qualify stage-supported sources; interleaved pinned PMU measurements."""
import argparse, hashlib, json, os, platform, statistics, subprocess, sys, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(); p.add_argument('artifacts'); args=p.parse_args()
A=Path(args.artifacts).resolve(); A.mkdir(parents=True,exist_ok=True)
def run(cmd,**kwargs):
    r=subprocess.run([str(x) for x in cmd],capture_output=True,**kwargs)
    assert r.returncode==0,(cmd,r.stdout[-1000:],r.stderr[-1000:])
    return r
cpu=min(os.sched_getaffinity(0)); affinity=['taskset','-c',str(cpu)]
exe=A/'cppgm-final'; exe.write_bytes((ROOT/'dev/cppgm++').read_bytes()); exe.chmod(0o755)
base=A/'cppgm-group-baseline'; base.write_bytes((A.parent/'base-parser').read_bytes()); base.chmod(0o755)
reference=A/'cppgm-reference'; reference.write_bytes((ROOT/'reference-binaries/cppgm++').read_bytes()); reference.chmod(0o755)
inputs={}
for n in [2000,20000]:
    path=A/f'ordinary{n}.cpp'
    path.write_bytes(run([sys.executable,ROOT/'student.tests/pa5-workload.py',n]).stdout)
    inputs[f'ordinary{n}']=path
mix=A/'extended2000.cpp'; lines=['typedef unsigned long word;','void* operator new(decltype(sizeof(0)), void*);']
for i in range(2000):
    lines += [f'word f{i}(word x, word y, void* storage) {{',
              '  word z=x*3+y;',
              '  for(word k=0;k<8;++k) { z^=(x<<k)+y; }',
              '  auto f=[z,&y](word a) mutable noexcept(true)->word { return a+z+y; };',
              '  word* p=new word(z); word* q=new word[3]{x,y,z};',
              '  word* s=new (storage) word;',
              '  delete p; delete[] q;',
              '  try { if(z>x) throw z; } catch(word a) { z=a; }',
              '  return f(z)+sizeof(const word*)+static_cast<word>(x+y);','}']
mix.write_text('\n'.join(lines)+'\n'); inputs['extended2000']=mix
# Largest currently accepted fixture; course syntax-only, so not host-qualified.
accepted=[]
for path in (ROOT/'pa5/tests').rglob('*.t'):
    status=Path(str(path)[:-2]+'.my.exit_status')
    if status.exists() and status.read_text().strip()=='EXIT_SUCCESS': accepted.append(path)
fixture=max(accepted,key=lambda p:p.stat().st_size)
inputs['fixture']=A/'fixture.cpp'; inputs['fixture'].write_bytes(fixture.read_bytes())
variants={
 'student':[exe,'--emit-ast','--telemetry','-o','/dev/null'],
 'baseline':[base,'--emit-ast','--telemetry','-o','/dev/null'],
 'gcc':['g++','-std=c++11','-fsyntax-only'],
 'clang':['clang++','-std=c++11','-fsyntax-only'],
 'reference':[reference,'--emit-ast','-o','/dev/null'],
}
manifest={'cpu':cpu,'uname':platform.uname()._asdict(),'lscpu':run(['lscpu']).stdout.decode(),
          'versions':{h:run([h,'--version']).stdout.decode() for h in ['g++','clang++','perf']},
          'flags':variants,'fixture_original':str(fixture.relative_to(ROOT)),
          'hashes':{str(path.name):hashlib.sha256(path.read_bytes()).hexdigest() for path in [exe,base,reference,*inputs.values()]}}
# Freeze local compiler output size separately from generated-program code size.
manifest['compiler_size']=run(['size',exe]).stdout.decode()
manifest['compiler_flags']='g++ -std=gnu++11 -Wall -O3; repository dev source sets'
manifest['generated_program_runtime_text_object_size']='N/A: PA5 syntax-only, no native emission'
(A/'manifest.json').write_text(json.dumps(manifest,indent=2,default=str))
summary={}; rows=[]
for name,source in inputs.items():
    supported=['student','gcc','clang','reference'] if name!='fixture' else ['student','reference']
    if name.startswith('ordinary'): supported.insert(1,'baseline')
    # Hosts accept identical bytes before timings; dump observation compared once.
    for v in supported: run([*affinity,*variants[v],source])
    for v in ['student','reference']:
        run([exe if v=='student' else reference,'--emit-ast','-o',A/(name+'-'+v+'.ast'),source])
    student_dump=A/(name+'-student.ast'); ref_dump=A/(name+'-reference.ast')
    assert student_dump.read_bytes()==ref_dump.read_bytes(),name
    summary[name]={'source_bytes':source.stat().st_size,'ast_bytes':student_dump.stat().st_size,'variants':{}}
    for repeat in range(3):
        # Warmups above; all variants have identical CPU and environment.
        for v in supported:
            stem=A/f'{name}-{v}-{repeat}'
            counts=Path(str(stem)+'.perf'); timing=Path(str(stem)+'.time')
            cmd=['perf','stat','-x',';','-o',counts,'-e','{instructions:u,cycles:u}', '--',*affinity,
                 '/usr/bin/time','-f','%e %U %S %M','-o',timing,*variants[v],source]
            r=run(cmd); Path(str(stem)+'.stderr').write_bytes(r.stderr)
            data={}
            for line in counts.read_text().splitlines():
                cols=line.split(';')
                if len(cols)>4 and cols[2] in ['instructions:u','cycles:u']:
                    assert cols[0].isdigit(),line
                    assert float(cols[4])>=95,line
                    data[cols[2]]=int(cols[0]); data[cols[2]+'_running_percent']=float(cols[4])
            assert len(data)==4,counts.read_text()
            wall,user,system,rss=map(float,timing.read_text().split())
            data.update(wall_s=wall,user_s=user,system_s=system,rss_kib=rss,ipc=data['instructions:u']/data['cycles:u'])
            rows.append(dict(workload=name,variant=v,repeat=repeat,**data))
    for v in supported:
        selected=[r for r in rows if r['workload']==name and r['variant']==v]
        summary[name]['variants'][v]={k:{'median':statistics.median(r[k] for r in selected),
                                      'range':[min(r[k] for r in selected),max(r[k] for r in selected)]}
                                   for k in ['instructions:u','cycles:u','ipc','wall_s','user_s','system_s','rss_kib']}
    (A/'runs.json').write_text(json.dumps(rows,indent=2)); (A/'summary.json').write_text(json.dumps(summary,indent=2))
    print(name,json.dumps(summary[name]),flush=True)
# Slowest supported scale input, extra event groups are not multiplexed with primary PMU pair.
source=inputs['ordinary20000']
for v in ['student','gcc','clang','reference']:
    for label,event in [('branch','{branches:u,branch-misses:u}'),('cache','{cache-references:u,cache-misses:u}')]:
        for repeat in range(3):
            stem=A/f'profile-{v}-{label}-{repeat}'
            r=run(['perf','stat','-x',';','-o',str(stem)+'.perf','-e',event,'--',*affinity,*variants[v],source])
            Path(str(stem)+'.stderr').write_bytes(r.stderr)
run(['perf','record','-e','cycles:u','-F','199','--call-graph','dwarf,8192','-o',A/'cycles.data','--',*affinity,*variants['student'],source])
r=run(['perf','report','--stdio','-i',A/'cycles.data','--no-children','--percent-limit','1'])
(A/'cycles.report').write_bytes(r.stdout)
print('Frozen/qualified PMU controls complete:',A,flush=True)
