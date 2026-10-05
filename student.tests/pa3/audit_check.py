#!/usr/bin/env python3
"""Independent whole-stage oracle: typed trees, all operators, host/ref boundary."""
import json
import os
from pathlib import Path
import random
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TOOL = os.environ.get('PA3_TOOL', str(ROOT/'dev/ppexpr'))
A = Path(os.environ.get('RALPH_ARTIFACT_DIR', tempfile.gettempdir()))/'pa3-audit'
A.mkdir(parents=True, exist_ok=True)
MASK = (1 << 64)-1
SIGN = 1 << 63
rng = random.Random(3303006)
OPS = ('+', '-', '*', '/', '%', '<<', '>>', '<', '>', '<=', '>=', '==', '!=', '&', '^', '|', '&&', '||')


def signed(x):
    return x-(1 << 64) if x & SIGN else x


def value(node):
    """Independently compute static type before lazy evaluation; wrap storage."""
    if node[0] == 'leaf':
        return node[1], node[2], True
    op = node[0]
    if op == '?:':
        x, _, legal_x = value(node[1])
        y, yu, legal_y = value(node[2])
        z, zu, legal_z = value(node[3])
        return (y if x else z), yu or zu, legal_x and (legal_y if x else legal_z)
    x, xu, legal_x = value(node[1])
    if len(node) == 2:
        y = {'!': int(not x), '~': ~x, 'neg': -x, 'pos': x}[op]
        legal = legal_x and (op != 'neg' or xu or x != SIGN)
        return y & MASK, False if op == '!' else xu, legal
    y, yu, legal_y = value(node[2])
    u = xu or yu
    xx, yy = (x, y) if u else (signed(x), signed(y))
    legal = legal_x and legal_y
    if op == '&&':
        return int(bool(x) and bool(y)), False, legal_x and (legal_y if x else True)
    if op == '||':
        return int(bool(x) or bool(y)), False, legal_x and (True if x else legal_y)
    if op in ('<<', '>>'):
        if y >= 64:
            return 0, xu, False
        xx = x if xu else signed(x)
        z = xx << y if op == '<<' else xx >> y
        if op == '<<' and not xu:
            legal &= xx >= 0 and z < SIGN
        return z & MASK, xu, legal
    if op in ('/', '%'):
        if not yy or (not u and xx == -SIGN and yy == -1):
            return 0, u, False
        q = abs(xx)//abs(yy) * (-1 if (xx < 0) != (yy < 0) else 1)
        return (q if op == '/' else xx-q*yy) & MASK, u, legal
    if op in ('<', '>', '<=', '>=', '==', '!='):
        z = {'<': xx < yy, '>': xx > yy, '<=': xx <= yy, '>=': xx >= yy,
             '==': xx == yy, '!=': xx != yy}[op]
        return int(z), False, legal
    z = {'+': lambda: xx+yy, '-': lambda: xx-yy, '*': lambda: xx*yy,
         '&': lambda: x & y, '^': lambda: x ^ y, '|': lambda: x | y}[op]()
    if not u and op in ('+', '-', '*'):
        legal &= -SIGN <= z < SIGN
    return z & MASK, u, legal


def text(n):
    if n[0] == 'leaf':
        return str(n[1])+('ull' if n[2] else 'll')
    if n[0] == '?:':
        return '('+text(n[1])+' ? '+text(n[2])+' : '+text(n[3])+')'
    if len(n) == 2:
        return '('+{'neg': '-', 'pos': '+'}.get(n[0], n[0])+' '+text(n[1])+')'
    return '('+text(n[1])+' '+n[0]+' '+text(n[2])+')'


def tree(depth):
    if not depth or rng.random() < .23:
        return ('leaf', rng.choice((0, 1, 2, 7, 31, 63, 64, 255, 997)), bool(rng.randrange(2)))
    op = rng.choice(OPS+('?:', 'neg', 'pos', '!', '~'))
    if op == '?:':
        return (op, tree(depth-1), tree(depth-1), tree(depth-1))
    return (op, tree(depth-1)) if op in ('neg', 'pos', '!', '~') else (op, tree(depth-1), tree(depth-1))


# Avoid relying on host behavior for undefined arithmetic. Oracle UB rows are
# not used as correctness evidence. Course-defined shift/division errors and
# full unsigned boundaries are explicitly asserted separately below.
rows = []
while len(rows) < 6000:
    n = tree(5)
    bits, u, legal = value(n)
    if legal:
        rows.append((text(n), str(bits if u else signed(bits))+('u' if u else '')))
