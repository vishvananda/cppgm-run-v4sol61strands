#!/usr/bin/env python3
"""Independent PA4 whole-stage interactions, including the course/host boundary."""
import hashlib
import json
import os
from pathlib import Path
import random
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TOOL = Path(os.environ.get('PA4_TOOL', ROOT/'dev/preproc')).resolve()
A = Path(os.environ.get('RALPH_ARTIFACT_DIR', tempfile.gettempdir()))/'pa4-audit'
A.mkdir(parents=True, exist_ok=True)
ledger = []

def records(text):
    return [x for x in text.splitlines() if not x.startswith(('preproc ', 'sof ', 'eof'))]

def check(name, source, expected=None, companions=None, hosts=True, success=True, sources=None, reference_agrees=True):
    with tempfile.TemporaryDirectory() as d:
        d = Path(d)
        (d/'input.cc').write_text(source)
        for path, text in (companions or {}).items():
            target = d/path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text)
        # One inode, two spellings: pragma once must not depend on path equality.
        if (d/'once.hh').exists():
            os.link(d/'once.hh', d/'alias.hh')
        args = sources or ['input.cc']
        observations = {}
        for variant, command in [('student', [str(TOOL)]), ('reference', [str(ROOT/'dev/preproc-ref')])]:
            out = d/(variant+'.dump')
            r = subprocess.run([*command, '-o', str(out), *args], cwd=d, capture_output=True, text=True)
            observations[variant] = {'exit': r.returncode, 'stderr': r.stderr, 'records': records(out.read_text()) if out.exists() else []}
        student = observations['student']
        assert (student['exit']==0)==success, (name, student)
        if success:
            if reference_agrees:
                assert observations['reference']['exit']==0, (name, observations)
                assert student['records']==observations['reference']['records'], (name, observations)
            else:
                assert expected is not None, 'reference disagreement requires an independent contract result'
            if expected is not None:
                assert student['records']==expected, (name, student['records'], expected)
        elif hosts:
            assert observations['reference']['exit']!=0, (name, observations)
        if hosts:
            for host in ('g++', 'clang++'):
                r = subprocess.run([host, '-std=c++11', '-E', '-P', '-x', 'c++', args[0]], cwd=d, capture_output=True, text=True)
                observations[host] = {'exit': r.returncode, 'stdout': r.stdout, 'stderr': r.stderr}
                assert (r.returncode==0)==success, (name, host, r.stderr)
                if success:
                    # Independently literal-only portable cases; tokenize host output
                    # with the inherited post-token observation, not the preprocessor.
                    host_text = '\n'.join(line for line in r.stdout.splitlines() if not line.lstrip().startswith('#pragma'))
                    p = subprocess.run([str(ROOT/'dev/posttoken')], input=host_text, capture_output=True, text=True)
                    assert p.returncode==0, (name, host, p.stderr)
                    want = [x for x in p.stdout.splitlines() if x!='eof']
                    assert want==student['records'], (name, host, want, student['records'])
        ledger.append({'name': name, 'source': source, 'companions': companions or {},
                   'sources': args, 'hosts_compared': hosts, 'reference_agrees': reference_agrees,
                   'expected': expected, 'sha256': hashlib.sha256(source.encode()).hexdigest(),
                   'observations': observations})

check('rescan-stringize-paste-redefine', '''#define S1(x) #x
#define S(x) S1(x)
#define CAT(a,b) a##b
#define F(x) S(x)
F(a CAT(b,c) "hi") ;
#define G(x) H(x)
#define H(x) G(x)
G(4)
#undef G
#define G(x) x
G(5)
#line 200 "virtual.cc"
#define L __LINE__
#define V(x) x
V(L)
''')
check('empty-variadic-placemarker', '''#define C(a,b) a##b
#define V(x,...) x + __VA_ARGS__
#define S(...) #__VA_ARGS__
C(,1) C(2,) C(,) V(3,4,5) S() ; S(a, b) ;
''')
check('lazy-typed-conditional-inactive', '''#define FLAG 7
#if defined FLAG && FLAG==7 && (1 || 1/0) && ((1? -1 : 1U)>0)
done
#else
#error fail
#endif
#if 0
#include "missing.hh"
#error ignored
#if !this is @ invalid
bad
#endif
#elif (8>>2)==2 && ('a'==97) && (0 && (1/0))==0
done
#else
#error fail
#endif
''', ['identifier done']*2)
check('once-alias-repeat-header-condition', '''#define ENABLE 1
#include "once.hh"
#include "alias.hh"
#include "once.hh"
#include "ordinary.hh"
#undef ENABLE
#define ENABLE 2
#include "ordinary.hh"
VALUE
''', ['literal 31 int 1F000000', 'literal 1 int 01000000', 'literal 2 int 02000000', 'literal 31 int 1F000000'],
      {'once.hh': '#pragma once\n#define VALUE 31\nVALUE\n', 'ordinary.hh': '#if ENABLE==1\n1\n#else\n2\n#endif\n'})
