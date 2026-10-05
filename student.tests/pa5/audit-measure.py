#!/usr/bin/env python3
"""Final audit: freeze entry/final/host controls and retain paired PMU observations."""
from pathlib import Path
import hashlib,json,os,platform,statistics,subprocess,sys
R=Path(__file__).resolve().parents[2]
A=Path(sys.argv[1]).resolve();A.mkdir(parents=True,exist_ok=True)
prior=Path(os.environ['RALPH_ARTIFACT_DIR'])/'pa5/loop13'
cpu=int(os.environ.get('PA5_CPU',min(os.sched_getaffinity(0))))
assert cpu in os.sched_getaffinity(0)
pin=['taskset','-c',str(cpu)]
def run(c):
 r=subprocess.run(list(map(str,c)),capture_output=True)
 assert r.returncode==0,(c,r.stdout[-2000:],r.stderr[-2000:]);return r
bins={}
for n,p in [('student',R/'dev/cppgm++'),('entry',prior/'final-perf-ordinary/cppgm-final'),('reference',R/'reference-binaries/cppgm++')]:
 q=A/n;q.write_bytes(p.read_bytes());q.chmod(0o755);bins[n]=q
inputs={}
for group,n in [('ordinary','ordinary20000'),('ordinary','ordinary2000'),('scopes','scopes20000'),('classes','classes20000'),('parameters','parameters60000'),('parameters','parameters2000'),('templates','templates20000'),('templates','templates2000')]:
 p=A/(n+'.cpp');p.write_bytes((prior/f'final-perf-{group}'/(n+'.cpp')).read_bytes());inputs[n]=p
commands={n:[b,'--emit-ast','-o','/dev/null'] for n,b in bins.items()}
commands.update(gcc=['g++','-std=c++11','-fsyntax-only'],clang=['clang++','-std=c++11','-fsyntax-only'])
manifest={'cpu':cpu,'platform':platform.uname()._asdict(),'lscpu':run(['lscpu']).stdout.decode(),
 'versions':{v:run([v,'--version']).stdout.decode() for v in ['g++','clang++','perf']},
 'commands':{n:list(map(str,c)) for n,c in commands.items()},
 'hashes':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [*bins.values(),*inputs.values()]},
 'build_flags':'g++ -std=gnu++11 -Wall -O3 (registered frontend sources)',
 'size':{n:run(['size',p]).stdout.decode() for n,p in bins.items()},
 'native_program_metrics':'not available at PA5; no student native emission; PA33/34 instruction gate retained',
 'warmup':'one per command/input outside three interleaved measurements',
 'counters':'{instructions:u,cycles:u}; raw running/time enabled retained',
 'comparison_scope':'PA5 syntax plus AST view versus C++11 host syntax+semantics; same bytes, no native-code parity claim'}
(A/'manifest.json').write_text(json.dumps(manifest,indent=2));rows=[];summary={}
for n,p in inputs.items():
 for tool,c in commands.items():run(pin+c+[p])
 for tool in bins:
  out=A/(tool+'.ast');run([bins[tool],'--emit-ast','-o',out,p])
 assert (A/'student.ast').read_bytes()==(A/'entry.ast').read_bytes()==(A/'reference.ast').read_bytes(),n
 summary[n]={'input_bytes':p.stat().st_size,'ast_bytes':(A/'student.ast').stat().st_size,'variants':{}}
 for tool in bins:(A/(tool+'.ast')).unlink()
 for repeat in range(3):
  for tool,c in commands.items():
   stem=A/f'{n}-{tool}-{repeat}'
   r=run(['perf','stat','-x',';','-o',str(stem)+'.perf','-e','{instructions:u,cycles:u}','--',*pin,'/usr/bin/time','-f','%e %U %S %M','-o',str(stem)+'.time',*c,p])
   Path(str(stem)+'.stderr').write_bytes(r.stderr);counts={}
   for line in Path(str(stem)+'.perf').read_text().splitlines():
    fields=line.split(';')
    if len(fields)>4 and fields[2] in ['instructions:u','cycles:u']:
     assert fields[0].isdigit() and float(fields[4])>=99.9,(stem,line)
     counts[fields[2]]=int(fields[0]);counts[fields[2]+'_running_percent']=float(fields[4])
   assert len(counts)==4,stem
   wall,user,system,rss=map(float,Path(str(stem)+'.time').read_text().split())
   rows.append(dict(workload=n,variant=tool,repeat=repeat,**counts,wall_s=wall,user_s=user,system_s=system,rss_kib=rss,ipc=counts['instructions:u']/counts['cycles:u']))
 for tool in commands:
  selected=[r for r in rows if r['workload']==n and r['variant']==tool]
  summary[n]['variants'][tool]={k:{'median':statistics.median(r[k] for r in selected),'range':[min(r[k] for r in selected),max(r[k] for r in selected)]} for k in ['instructions:u','cycles:u','ipc','wall_s','user_s','system_s','rss_kib']}
 (A/'runs.json').write_text(json.dumps(rows,indent=2));(A/'summary.json').write_text(json.dumps(summary,indent=2))
 print(n,json.dumps(summary[n]),flush=True)
# The largest syntax workload and the host-relative worst one: primary pair,
# independent branches/cache groups; sampling follows spec exactly.
for n in ['templates20000','parameters60000']:
 p=inputs[n]
 for tool in ['student','gcc']:
  for repeat in range(3):
   for label,event in [('branch','{branches:u,branch-misses:u}'),('cache','{cache-references:u,cache-misses:u}')]:
    stem=A/f'{n}-{tool}-{label}-{repeat}'
    run(['perf','stat','-x',';','-o',str(stem)+'.perf','-e',event,'--',*pin,*commands[tool],p])
 for tool in ['student','gcc']:
  data=A/(n+'-'+tool+'.data')
  result=run(['perf','record','-e','cycles:u','-F','99','--call-graph','dwarf,8192','-o',data,'--',*pin,*commands[tool],p])
  (A/(n+'-'+tool+'-record.stderr')).write_bytes(result.stderr)
  (A/(n+'-'+tool+'-report.txt')).write_bytes(run(['perf','report','--stdio','--no-children','-i',data]).stdout)