for op in OPS:
    for a in (0, 1, 7, 63):
        for b in (1, 2, 3):
            for u in (False, True):
                n = (op, ('leaf', a, u), ('leaf', b, not u))
                bits, result_u, legal = value(n)
                assert legal
                rows.append((text(n), str(bits if result_u else signed(bits))+('u' if result_u else '')))
source = ''.join(e+'\n' for e, _ in rows)
expected = ''.join(v+'\n' for _, v in rows)+'eof\n'
p = subprocess.run([TOOL], input=source, text=True, capture_output=True, check=True)
assert p.stdout == expected, next((row, got) for row, got in zip(rows, p.stdout.splitlines()) if row[1] != got)
(A/'oracle.expr').write_text(source)
(A/'oracle.expected').write_text(expected)

edge = [
    ('18446744073709551615ull + 1ull', '0u'),
    ('0x80000000', '2147483648u'), ('0x8000000000000000', '9223372036854775808u'),
    ('9223372036854775807ll', '9223372036854775807'),
    ('9223372036854775808ll', 'error'), ('18446744073709551616ull', 'error'),
    ('0 ? 1/0u : -1', '18446744073709551615u'),
    ('1 ? -1 : 0u', '18446744073709551615u'),
    ('1 ? -1 : 0u >> 2', '18446744073709551615u'),
    ('(1 ? -1 : 0u) < 0', '0'), ('-1 >> 0u', '-1'),
    ('0 && 1/0', '0'), ('1 || 1/0', '1'), ('1/0', 'error'),
    ('1 % 0', 'error'), ('1 << -1', 'error'), ('1 >> 64', 'error'),
    ('1 ? 4 : 3.5', 'error'), ('0 && 1_xyz', 'error'),
    ('1 || (2 +)', 'error'), ('0 ? 2 : (3,4)', 'error'),
    ('7\n8', '7\n8'), ('1 ? (2 ? 3 : 4) : 5', '3'),
    ('(1 ? 2) : 3', 'error'), ('1 ? 2 : 3 ? 4 : 5', '2'),
    ('1 - 2 - 3', '-4'), ('8 / 4 / 2', '1'), ('1 << 2 + 1', '8'),
    ('1 < 2 == 1', '1'), ('1 | 2 ^ 3 & 1', '3'),
    ('defined(é) + defined(π) + sizeof + class', '2'),
    ("u'\\uffff' + -1", '65534u'), ("U'\\U0001f600'", '128512u'),
    ("L'\\uffff'", '65535'), ("'\\x80'", '128'),
    ('1/*ignored\nphysical newline*/+2', '3'), ('4??/\n+5', '9'),
    ('1 || R"x(raw\nline)x"\n8', 'error\n8'),
    ('1u\n\n2u // comment', '1u\n2u'),
]
r = subprocess.run([TOOL], input='\n'.join(e for e, _ in edge)+'\n', text=True, capture_output=True, check=True)
assert r.stdout == ''.join(v+'\n' for _, v in edge)+'eof\n', r.stdout
for e in ('1 +\n"\\q"', '0 && "unterminated\n', '1 ?\n/* unterminated'):
    assert subprocess.run([TOOL], input=e, text=True, capture_output=True).returncode != 0

# Equivalent expression text, not a host-generated answer used by the student.
with tempfile.TemporaryDirectory() as temp:
    pp = Path(temp)/'oracle.cpp'
    pp.write_text(''.join(f'#if ({e}) != {v[:-1]+"ull" if v.endswith("u") else v+"ll"}\n#error oracle_{i}\n#endif\n' for i, (e, v) in enumerate(rows)))
    for host in ('g++', 'clang++'):
        result = subprocess.run([host, '-std=c++11', '-E', '-P', str(pp)],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (A/(host+'-oracle.stderr')).write_bytes(result.stderr)
        assert result.returncode == 0, result.stderr[:2000]

ref = subprocess.run([ROOT/'pa3/ppexpr-ref'], input=source, text=True, capture_output=True, check=True)
assert ref.stdout == expected, 'reference mismatch on qualified rows'
(A/'oracle-reference.json').write_text(json.dumps({'rows': len(rows), 'seed': 3303006,
    'gcc': 'pass', 'clang': 'pass', 'reference': 'pass', 'edges': len(edge)}, indent=2))
print(f'PA3 independent audit PASS: {len(rows)} typed oracle/GCC/Clang/reference rows; {len(edge)} boundary/interactions')
