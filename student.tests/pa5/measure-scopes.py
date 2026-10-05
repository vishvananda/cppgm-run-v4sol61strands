#!/usr/bin/env python3
"""Freeze and measure indexed-scope scaling on identical portable PA5 bytes."""
import hashlib,json,os,platform,statistics,subprocess,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2]
A=Path(sys.argv[1]).resolve(); A.mkdir(parents=True,exist_ok=True)
def run(cmd):
 r=subprocess.run([str(x) for x in cmd],capture_output=True)
 assert r.returncode==0,(cmd,r.stdout[-1500:],r.stderr[-1500:]); return r
cpu=min(os.sched_getaffinity(0)); pinned=['taskset','-c',str(cpu)]
exe=A/'compiler'; exe.write_bytes((R/'dev/cppgm++').read_bytes());exe.chmod(0o755)
ref=A/'reference';ref.write_bytes((R/'reference-binaries/cppgm++').read_bytes());ref.chmod(0o755)
inputs={}
for n in [2000,20000]:
 p=A/f'scopes{n}.cpp'
 lines=[]
 for i in range(n):
  lines += [f'namespace n{i} {{ using word=unsigned long; enum class mode:word {{ zero, one=1 }}; enum status {{ ready, done=ready+1 }}; word f(word x){{return x+done;}} }}',
            f'namespace n{i} {{ word g(word x){{return f(x);}} }} namespace alias{i}=n{i};',
            f'namespace m{i} {{ using alias{i}::word; using namespace alias{i}; word h(word x){{return g(x)+word(mode::one);}} }}',
            f'unsigned long call{i}(unsigned long x) {{ alias{i}::word y=alias{i}::f(x); return m{i}::h(y); }}']
 p.write_text('\n'.join(lines)+'\n'); inputs[f'scopes{n}']=p
# Same identifier per independent namespace: adding unrelated scopes cannot
# increase a category query's candidate/visited set.
variants={'student':[exe,'--emit-ast','--telemetry','-o','/dev/null'],
          'gcc':['g++','-std=c++11','-fsyntax-only'],
          'clang':['clang++','-std=c++11','-fsyntax-only'],
          'reference':[ref,'--emit-ast','-o','/dev/null']}
manifest={'cpu':cpu,'platform':platform.uname()._asdict(),'lscpu':run(['lscpu']).stdout.decode(),
 'versions':{v:run([v,'--version']).stdout.decode() for v in ['g++','clang++','perf']},
 'commands':{v:list(map(str,c)) for v,c in variants.items()},
 'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [exe,ref,*inputs.values()]}}
(A/'manifest.json').write_text(json.dumps(manifest,indent=2))
summary={}
for name,p in inputs.items():
 s=A/(name+'.ast');o=A/(name+'.ref')
 run([exe,'--emit-ast','-o',s,p]);run([ref,'--emit-ast','-o',o,p]);assert s.read_bytes()==o.read_bytes(),name
 for v in ['gcc','clang']:run(variants[v]+[p])
 for cmd in variants.values():run(pinned+cmd+[p])
 rows={v:[] for v in variants}
 for iteration in range(3):
  for v,cmd in variants.items():
   stem=A/f'{name}-{v}-{iteration}'; counts=Path(str(stem)+'.counts'); timing=Path(str(stem)+'.time')
   r=run(['perf','stat','-x',';','-o',counts,'-e','{instructions:u,cycles:u}', '--',*pinned,
          '/usr/bin/time','-f','wall=%e user=%U sys=%S rss=%M','-o',timing,*cmd,p])
   Path(str(stem)+'.telemetry').write_bytes(r.stderr)
   row={}
   for line in counts.read_text().splitlines():
    parts=line.split(';')
    if len(parts)>4 and parts[2] in ['instructions:u','cycles:u']:
     assert parts[0].isdigit() and float(parts[4])>=99.9,(stem,line)
     row[parts[2]]=int(parts[0])
   assert len(row)==2,(stem,counts.read_text())
   row['ipc']=row['instructions:u']/row['cycles:u']
   for item in timing.read_text().split():
    k,value=item.split('='); row[k]=float(value)
   rows[v].append(row)
 summary[name]={'bytes':p.stat().st_size,'ast_bytes':s.stat().st_size,'variants':{}}
 for v,data in rows.items():
  summary[name]['variants'][v]={k:{'median':statistics.median([x[k] for x in data]),'range':[min(x[k] for x in data),max(x[k] for x in data)]} for k in data[0]}
 s.unlink();o.unlink()
(A/'summary.json').write_text(json.dumps(summary,indent=2)); print(json.dumps(summary,indent=2))
# Supplemental events and sampling are separate, never part of PMU A/B claims.
large=inputs['scopes20000']
for iteration in range(3):
 run(['perf','stat','-x',';','-o',A/f'branch-cache-{iteration}.txt','-e',
      'branches:u,branch-misses:u,cache-references:u,cache-misses:u','--',*pinned,*variants['student'],large])
r=subprocess.run(['perf','record','-q','-e','cycles:u','-o',str(A/'profile.data'),'--',*pinned,*variants['student'],str(large)],capture_output=True)
(A/'profile-status.txt').write_bytes(r.stdout+r.stderr)
if r.returncode==0: (A/'profile.txt').write_bytes(run(['perf','report','--stdio','--no-children','-i',A/'profile.data']).stdout)
(A/'compiler-size.txt').write_bytes(run(['size',exe]).stdout)