# Course include search is deliberately presumed-__FILE__ relative, unlike hosts.
check('presumed-include-minimal-reducer', '#line 80 "virtual/input.cc"\n#include "next.hh"\n',
      ['literal 19 int 13000000'], {'virtual/next.hh': '19\n'},
      hosts=False, reference_agrees=False)
check('line-changes-include-search-and-pragma-origin', '''#line 80 "virtual/input.cc"
#define HEADER "next.hh"
#include HEADER
#include "once.hh"
#include "alias.hh"
__LINE__
''', ['literal 19 int 13000000', 'literal 23 int 17000000', 'literal 84 int 54000000'],
      {'virtual/next.hh': '19\n', 'once.hh': '#line 600 "pretend.hh"\n#define P _Pragma("once")\nP\n23\n'}, hosts=False, reference_agrees=False)
check('primary-reset', '#define M 1\n#pragma once\n#include "once.hh"\nM\n',
      ['literal 7 int 07000000','literal 1 int 01000000','literal 7 int 07000000','identifier M'],
      {'once.hh':'#pragma once\n7\n', 'second.cc':'#include "once.hh"\nM\n'}, hosts=False, sources=['input.cc','second.cc'])
check('predefined-once-prescan-counter', '''#define DUP(x) x x x
#define P _Pragma("ignored")
P
DUP(__COUNTER__)
__COUNTER__
''', ['literal 0 int 00000000']*3+['literal 1 int 01000000'])
check('raw-literal-argument-and-concatenation', '''#define ID(x) x
#define STR(x) #x
ID(R"tag(a, (b) \\n)tag") "end" ; STR(a /*comment*/ b) ;
''')
for name, source in [
    ('argument-mismatch','#define F(x,y) x+y\nF(1)\n'),
    ('invalid-paste','#define C(a,b) a##b\nC(+,*)\n'),
    ('invalid-active-condition','#if 1/0\ndone\n#endif\n'),
    ('invalid-selected-elif','#if 0\n#elif 1/0\n#endif\n'),
    ('unmatched-endif','#endif\n'),
    ('missing-include','#include "missing.hh"\n'),
    ('active-error','#if 1\n#error active\n#endif\n')]:
    check(name, source, success=False)
# Explicit course paint divergence: inherited contract, not a host oracle.
check('course-paint-boundary', '#define f(x) 1 x\n#define g(x) 2 x\ng(f)(g)(3)\n',
      ['literal 2 int 02000000', 'literal 1 int 01000000', 'identifier g', 'simple ( OP_LPAREN', 'literal 3 int 03000000', 'simple ) OP_RPAREN'], hosts=False)
# Actual required work with many distinct headers, once aliases and repeat includes.
count = 500
headers = {f'headers/h{i}.hh':f'#pragma once\n#define V{i} {i}\n' for i in range(count)}
source = ''.join(f'#include "headers/h{i}.hh"\n' for i in range(count))
source += ''.join(f'#include "headers/../headers/h{i}.hh"\nV{i}\n' for i in range(count))
check('many-distinct-header-identities', source, companions=headers)
# Independently evaluate randomized acyclic macro trees, combined with paste,
# repeated arguments, lazy controlling expressions and redefinition.
rng = random.Random(4400406)
for batch in range(12):
    defs = '#define CAT(a,b) a##b\n#define TWICE(x) ((x)+(x))\n#define ZERO(x) 0\n'
    lines = []
    for i in range(30):
        values = [rng.randrange(1,20) for _ in range(4)]
        a,b,c,d = values
        value = 2*(a+b)*(c+d)
        expression = f'(TWICE({a}+{b})*({c}+{d}))'
        defs += f'#define M{i} {expression}\n'
        lines += [f'#if CAT(M,{i})=={value} && (0 ? 1/0 : 1)\n{value}\n#else\n#error wrong\n#endif\n']
    check(f'random-combined-{batch}', defs+''.join(lines))
tag = os.environ.get('PA4_AUDIT_LABEL', hashlib.sha256(TOOL.read_bytes()).hexdigest()[:12])
destination = A/f'semantic-observations-{tag}.json'
repeat = 0
while destination.exists():
    repeat += 1
    destination = A/f'semantic-observations-{tag}-{repeat}.json'
destination.write_text(json.dumps(ledger, indent=2))
(A/'semantic-observations.json').write_text(json.dumps(ledger, indent=2))
print(f'PA4 independent audit: {len(ledger)} interaction families/batches passed; raw host/reference/student observations: {destination}')
