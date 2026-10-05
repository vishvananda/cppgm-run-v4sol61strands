#!/usr/bin/env python3
"""Portable namespace/enum category controls, not course fixture substitutes."""
from pathlib import Path
import os, subprocess, tempfile
R=Path(__file__).resolve().parents[2]
T=Path(os.environ.get('PA5_TOOL',R/'dev/cppgm++')).resolve()
REF=R/'reference-binaries/cppgm++'
portable=R/'student.tests/pa5/scopes-portable.cpp'
cases={
 'namespace_reopen':'namespace plain { typedef int word; } namespace plain { word f(word x) { return x; } } plain::word x;',
 'alias_shadow':'namespace plain { typedef int word; } namespace alias=plain; void f(){ int alias=1; alias::word x; x=alias; }',
 'inline':'namespace lib { inline namespace abi { using word=int; } } lib::word x;',
 'unnamed':'namespace lib { namespace { typedef int word; } namespace { word x; } word y; }',
 'using_transitive':'namespace a { typedef int word; } namespace b { using namespace a; } namespace c { using namespace b; word x; }',
 'using_common_ancestor':'namespace source{int word=1;} namespace owner{using word=int; namespace inner{using namespace source; word x;} void f(){using namespace source; word y;}}',
 'using_transitive_ancestor':'namespace source{int word=1;} namespace transit{using namespace source;} namespace owner{using word=int; namespace inner{using namespace transit; word x;}}',
 'using_namespace_only':'namespace a{using word=int;} void f(){int a=1; using namespace a; word x=a;}',
 'using_cycle':'namespace a { typedef int word; } namespace b { using namespace a; } namespace a { using namespace b; } void f(){ using namespace b; word x; }',
 'using_shadow':'namespace a { typedef int word; } void f(){ using a::word; word x; { int word=1; word*=2; } word y; }',
 'qualified_value':'namespace Values { int fetch(int x){ return x; } } int f(){ return sizeof(Values::fetch(1)); }',
 'qualified_cast':'namespace plain { typedef unsigned word; } unsigned f(unsigned x){ plain::word(x+1); return (plain::word)(x)+plain::word(x); }',
 'qualified_decl':'namespace plain { typedef int word; } void f(){ plain::word(x); plain::word (*p)(int); plain::word y[2]; }',
 'enum_alias':'enum class mode:int { off, on=1 }; typedef mode other; using third=other; int f(){ return int(third::on); }',
 'enum_shadow':'enum class lower { off, on=1 }; int f(int lower){ return int(lower::on)+lower; }',
 'unscoped_enum':'enum lower { a,b=a+1,c=b+1, }; int f(){ return lower::c+b; }',
 'enum_objects':'enum lower { a,b }; enum lower first=a, *ptr=&first; typedef enum { c,d=c+1 } other; other second=d;',
 'enum_cv':'enum lower { a } const x=a; enum class upper { b } const y=upper::b;',
 'paren_qualified':'namespace a { using word=int; } int f(){ return (a::word(1))+(a::word()); }',
 'enum_opaque':'enum class lower: unsigned; enum class lower: unsigned { a }; lower f(){ return lower::a; }',
 'global':'namespace a { using word=int; } namespace b { using word=double; ::a::word f(::a::word x){ return x; } }',
 'namespace_siblings':'namespace a { using word=int; } namespace b { int word=1; int f(){ return word<2; } } a::word x;',
 'qualified_condition':'namespace a { using word=int; } int f(int x){ if(a::word y=x) return y; for(a::word i=0;i<x;++i) x-=i; return x; }',
}
with tempfile.TemporaryDirectory() as d:
 d=Path(d)
 for name,source in [('portable',portable.read_text())]+list(cases.items()):
  p=d/(name+'.cpp'); p.write_text(source)
  for h in ['g++','clang++']:
   r=subprocess.run([h,'-std=c++11','-fsyntax-only',p],capture_output=True)
   assert r.returncode==0,(name,h,r.stderr)
  r=subprocess.run([T,'--emit-ast','-o',d/'student.ast',p],capture_output=True)
  assert r.returncode==0,(name,r.stdout,r.stderr)
  text=(d/'student.ast').read_text()
  assert 'translation-unit' in text
  if name=='qualified_value': assert 'sizeof-expression\n' in text and 'call-expression' in text and 'type-id' not in text
  if name=='namespace_siblings': assert 'binary-expression OP_LT:<' in text
  if name=='alias_shadow': assert 'decl-specifier alias::word' in text
  if name=='enum_objects': assert text.count('enum-specifier')==3
  # Reference is observation only. New ordinary-qualified alias/cast handling
  # is qualified above and checked through the retained graph, not an oracle fix.
  if name in ['namespace_reopen','alias_shadow','inline','using_transitive','using_cycle','using_shadow','unscoped_enum','enum_opaque','global','namespace_siblings']:
   r=subprocess.run([REF,'--emit-ast','-o',d/'reference.ast',p],capture_output=True)
   assert r.returncode==0,(name,r.stdout,r.stderr)
   assert (d/'student.ast').read_bytes()==(d/'reference.ast').read_bytes(),name
 for name,s in {'namespace_close':'namespace a { int x;','enum_initializer':'enum lower { a= };','enum_comma':'enum lower { a,,b };','empty_enum_forward':'enum;','using_missing':'using namespace ;','alias_missing':'namespace a=;'}.items():
  p=d/(name+'.cpp');p.write_text(s)
  assert subprocess.run([T,'--emit-ast','-o',d/'bad.ast',p],capture_output=True).returncode!=0,name
print('PA5 namespace/enum controls passed: portable integration + 23 families + 6 rejections')
