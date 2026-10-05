#!/usr/bin/env python3
"""Stage-supported behavior, portable qualification and independent graph checks."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOL=Path(os.environ.get('PA5_TOOL',ROOT/'dev/cppgm++')).resolve()
REF=ROOT/'reference-binaries/cppgm++'
A=Path(os.environ.get('RALPH_ARTIFACT_DIR','/tmp'))/'pa5'
A.mkdir(parents=True,exist_ok=True)
def run(tool, source, output):
    return subprocess.run([str(tool),'--emit-ast','-o',str(output),str(source)],capture_output=True)
portable=ROOT/'student.tests/pa5-portable.cpp'
for host in ['g++','clang++']:
    r=subprocess.run([host,'-std=c++11','-fsyntax-only',str(portable)],capture_output=True)
    assert r.returncode==0,(host,r.stderr.decode())
r=run(TOOL,portable,A/'portable.ast'); assert r.returncode==0,r.stderr+r.stdout
r=run(REF,portable,A/'portable.ref'); assert r.returncode==0,r.stderr+r.stdout
assert (A/'portable.ast').read_bytes()==(A/'portable.ref').read_bytes()
# All portable families are valid C++11; reference is additional dump parity,
# not a substitute for graph invariants or host qualification.
cases={
 'scope':'typedef int name; int f(int C) { { int name=1; name+=C; } name x=2; if (int T=C) return T<2; return x; }',
 'associativity':'int f(int a,int b,int c) { a=b=c; return a-b-c + (a?b:c?b:a); }',
 'decl_preference':'typedef int kind; int f() { kind(a); a=1; kind (*ptr)(int); kind(e)[5]; return a; }',
 'conversion':'typedef int kind; int f(int x) { int(x+1); return ((kind())) + (kind)(x) + static_cast<int>(x); }',
 'function_types':'typedef int value_type; int named(int(value)); int callback(int(value_type)); int variadic(int(...)); using Fn=int(int);',
 'allocation':'void* operator new(decltype(sizeof(0)), void*); void f(void* p) { new (int); new (char*)(); new (p) int; }',
 'lambda':'int f(int x,int y) { auto a=[x,&y](int n) mutable noexcept(true)->int { return x+y+n; }; auto b=[=] { return x; }; return a(b()); }',
 'typed_exceptions':'void f() throw(int,int*); void g() try { throw 1; } catch(int x) { throw; } catch(...) {}',
 'traits':'int f(int x) { return sizeof(int*) + alignof(int) + noexcept(x+1); }',
 'qualified_expression':'int value=(limits::epsilon())*2; int f() { return N::f()+::g(); }',
}
syntax_only={'qualified_expression'}
with tempfile.TemporaryDirectory() as d:
    d=Path(d)
    for name,source in cases.items():
        p=d/(name+'.cpp'); p.write_text(source)
        if name not in syntax_only:
            for h in ['g++','clang++']:
                r=subprocess.run([h,'-std=c++11','-fsyntax-only',str(p)],capture_output=True)
                assert r.returncode==0,(name,h,r.stderr.decode())
        r=run(TOOL,p,d/'student.ast'); assert r.returncode==0,(name,r.stdout,r.stderr)
        r=run(REF,p,d/'reference.ast'); assert r.returncode==0,(name,r.stdout,r.stderr)
        assert (d/'student.ast').read_bytes()==(d/'reference.ast').read_bytes(),name
    # Distinguish syntactic rejection from course's deliberately unresolved semantics.
    for i,source in enumerate(['int f( {', 'int f() { return 1 +; }',
                               'int f() { if(x) return 1;', 'int f() { try {} }',
                               'int f() { auto a=[&x y]{}; }', '[[x(]];']):
        p=d/f'bad{i}.cpp'; p.write_text(source)
        assert run(TOOL,p,d/'bad.ast').returncode!=0,source
    # Explicit driver and TU reset; no hidden process-environment telemetry.
    p=d/'one.cpp'; p.write_text('typedef int name; int f() { name x=1; return x; }')
    q=d/'two.cpp'; q.write_text('int f() { int name=1; return name; }')
    r=subprocess.run([TOOL,'--emit-ast','-o',d/'multi.ast',p,q],capture_output=True)
    assert r.returncode==0,r.stdout+r.stderr
    assert (d/'multi.ast').read_text().count('start translation unit')==2
    for args in [[],['--emit-ast'],['--emit-ast','-o',str(d/'out')],['--emit-ast',str(p)],['--emit-ast','-c','-o',str(d/'out'),str(p)]]:
        assert subprocess.run([TOOL,*args],capture_output=True).returncode!=0,args
print('PA5 explicit controls passed: portable file + 10 families + 6 rejections + driver/TU reset')
