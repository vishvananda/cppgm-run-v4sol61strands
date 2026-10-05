#!/usr/bin/env python3
"""Observe independent remaining capability families, without treating gaps as passing."""
from pathlib import Path
import subprocess, tempfile, os, json
R=Path(__file__).resolve().parents[2]
A=Path(os.environ.get('RALPH_ARTIFACT_DIR','/tmp'))/'pa5'; A.mkdir(parents=True,exist_ok=True)
cases={
 'namespace':'namespace n { typedef int word; int f(word x) { return x; } } int g() { return n::f(1); }',
 'class_members':'struct lower { int x; lower():x(1) {} int f() const { return x; } };',
 'enum':'enum class lower : unsigned { zero, one=1 };',
 'template':'template<class value> value identity(value x) { return x; } int f() { return identity<int>(1); }',
 'dependent':'template<class value> typename value::type f(value x) { return x.template get<0>(); }',
 'non_type_template':'template<int count> int f() { return count; }',
 'qualified_special':'struct lower { lower(); }; lower::lower() {}',
}
observations={}
with tempfile.TemporaryDirectory() as d:
 d=Path(d)
 for n,s in cases.items():
  p=d/(n+'.cpp'); p.write_text(s); observations[n]={}
  for h in ['g++','clang++']:
   r=subprocess.run([h,'-std=c++11','-fsyntax-only',p],capture_output=True)
   assert r.returncode==0,(n,h,r.stderr); observations[n][h]=r.returncode
  for v,t in [('student',R/'dev/cppgm++'),('reference',R/'reference-binaries/cppgm++')]:
   r=subprocess.run([t,'--emit-ast','-o',d/(n+'-'+v+'.ast'),p],capture_output=True)
   observations[n][v]={'exit':r.returncode,'diagnostic':(r.stdout+r.stderr).decode()}
   if v=='reference': assert r.returncode==0,(n,r.stdout,r.stderr)
(A/'remaining-capabilities.json').write_text(json.dumps(observations,indent=2))
print('PA5 remaining capability observations:', {n:o['student']['exit'] for n,o in observations.items()})
