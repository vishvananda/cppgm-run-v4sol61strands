#!/usr/bin/env python3
"""Explicit portable template/dependent interactions and exact syntax views."""
from pathlib import Path
import os, subprocess, tempfile, json
R=Path(__file__).resolve().parents[2]
T=Path(os.environ.get('PA5_TOOL',R/'dev/cppgm++'));
A=Path(os.environ.get('RALPH_ARTIFACT_DIR','/tmp'))/'pa5/loop13'; A.mkdir(parents=True,exist_ok=True)
cases={
 'identity':'template<class value> value identity(value x) { return x; } int f() { return identity<int>(1); }',
 'nested_shift':'template<int n> struct leaf {}; template<class value> struct box {}; box<box<leaf<(8 >> 1)>>> x;',
 'nested_compare':'template<bool n> struct leaf {}; template<class value> struct box {}; box<leaf<(4 > 1)>> x;',
 'parameter_kinds':'template<template<class> class item, class value, int n=2> struct box {}; template<class value> struct leaf {}; box<leaf,int> x;',
 'parameter_reset':'template<template<class> class item> struct a {}; template<class item> struct b { item f(item x) {return x;} };',
 'parameter_shadow':'template<class value> struct type {}; bool f(int type) {return type < 2 || type > 4;}',
 'dependent_member':'template<class value> auto f(value x)->decltype(x.template get<0>()) {return x.template get<0>();}',
 'dependent_scope':'template<class value> struct box {template<class other> struct item {};}; template<class value> using alias=typename box<value>::template item<int>;',
 'logical_arguments':'template<bool> struct test {}; template<class value> struct trait {static const bool flag=true;}; template<class value> struct box: test<trait<value>::flag && (trait<int>::flag || !trait<char>::flag)> {};',
 'specialization':'template<class value> struct box {}; template<class value> struct box<value*> {}; template<> struct box<int> {}; extern template struct box<char>;',
 'function_instantiation':'template<class value> void f(value) {} extern template void f<int>(int); template void f<char>(char);',
 'class_instantiation':'template<int n> struct box {}; extern template struct box<1>; template struct box<2>; box<3> x;',
 'class_template_constructor':'template<class value> struct box {box();}; template<class value> box<value>::box() {}',
 'dependent_conversion_argument':'template<bool> struct check {}; template<class value> struct sign:check<value(-1) < value(0)> {}; sign<int> x;',
 'conversion_argument':'template<bool> struct check {}; template<class values> using checks=check<bool(values::flag)>; struct tag {static const int flag=1;}; checks<tag> x;',
 'rendered_types':'template<class value> struct box {}; box<unsigned long> a; box<int const volatile> b; box<volatile unsigned long const*> c; template<class value> auto f(value x)->box<value*> {return {};}',
 'trait_arguments':'template<bool> struct check {}; check<noexcept(1+2)> x; check<(alignof(int)>0)> y; template<class value> auto f()->decltype(typename value::type{}) {return {};}',
 'member_pointer_argument':'template<class value> struct box {}; struct tag {int member;}; template<class value> using alias=box<int value::*>; alias<tag> x;',
 'parameter_attribute':'template<class value> value f([[unused]] value x) {return x;}',
 'function_pointer_argument':'template<class value> struct callback {}; template<class value> using alias=callback<value(*)[]>; alias<int> x;',
 'function_argument':'template<class value> struct callback {}; typedef int word; callback<bool(word,const word&)> x;',
 'operator_template':'template<class value> struct box {}; template<class value> int operator+(box<value>,box<value>); int f(box<int> x) {return operator+<int>(x,x) < 2;}',
 'literal_operator':'template<char... chars> unsigned long long operator "" _count(); auto x=operator "" _count<\'1\',\'2\'>();',
 'pack_base':'template<class... values> struct box:values... {box():values()... {}};',
 'pack_call':'template<class... values> void consume(values...); template<class... values> void f(values... x) {consume(x...);}',
 'direct_initializer':'struct tag {}; int next(); struct holder {holder(tag,int);}; void f() {holder x(tag(),next());}',
 'qualified_direct':'namespace n {struct tag {}; int next();} struct holder {holder(n::tag,int);}; void f() {holder x(n::tag(),n::next());}',
 'decltype_member':'template<class value> value declval(); template<class value> auto f(value x)->decltype(declval<value>().operator->()) {return x.operator->();}',
 'late_template_default':'struct box { int f(int x=helper<int,long>()) {return x;} template<class a,class b> static int helper() {return 1;} };',
 'late_template_initializer':'struct box { int x=helper<int,long>(); template<class a,class b> static int helper() {return 1;} };',
 'nested_lambda':'template<class value> int f(value x) {auto fn=[&x](int n) {return x.template get<2>() + n;}; return fn(1);}',
 'declaration_preference':'template<class value> struct box {}; void f() {box<int>(x); box<int>(y)->member=1;}',
}
# declaration_preference deliberately only syntax-valid (member expression has no
# semantic object); portable counterpart supplies an operator-> below.
cases['declaration_preference']='template<class value> struct box {box(); box(int); box* operator->(); int member;}; void f(int y) {box<int>(x); box<int>(y)->member=1;}'
observations={}; failures=[]
with tempfile.TemporaryDirectory() as d:
 d=Path(d)
 # The integration includes a standard dependent conversion rejected by the
 # reference (reference-notes.md). Qualify it independently, not by weakening
 # the exact comparisons for all reference-supported families below.
 integration=R/'student.tests/pa5/templates-portable.cpp'
 for host in ['g++','clang++']:
  r=subprocess.run([host,'-std=c++11','-fsyntax-only',integration],capture_output=True)
  if r.returncode: failures.append(('integration',host,r.stderr.decode()))
 r=subprocess.run([T,'--emit-ast','-o',d/'integration.ast',integration],capture_output=True)
 if r.returncode: failures.append(('integration','student',r.stderr.decode()))
 for name,text in cases.items():
  source=d/(name+'.cpp');source.write_text(text)
  row={};observations[name]=row
  for host in ['g++','clang++']:
   r=subprocess.run([host,'-std=c++11','-fsyntax-only',source],capture_output=True)
   row[host]=dict(exit=r.returncode,diagnostic=r.stderr.decode())
   if r.returncode: failures.append((name,host,r.stderr.decode()))
  for variant,tool in [('student',T),('reference',R/'reference-binaries/cppgm++')]:
   out=d/(name+'-'+variant+'.ast')
   r=subprocess.run([tool,'--emit-ast','-o',out,source],capture_output=True)
   row[variant]=dict(exit=r.returncode,diagnostic=(r.stdout+r.stderr).decode())
   if out.exists(): (A/(name+'-'+variant+'.ast')).write_bytes(out.read_bytes())
   if variant=='student' and r.returncode: failures.append((name,variant,row[variant]))
  if row['student']['exit']==row['reference']['exit']==0:
   row['equal']=(A/(name+'-student.ast')).read_bytes()==(A/(name+'-reference.ast')).read_bytes()
   if not row['equal']: failures.append((name,'tree mismatch'))
 for i,text in enumerate(['template<class T struct x {};','template<class T> struct x {','template<int n=> struct x {};','template<class T> struct x {}; x<x<int> y;','template<template<class> struct x {};','template<class T> void f(T) {} template void f<int>(int) {}']):
  source=d/'bad.cpp';source.write_text(text)
  r=subprocess.run([T,'--emit-ast','-o',d/'bad.ast',source],capture_output=True,timeout=10)
  if r.returncode!=1: failures.append(('rejection',i,r.returncode))
(A/'template-controls.json').write_text(json.dumps(observations,indent=2))
assert not failures,failures
print('Template controls passed:',len(cases),'portable families + 6 grammar rejections')
