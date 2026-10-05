#!/usr/bin/env python3
"""Explicit PA4 behavior controls, portable GCC/Clang qualification and scale."""
import os
import pathlib
import random
import re
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = pathlib.Path(os.environ.get('PA4_TOOL', ROOT/'dev/preproc'))

def invoke(source, want=True, companions=None):
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        (tmp/'input.cc').write_text(source)
        for name, contents in (companions or {}).items():
            (tmp/name).write_text(contents)
        result = subprocess.run([str(TOOL), '-o', str(tmp/'out'), 'input.cc'], cwd=tmp,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert (result.returncode == 0) == want, (source[:1000], result.stderr)
        return (tmp/'out').read_text() if want else ''

def records(out):
    return [x for x in out.splitlines() if not x.startswith(('preproc ', 'sof ', 'eof'))]

def normalized(text):
    return re.findall(r'[A-Za-z_][A-Za-z_0-9]*|[0-9]+|[^\s]', text)

def portable(source, expected):
    for host in ('g++', 'clang++'):
        run = subprocess.run([host, '-std=c++11', '-E', '-P', '-x', 'c++', '-'],
                             input=source, text=True, capture_output=True)
        assert run.returncode == 0, (host, run.stderr, source[:1000])
        assert normalized(run.stdout) == normalized(expected), (host, run.stdout, expected)

cases = [
    ('#define A(x) x+x\nA(7)\n', '7 + 7', ['literal 7 int 07000000', 'simple + OP_PLUS', 'literal 7 int 07000000']),
    ('#define F(x) x\n#define G F\nG\n(8)\n', '8', ['literal 8 int 08000000']),
    ('#define CAT(a,b) a##b\n#define XY 17\nCAT(X,Y)\n', '17', ['literal 17 int 11000000']),
    ('#define B(a,b) a##b\nB(,6) B(5,) B(,)\n', '6 5', ['literal 6 int 06000000','literal 5 int 05000000']),
    ('#define R(x) R(x)\nR(1)\n', 'R(1)', ['identifier R','simple ( OP_LPAREN','literal 1 int 01000000','simple ) OP_RPAREN']),
    ('#define V(x,...) x __VA_ARGS__\nV(1,2,3)\n', '1 2,3', ['literal 1 int 01000000','literal 2 int 02000000','simple , OP_COMMA','literal 3 int 03000000']),
    ('#if 0 && 1/0\n#error bad\n#elif defined(__cplusplus) && __cplusplus==201103L\n4\n#endif\n', '4', ['literal 4 int 04000000']),
    ('#line 41 "virtual.cc"\n__LINE__\n#line 12\n__LINE__\n', '41 12', ['literal 41 int 29000000','literal 12 int 0C000000']),
]
for source, expected, tokens in cases:
    portable(source, expected)
    assert records(invoke(source)) == tokens

# Raw string trigraph contents must not repeat translation phase 1 after #.
s = '#define S(x) #x\nS(R"(??=)")\n'
assert '5222283F3F3D292200' in invoke(s)
# Newlines/comments are normalized for stringizing; literals preserve escaping.
assert 'literal "a b"' in invoke('#define S(x) #x\nS( a /* c */\n b )\n')
assert 'literal "a" "b" array of 3 char 616200' in invoke('"a"\n#define UNUSED 3\n"b"\n')

for source in [
    '#define F(x,x) x\n', '#define F(...) #a\n', '#define F x##\n',
    '#define F 2\n#define F 3\n', '#define F(a,b) a\nF(1)\n',
    '#define F(a) a\nF(1\n', '#define F(a,b) a##b\nF(+,*)\n',
    '#if 1\n', '#else\n', '#if 1/0\n#endif\n', '#error fail\n',
    '#include "missing-file"\n', '_Pragma(2)\n', '@\n', '__VA_ARGS__\n',
]:
    invoke(source, False)

# Includes, once identity, conditional state and macro state are per TU.
source = '#include "header.hh"\n#include "./header.hh"\nM\n'
assert records(invoke(source, companions={'header.hh':'#pragma once\n#define M 9\n5\n'})) == ['literal 5 int 05000000','literal 9 int 09000000']
assert records(invoke(source, companions={'header.hh':'_Pragma("once")\n#define M 9\n5\n'})) == ['literal 5 int 05000000','literal 9 int 09000000']
with tempfile.TemporaryDirectory() as tmp:
    tmp = pathlib.Path(tmp)
    (tmp/'a').write_text('#define M 9\n#include "h"\nM\n')
    (tmp/'b').write_text('#include "h"\nM\n')
    (tmp/'h').write_text('#pragma once\n2\n')
    subprocess.run([str(TOOL),'-o',str(tmp/'out'),'a','b'],cwd=tmp,check=True)
    assert records((tmp/'out').read_text()) == ['literal 2 int 02000000','literal 9 int 09000000','literal 2 int 02000000','identifier M']

# Indexed definitions + persistent unavailable sets: chain sizes must not
# trigger recursion limits or quadratic work. Repeated parameters prescan once.
for count in (1000, 10000, 30000):
    source = '#define F0() done\n'+''.join(f'#define F{i}() F{i-1}()\n' for i in range(1,count+1))
    source += '#define REPEAT(x) x x x x\nREPEAT(F'+str(count)+'())\n'
    assert records(invoke(source)) == ['identifier done']*4
    portable(source, 'done '*4)

# Repeated aliases reuse immutable paint facts instead of allocating per call.
source='#define A B\n#define B C\n#define C done\n'+'A\n'*100000
assert records(invoke(source))==['identifier done']*100000
portable(source,'done '*100000)
# Undef/redefine releases definition storage rather than retaining history.
source=''.join('#define A done\nA\n#undef A\n' for _ in range(20000))
assert records(invoke(source))==['identifier done']*20000
portable(source,'done '*20000)
# Includes without once replay language processing, not copied immutable bytes.
assert records(invoke('#include "h"\n#include "h"\n',companions={'h':'2\n'}))==['literal 2 int 02000000']*2

# Random independent portable DAG expansion, not reference fixture answers.
rng = random.Random(412)
defs = []; values = []
for i in range(700):
    if not i or rng.randrange(3)==0:
        text = str(rng.randrange(100)); value = [text]
    else:
        a,b = rng.randrange(i),rng.randrange(i)
        text = f'N{a} + N{b}'
        value = values[a]+['+']+values[b]
    defs.append(f'#define N{i} {text}\n'); values.append(value)
selected = list(range(600,700))
source = ''.join(defs)+''.join(f'N{i}\n' for i in selected)
expected = [t for i in selected for t in values[i]]
portable(source, ' '.join(expected))
out = records(invoke(source))
observed = [line.split()[1] for line in out]
assert observed == expected
print('PA4 personal behavior/scale controls passed; portable cases qualified with GCC and Clang')
