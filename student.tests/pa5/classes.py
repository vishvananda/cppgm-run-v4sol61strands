#!/usr/bin/env python3
"""Portable class/member capability controls and full explicit-view comparisons."""
import os, subprocess, tempfile
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path(os.environ.get('PA5_TOOL',R/'dev/cppgm++'))
REF=R/'reference-binaries/cppgm++'
cases={
 'alignment':'struct alignas(16) lower { alignas(8) int x; };',
 'late_body':'struct lower { int f() { inner x{}; return x.value; } struct inner { int value; }; };',
 'default_point':'typedef int kind; struct lower { int f(int x=sizeof(kind), int kind=0) {return x+kind;} };',
 'default_own_shadow':'typedef int kind; struct lower { int f(int kind=sizeof(kind)); };',
 'default_nested_lambda':'typedef int kind; struct lower { int f(int x=[](int kind){return sizeof(kind);}(0), int kind=0); };',
 'late_defaults':'struct lower { int f(int x=sizeof(inner)) noexcept(sizeof(inner)>0) { return x; } struct inner { int value; }; };',
 'late_members':'struct lower { int size=sizeof(inner); int other{sizeof(inner)}; struct inner { char data[3]; }; };',
 'nested_complete':'struct lower { struct middle { int f(){ inner x{}; return x.value; } }; struct inner { int value; }; };',
 'local_complete':'void f(){ struct lower { int g(){ inner x{}; return x.value; } struct inner { int value; }; }; }',
 'local_in_deferred':'struct lower { int f(){ struct local { int g(){ inner x{}; return x.value; } struct inner { int value; }; }; local x; return x.g(); } };',
 'nested_function_try':'struct lower { lower() try : x{1} {} catch(...) { throw; } int x; };',
 'inherited_type':'struct base { typedef int word; }; int word; struct derived:base { word f(word x){ return (word)x; } };',
 'base_shadow':'struct base { typedef int kind; }; int kind; struct derived:base { int kind; int f(){ return kind+2; } };',
 'diamond':'struct root{typedef int word;}; struct left:virtual root{}; struct right:virtual root{}; struct lower:left,right { word f(word x){return x;} };',
 'qualified_scope':'struct lower { typedef int word; word f(word); }; int word; lower::word lower::f(word x){ word y=x; return y; }',
 'qualified_conversion':'namespace detail { struct lower { typedef int word; operator word()const; }; lower::operator word()const{return 1;} }',
 'special_specifiers':'struct lower { explicit lower(int){} virtual ~lower(){} };',
 'special_initializers':'struct lower { lower()=default; lower(const lower&)=delete; lower& operator=(const lower&)=default; };',
 'class_object_forms':'struct lower {int x;} object, other{}; typedef struct {int x;} word; typedef union{int x;long y;} storage;',
 'member_pointer':'namespace n{typedef int word; struct lower{};} void f(){n::word (n::lower::*p)()=0; (void)p;}',
 'operator_calls':'struct lower{int operator()(int x){return x;} int operator[](int x){return x;}}; int f(lower& x){return x.operator()(1)+x.operator[](2);}',
 'virt_suffix':'struct base {virtual void f();}; struct lower:base {void f() override;};',
 'bit_fields':'struct lower { unsigned a:1,b:2; unsigned:0; char c:0?1:2; };',
 'decltype_base':'namespace n{struct base{};} struct lower:decltype(n::base()){lower():decltype(n::base())(){}};',
 'declaration_preference':'struct lower{lower(int=0,int=0); operator int() const;}; void f(int x){lower(a);lower(x,2)+x;lower b(int());}',
 'scalar_literal_retention':'struct lower{ int f(){ return 28; } const char* text="retained"; char c=\'q\'; int n{28}; };',
}
# Every new portable case is qualified on the identical bytes by both hosts.
with tempfile.TemporaryDirectory() as d:
 d=Path(d)
 for name,source in [('integration',(R/'student.tests/pa5/classes-portable.cpp').read_text())]+list(cases.items()):
  p=d/(name+'.cpp'); p.write_text(source)
  for h in ['g++','clang++']:
   r=subprocess.run([h,'-std=c++11','-fsyntax-only',p],capture_output=True)
   assert r.returncode==0,(name,h,r.stderr.decode())
  reference_gaps={'inherited_type','qualified_scope','default_own_shadow','default_nested_lambda'}
  for tool,suffix in [(T,'ast'),(REF,'ref')]:
   r=subprocess.run([tool,'--emit-ast','-o',d/suffix,p],capture_output=True)
   if tool==REF and name in {'inherited_type','qualified_scope'}:
    assert r.returncode!=0,(name,'reference gap unexpectedly resolved; review parity')
   else:
    assert r.returncode==0,(name,tool,r.stdout.decode(),r.stderr.decode())
  if name not in reference_gaps:
   assert (d/'ast').read_bytes()==(d/'ref').read_bytes(),name
  else:
   text=(d/'ast').read_text()
   if name=='inherited_type':
    assert 'parameter-declaration' in text and 'cast-expression' in text
   elif name=='qualified_scope':
    assert 'decl-specifier TT_IDENTIFIER:word' in text
   else:
    assert 'sizeof-expression\n' in text and 'id-expression kind' in text
 for i,source in enumerate(['struct lower{', 'struct lower { int a:; };','struct lower { lower():x(1) };','struct lower { int f(){ return 1; }','struct lower { int f(int x=); };','struct lower { int x={1; };']):
  p=d/f'reject{i}.cpp';p.write_text(source)
  r=subprocess.run([T,'--emit-ast','-o',d/'bad',p],capture_output=True)
  assert r.returncode!=0,(i,source)
print(f'PA5 class controls passed: integration + {len(cases)} portable families + 6 grammar rejections')
