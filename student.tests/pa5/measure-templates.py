#!/usr/bin/env python3
"""Same-byte portable template/hosted workloads, frozen three-run PMU protocol."""
from pathlib import Path
import hashlib,json,os,platform,statistics,subprocess,sys
R=Path(__file__).resolve().parents[2]
A=Path(sys.argv[1]).resolve(); A.mkdir(parents=True,exist_ok=True)
def run(cmd):
 r=subprocess.run(list(map(str,cmd)),capture_output=True)
 assert r.returncode==0,(cmd,r.stdout[-2000:],r.stderr[-2000:]);return r
cpu=min(os.sched_getaffinity(0));pin=['taskset','-c',str(cpu)]
student=A/'compiler';student.write_bytes((R/'dev/cppgm++').read_bytes());student.chmod(0o755)
ref=A/'reference';ref.write_bytes((R/'reference-binaries/cppgm++').read_bytes());ref.chmod(0o755)
header=A/'include.cpp';header.write_text('#include <initializer_list>\n')
# Freeze the host header's expanded C++11 bytes, not a compiler implementation
# dependency. Disable unrelated host-only bf16/attribute extensions explicitly.
flags=['-std=c++11','-U__BFLT16_DIG__','-D__attribute__(x)=']
hosted=run(['g++',*flags,'-E','-P',header]).stdout.decode()
inputs={}
for n in [2000,20000]:
 lines=[hosted]
 for i in range(n):
  lines += [f'namespace n{i} {{',
   'template<int n> struct tag {}; template<class value> struct box { typedef value type; };',
   'template<template<class> class item, class value, int n=2> struct holder: item<value> {',
   ' holder():data{} {} template<int count> value get(value x=sizeof(late)) const noexcept(sizeof(late)>0) {return data+x+count;}',
   ' struct late { int x; }; value data=sizeof(box<tag<(8 >> 1)>>); };',
   'template<class value> using alias=typename box<value>::type;',
   'template<class value> auto call(value x)->decltype(x.template get<2>()) {return x.template get<2>();}',
   'template<class... values> void consume(values...); template<class... values> void pack(values... x) { consume(x...); }',
   'box<box<tag<(4 >> 1)>>> nested; holder<box,int> object;',
   'int f(std::initializer_list<int> x) { int result=0; for(auto value:x) result+=value; return result; }',
   '}']
 p=A/f'templates{n}.cpp';p.write_text('\n'.join(lines)+'\n');inputs[p.stem]=p
variants={'student':[student,'--emit-ast','--telemetry','-o','/dev/null'],
 'gcc':['g++','-std=c++11','-fsyntax-only'],'clang':['clang++','-std=c++11','-fsyntax-only'],
 'reference':[ref,'--emit-ast','-o','/dev/null']}
manifest={'cpu':cpu,'platform':platform.uname()._asdict(),'lscpu':run(['lscpu']).stdout.decode(),
 'versions':{v:run([v,'--version']).stdout.decode() for v in ['g++','clang++','perf']},
 'commands':{v:list(map(str,c)) for v,c in variants.items()},'header_preprocess_flags':flags,
 'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [student,ref,*inputs.values()]},
 'compiler_build_flags':'g++ -std=gnu++11 -Wall -O3; dev source sets',
 'compiler_size':run(['size',student]).stdout.decode(),
 'generated_runtime_text_object_size':'N/A: PA5 only retains syntax; no native emission'}
(A/'manifest.json').write_text(json.dumps(manifest,indent=2));summary={};rows=[]
for name,p in inputs.items():
 for v,c in variants.items():run(pin+c+[p]) # qualify and warm outside counts
 s=A/(name+'.ast');o=A/(name+'.ref')
 run([student,'--emit-ast','-o',s,p]);run([ref,'--emit-ast','-o',o,p]);assert s.read_bytes()==o.read_bytes(),name
 summary[name]={'bytes':p.stat().st_size,'ast_bytes':s.stat().st_size,'variants':{}}
 s.unlink();o.unlink()
 for repeat in range(3):
  for v,c in variants.items():
   stem=A/f'{name}-{v}-{repeat}';counts=Path(str(stem)+'.perf');timing=Path(str(stem)+'.time')
   r=run(['perf','stat','-x',';','-o',counts,'-e','{instructions:u,cycles:u}','--',*pin,
    '/usr/bin/time','-f','%e %U %S %M','-o',timing,*c,p]);Path(str(stem)+'.stderr').write_bytes(r.stderr)
   data={}
   for line in counts.read_text().splitlines():
    c=line.split(';')
    if len(c)>4 and c[2] in ['instructions:u','cycles:u']:
     assert c[0].isdigit() and float(c[4])>=99.9,line
     data[c[2]]=int(c[0]);data[c[2]+'_running_percent']=float(c[4])
   assert len(data)==4,counts.read_text()
   wall,user,system,rss=map(float,timing.read_text().split())
   data.update(wall_s=wall,user_s=user,system_s=system,rss_kib=rss,ipc=data['instructions:u']/data['cycles:u'])
   rows.append(dict(workload=name,variant=v,repeat=repeat,**data))
 for v in variants:
  selected=[r for r in rows if r['workload']==name and r['variant']==v]
  summary[name]['variants'][v]={k:{'median':statistics.median(r[k] for r in selected),
   'range':[min(r[k] for r in selected),max(r[k] for r in selected)]} for k in data}
 (A/'runs.json').write_text(json.dumps(rows,indent=2));(A/'summary.json').write_text(json.dumps(summary,indent=2))
 print(name,json.dumps(summary[name]),flush=True)
large=inputs['templates20000']
for v,c in variants.items():
 for label,event in [('branch','{branches:u,branch-misses:u}'),('cache','{cache-references:u,cache-misses:u}')]:
  for repeat in range(3):
   run(['perf','stat','-x',';','-o',A/f'profile-{v}-{label}-{repeat}.perf','-e',event,'--',*pin,*c,large])
# Attribution, separate from acceptance measurements.
run(['perf','record','-q','-e','cycles:u','-F','99','--call-graph','dwarf,8192','-o',A/'profile.data','--',*pin,*variants['student'],large])
(A/'profile.txt').write_bytes(run(['perf','report','--stdio','--no-children','-i',A/'profile.data']).stdout)
