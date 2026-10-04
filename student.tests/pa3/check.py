#!/usr/bin/env python3
"""Explicit PA3 semantic, scaling, and GCC/Clang qualification controls."""
import os
import pathlib
import random
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = pathlib.Path(os.environ.get('PA3_TOOL', ROOT/'dev/ppexpr'))

def run(source):
    return subprocess.run([TOOL], input=source.encode(), capture_output=True)

def check(source, expected):
    r = run(source)
    assert r.returncode == 0, (source[:100], r.stderr)
    assert r.stdout.decode() == expected+'eof\n', (source[:100], r.stdout[:400], expected[:400])

cases = [
    ('1 + 2 * 3 << 2 == 28', '1'),
    ('1 ? 0 ? 2 : 3 : 4', '3'), ('0 ? 1 : 0 ? 2 : 3', '3'),
    ('1 ? 0 ? 2u : -3 : 4', '18446744073709551613u'),
    ('0 ? 1/0u : -2', '18446744073709551614u'),
    ('(1 ? -2 : 3u) >> 1', '9223372036854775807u'),
    ('-3 >> 1u', '-2'), ('1u << -1', 'error'), ('1 >> 64u', 'error'),
    ('0 && (1 << 64)', '0'), ('1 || ((1 << 63) / -1)', '1'),
    ('0 ? (1 << 63) % -1 : 9', '9'),
    ('0 ? 1.0 : 2', 'error'), ('1 || 3_z', 'error'),
    ('0 && "bad"', 'error'), ('1 ? 2 : (3+)', 'error'),
    ('1 ? 2 : (4,5)', 'error'), ('1?2:3:4', 'error'),
    ('(1?2):3', 'error'), ('1?2', 'error'), ('1 2', 'error'),
    ('()', 'error'), ('1+=2', 'error'), ('++1', 'error'), ('1--2', 'error'),
    ('defined defined', '0'), ('defined(true)', '0'), ('defined (class)', '1'),
    ('defined (and)', 'error'), ('defined 0', 'error'), ('defined ()', 'error'),
    ('defined (a+b)', 'error'), ('defined (a', 'error'), ('defined a b', 'error'),
    ('true + false + nullptr + sizeof + char16_t', '1'),
    ('not false and true or false', '1'), ('compl 0u', '18446744073709551615u'),
    ("u'\\uffff' + -1", '65534u'), ("L'\\xffffffff'", 'error'),
    ("'\\x80'", '128'), ("u'a'", '97u'), ("L'a'", '97'),
    ('defined é + defined π', '2'), ('', None),
]
check('\n'.join(s for s, _ in cases)+'\n', ''.join(v+'\n' for _, v in cases if v is not None))
check('/* x\ny */ 2\n3\\\n+4\n??-0\n//empty\n8', '2\n7\n-1\n8\n')
check('R"x(a\nb)x"\n6\n', 'error\n6\n')
check('5uu 7\n5\ndefined (a\n9\n', 'error\n5\nerror\n9\n')
for source in ('1\n/* unclosed', '0 && "unterminated\n', '2\n"\\q"\n'):
    assert run(source).returncode != 0, source

# Explicit-stack parsing/evaluation must not consume one native frame per node.
depth = 100000
check('('*depth+'1'+')'*depth+'\n', '1\n')
check('! '*depth+'1\n', '1\n')
check('+'.join(['1']*depth)+'\n', str(depth)+'\n')
check('1?'*depth+'7'+':9'*depth+'\n', '7\n')
check('0?9:'*depth+'7\n', '7\n')

# Freeze portable arithmetic cases. Native C++ and #if use the same expressions;
# signed ranks/char16 promotion differences are covered above, not conflated
# with portable same-semantics cases. GCC and Clang both must accept assertions.
rng = random.Random(3003)
portable = []
for i in range(1200):
    a,b,c,d = [rng.randrange(1, 1000) for _ in range(4)]
    mode = i % 6
    if mode == 0: expr, value = f'{a}*{b}+{c}-{d}', a*b+c-d
    elif mode == 1: expr, value = f'({a}u-{b}u)*{c}u', ((a-b)*c) % (1<<64)
    elif mode == 2: expr, value = f'({a} < {b}) ? {c} : {d}', c if a<b else d
    elif mode == 3: expr, value = f'({a}%{b}) ^ ({c} & {d})', (a%b) ^ (c&d)
    elif mode == 4: expr, value = f'({a} << 3) >> 2', a*2
    else: expr, value = f'({a} > {b}) || ({c} == {d})', int(a>b or c==d)
    # PP arithmetic uses 64 bits while ordinary unsigned int uses 32. Force ULL
    # in both tools for native-C++ qualification of wrapping unsigned cases.
    if mode == 1: expr = expr.replace('u', 'ull')
    portable.append((expr, value, mode == 1))
check('\n'.join(e for e,_,_ in portable)+'\n', ''.join(str(v)+('u' if u else '')+'\n' for _,v,u in portable))
with tempfile.TemporaryDirectory() as temp:
    path = pathlib.Path(temp)/'qualification.cpp'
    path.write_text(''.join(f'static_assert(({e}) == {v}{"ull" if u else "ll"}, "case {i}");\n'
                            for i,(e,v,u) in enumerate(portable)))
    pp = pathlib.Path(temp)/'qualification-if.cpp'
    pp.write_text(''.join(f'#if ({e}) != {v}{"ull" if u else "ll"}\n#error case_{i}\n#endif\n'
                          for i,(e,v,u) in enumerate(portable)))
    for compiler in ('g++', 'clang++'):
        subprocess.run([compiler, '-std=c++11', '-fsyntax-only', str(path)], check=True)
        subprocess.run([compiler, '-std=c++11', '-E', '-P', str(pp)], check=True, stdout=subprocess.DEVNULL)
print('PA3 personal controls PASS: semantics, 100k-depth/chains, 1200 GCC/Clang-qualified cases')
