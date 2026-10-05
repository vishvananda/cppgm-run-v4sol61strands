#!/usr/bin/env python3
"""Expected grammar rejection must fail cleanly, not crash, hang or throw."""
from pathlib import Path
import os, subprocess, tempfile
R=Path(__file__).resolve().parents[2]
T=Path(os.environ.get('PA5_TOOL',R/'dev/cppgm++')).resolve()
sources=[R/'student.tests/pa5-portable.cpp',R/'student.tests/pa5/scopes-portable.cpp',R/'student.tests/pa5/classes-portable.cpp',R/'student.tests/pa5/templates-portable.cpp']
with tempfile.TemporaryDirectory() as directory:
 d=Path(directory); count=0
 for source in sources:
  text=source.read_text()
  # Fixed deterministic prefixes exercise partially constructed declarations,
  # scopes, operator names and detached complete-class queues.
  for end in sorted(set(range(0,len(text),max(1,len(text)//50)))|{len(text)}):
   p=d/'prefix.cpp';p.write_text(text[:end])
   r=subprocess.run([T,'--emit-ast','-o',d/'ast',p],capture_output=True,timeout=5)
   assert r.returncode in (0,1),(source,end,r.returncode,r.stderr.decode())
   assert b'AddressSanitizer' not in r.stderr and b'runtime error:' not in r.stderr,(source,end,r.stderr.decode())
   count+=1
 # Explicit reduced cases keep deferred-context rejection covered even if a
 # portable integration changes length and its sampled prefixes move.
 reducers=[
  'namespace patterns { template<class value> struct box { box():data{} {} box(value',
  'namespace patterns { struct box { int f() { return (; } }; }',
  'namespace patterns { template<class value> struct box { int f(int x=sizeof()); }; }',
  'template<class v> void f(v x) {typename decltype(x)::template item<int y;}',
  'template<class v> void f(v x) {typename decltype(x) y;}',
  'template<class v> struct box {template<class u> u f(u);}; template<class v> template<class u> u box<v>::f(u x) {return (;}',
  'struct x {enum class type:int {a=};};',
 ]
 for text in reducers:
  p=d/'reduced.cpp';p.write_text(text)
  r=subprocess.run([T,'--emit-ast','-o',d/'ast',p],capture_output=True,timeout=5)
  assert r.returncode==1,(text,r.returncode,r.stderr.decode())
  assert b'AddressSanitizer' not in r.stderr and b'runtime error:' not in r.stderr,(text,r.stderr.decode())
print(f'PA5 clean prefix rejection passed: {count} prefixes + {len(reducers)} deferred-context reducers')
