#!/usr/bin/env python3
"""Final whole-stage category, class-completion and dependent syntax interactions."""
from pathlib import Path
import subprocess,json
R=Path(__file__).resolve().parents[2]
import os
A=Path(os.environ.get('RALPH_ARTIFACT_DIR','/tmp'))/'pa5/loop14/interactions';A.mkdir(parents=True,exist_ok=True)
cases={
'decltype_type_argument':'struct base {typedef int type;}; base x; template<class v> struct box {}; box<decltype(x)::type> value;',
'decltype_qualified_typename':'template<class v> void f(v x) {typename decltype(x)::type y;}',
'decltype_new':'struct base {typedef int type;}; base x; void f(){auto p=new decltype(x)::type;delete p;}',
'qualified_param_shadow':'template<class value> struct box {template<class value2> value2 f(value2);}; template<class value> template<class value2> value2 box<value>::f(value2 x) {value y{}; return x;}',
'qualified_member_return':'template<class value> struct box {using type=value; template<class other> type f(other);}; template<class value> template<class other> typename box<value>::type box<value>::f(other x) {type y{};return y;}',
'enum_local_class':'int f(){struct x {enum type {a};}; return x::a;}',
'typename_declaration':'template<class v> void f() {typename v::type y;}',
 'typename_function_declaration':'template<class v> void f() {typename v::type y();}',
 'typename_parenthesized_declaration':'template<class v> void f() {typename v::type (y);}',
 'typename_parenthesized_pointer':'template<class v> void f() {typename v::type (*y)();}',
 'typename_parenthesized_reference':'template<class v> void f(int x) {typename v::type (&y)=x;}',
 'typename_function_conversion':'template<class v> void f() {typename v::type(1);}',
 'typename_mixed':'template<class v> void f() {typename v::type y{}; auto x=typename v::type();}',
 'decltype_template':'template<class v> void f(v x) {typename decltype(x)::template type<int> y;}',
 'member_body_enum':'struct x {int f(){enum type {a}; return a;}};',
 'member_body_class_enum':'struct x {int f(){struct y {enum type {a};}; return y::a;}};',
 'nested_class_template':'template<class outer> struct box {template<class inner> struct member {inner f(inner);};}; template<class outer> template<class inner> inner box<outer>::member<inner>::f(inner x) {return x;}',
 'multi_scope_template':'''namespace n { template<class v> struct box { using type=v; v get(v x=sizeof(late)) {return x;} struct late {}; }; } namespace m { namespace alias=n; using alias::box; template<class v> using wrap=box<box<v>>; } m::wrap<int> value;''',
'out_of_class_template':'''namespace n { template<class v> struct box { box(); ~box(); template<class u> u f(u); }; template<class v> box<v>::box() {} template<class v> box<v>::~box() {} template<class v> template<class u> u box<v>::f(u x) {return x;} }''',
'mixed_default_tail':'''struct outer { typedef int type; int f(int x=sizeof(late), type y=sizeof(type)) noexcept(sizeof(late)>0) { return x+y; } struct late {}; };''',
'import_completion':'''namespace n { typedef int type; } struct base { using type=n::type; }; struct outer:base { int f(type x=sizeof(late)) {return x;} struct late {}; };''',
'friend_template':'''template<class v> struct box { template<class u> friend u f(u x) {return x;} }; box<int> value;''',
'dependent_lambda':'''template<class v> auto f(v x)->decltype(x()) {auto g=[x](v y)->decltype(y()) {return y();}; return g(x);}''',
'alias_function_pointer':'''template<class v> using fn=v (*)(v); fn<int> ptr;''',
'pack_bases':'''template<class... bases> struct derived:bases... { derived():bases()... {} }; struct a {}; struct b {}; derived<a,b> value;''',
'noexcept_pack':'''template<class... values> bool f(values... x) noexcept(noexcept(sizeof...(values))) { return sizeof...(x)>0; }''',
'conversion_const':'''template<class v> struct box { operator v() const { return v(); } }; box<int> x;''',
'function_try_template':'''template<class v> struct box { box() try :data{} {} catch(...) {} v data; };''',
'array_function_type':'''template<class v> struct box {}; box<int[3]> a; box<int(int)> b; box<int(*)(int)> c;''',
'condition_using':'''namespace n { typedef int type; } int f(int x) {using n::type; if(type value=x) return value; for(type i=0;i<x;++i) x+=i; return x;}''',
'decltype_qualifier':'''struct base {typedef int type;}; base x; decltype(x)::type value;''',
'qualified_member':'''struct base {typedef int type;}; struct d:base { int f(base::type x) {return x;} };''',
'global_class_ctor':'''struct x {x();}; ::x::x() {}''',
'operator_template':'''struct x {}; template<class v> int operator+(x,v) {return 1;} int f(x a){return operator+<int>(a,1);}''',
'using_pack':'''template<class... values> struct box {template<class... other> using type=box<values...,other...>;}; box<int>::type<char> value;''',
'non_type_ptr':'''int x; template<int* p> struct box {}; box<&x> value;''',
'member_default':'''struct x {typedef int type; int f(type a=type(1), type b=type()) {return a+b;}};''',
'alignas_type':'''struct x { alignas(int) char a; alignas(16) int b; };''',
'initializer_templates':'''template<class v> struct box {v value;}; box<int> f() {return box<int>{1};}''',
'nested_enum':'''struct x { enum class type:int {a=1}; type f(type t=type::a) {return t;} };''',
'new_template':'''template<class v> struct box {v value;}; void f() { auto p=new box<int>{1}; delete p; }''',
'placement_template':'''void* operator new(decltype(sizeof(0)),void*); template<class v> struct box {}; void f(void* x) {new(x) box<int>{};}''',
}
rows=[]
for name,source in cases.items():
 p=A/(name+'.cpp');p.write_text(source+'\n');codes={};stderr={}
 for tool,cmd in [('gcc',['g++','-std=c++11','-fsyntax-only']),('clang',['clang++','-std=c++11','-fsyntax-only']),('student',[Path(os.environ.get('PA5_TOOL',R/'dev/cppgm++')),'--emit-ast','-o',A/(name+'.ast')]),('reference',[R/'reference-binaries/cppgm++','--emit-ast','-o',A/(name+'.ref')])]:
  r=subprocess.run(list(map(str,cmd+[p])),capture_output=True);codes[tool]=r.returncode;stderr[tool]=r.stderr.decode()
  assert b'AddressSanitizer' not in r.stderr and b'runtime error:' not in r.stderr,(name,tool,r.stderr)
 equal=None
 if not codes['student'] and not codes['reference']: equal=(A/(name+'.ast')).read_bytes()==(A/(name+'.ref')).read_bytes()
 rows.append(dict(name=name,codes=codes,equal=equal,stderr=stderr))
 assert all(c==0 for c in codes.values()),(name,codes,stderr)
 assert equal,(name,'AST mismatch')
 if os.environ.get('PA5_GRAPH'):
  r=subprocess.run([os.environ['PA5_GRAPH'],str(p)],capture_output=True)
  assert r.returncode==0,(name,r.stdout,r.stderr)
(A/'probes.json').write_text(json.dumps(rows,indent=2))

print(f'PA5 final-audit interactions passed: {len(cases)} GCC/Clang-qualified exact reference comparisons')
