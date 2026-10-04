#!/usr/bin/env python3
"""Independent whole-stage controls. Explicit only; no fixture/reference rewrites."""
import hashlib
import itertools
import json
import os
import pathlib
import random
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = pathlib.Path(os.environ.get('PA2_TOOL', ROOT/'dev/posttoken'))
REF = ROOT/'reference-binaries/posttoken'
ART = pathlib.Path(os.environ.get('RALPH_ARTIFACT_DIR', tempfile.gettempdir()))/'pa2-audit'
ART.mkdir(parents=True, exist_ok=True)


def run(tool, text):
    p = subprocess.run([tool], input=text.encode(), capture_output=True)
    assert p.returncode == 0, (text[:120], p.stderr)
    return p.stdout.decode()


def record(name, source, expected):
    actual = run(TOOL, source)
    assert actual == expected + 'eof\n', (name, source[:100], actual[:200], expected[:200])
    reference = run(REF, source)
    (ART/(name+'-comparison.json')).write_text(json.dumps({
        'input_sha256': hashlib.sha256(source.encode()).hexdigest(),
        'student_sha256': hashlib.sha256(actual.encode()).hexdigest(),
        'reference_sha256': hashlib.sha256(reference.encode()).hexdigest(),
        'reference_exact': reference == actual,
        'differences': [{'line': i+1, 'student': a, 'reference': b}
                        for i, (a, b) in enumerate(itertools.zip_longest(actual.splitlines(), reference.splitlines()))
                        if a != b]}, indent=2))


# Independent phase-6 oracle: numeric escapes are one code unit; source scalars
# are encoded only after the maximal sequence has selected its encoding.
rng = random.Random(20261004)
encodings = [('', 'char', 1, 'utf-8'), ('u8', 'char', 1, 'utf-8'),
             ('u', 'char16_t', 2, 'utf-16-le'), ('U', 'char32_t', 4, 'utf-32-le'),
             ('L', 'wchar_t', 4, 'utf-32-le')]
source = []; expected = []; portable = ['#include <cstdio>', '#include <cstdint>']
checks = []; encoding_expected = []
for n in range(300):
    prefix, ty, width, encoding = encodings[n % 5]
    chars = ''.join(chr(rng.choice([0x41, 0x7f, 0x80, 0x3c0, 0x20ac, 0x1f600])) for _ in range(rng.randrange(1, 8)))
    numeric = rng.randrange(1 << (width*8))
    suffix = '_π' if n % 7 == 0 else ''
    # Alternating prefix position proves encoding is not chosen from first part.
    part1 = '"\\x' + format(numeric, 'x') + '"'
    part2 = prefix + '"' + chars + '"'
    if n % 2:
        part1 = prefix + part1; part2 = '"' + chars + '"'
    literal = part1 + suffix + ' /* separation */ ' + part2 + suffix
    rendered = part1 + suffix + ' ' + part2 + suffix
    data = numeric.to_bytes(width, 'little') + chars.encode(encoding) + bytes(width)
    source.append(literal + ';')
    encoding_expected.append(data.hex().upper())
    head = ('user-defined-literal ' + rendered + ' ' + suffix + ' string ' if suffix else 'literal ' + rendered + ' ')
    expected.append(head + 'array of ' + str(len(data)//width) + ' ' + ty + ' ' + data.hex().upper() + '\nsimple ; OP_SEMICOLON\n')
    # Hosts qualify the same literal payload without operator-suffix invocation.
    portable.append('const ' + ty + ' s' + str(n) + '[] = ' + part1 + ' ' + part2 + ';')
    checks.append('for (unsigned char c : *reinterpret_cast<const unsigned char (*)[sizeof(s'+str(n)+')]>(s'+str(n)+')) std::printf("%02X", c); std::puts("");')
record('encoding-matrix', '\n'.join(source), ''.join(expected))
portable.append('int main() { ' + ' '.join(checks) + ' }')

# Same-source, finite, exactly representable floats: stream compatibility does
# not replace checking typed values against actual C++11 host literal values.
float_source = []; float_expected = []; float_decls = []; float_checks = []
for n in range(150):
    value = rng.randrange(1, 1000000) / (1 << rng.randrange(0, 6))
    suffix, ty, fmt = [('f', 'float', '<f'), ('', 'double', '<d')][n % 2]
    text = format(value, '.8f') + suffix
    data = struct.pack(fmt, value)
    float_source.append(text)
    float_expected.append('literal ' + text + ' ' + ty + ' ' + data.hex().upper() + '\n')
    float_decls.append(ty + ' f' + str(n) + ' = ' + text + ';')
    float_checks.append('for (unsigned char c : *reinterpret_cast<const unsigned char (*)[sizeof(f'+str(n)+')]>(&f'+str(n)+')) std::printf("%02X", c); std::puts("");')
record('floating-matrix', ' '.join(float_source), ''.join(float_expected))

# Prefix/suffix conflicts, all encodings, recovery, raw-vs-ordinary provenance.
source = []; expected = []
for left, right in itertools.product(encodings[1:], repeat=2):
    if left[0] == right[0]:
        continue
    literal = left[0]+'"a" '+right[0]+'"b"'
    source.append(literal+'; 3')
    expected.append('invalid '+literal+'\nsimple ; OP_SEMICOLON\nliteral 3 int 03000000\n')
for prefix, ty, width, encoding in encodings:
    literal = prefix+'R"d(\\n\\u03c0??=)d" '+prefix+'"\\n"'
    data = ('\\n\\u03c0??='+'\n').encode(encoding)+bytes(width)
    source.append(literal+';')
    expected.append('literal '+literal+' array of '+str(len(data)//width)+' '+ty+' '+data.hex().upper()+'\nsimple ; OP_SEMICOLON\n')
record('conflicts-raw-recovery', '\n'.join(source), ''.join(expected))

# Raw numeric UDLs must not inherit builtin 64-bit/float scanning limits.
huge = '9'*8192
record('unbounded-ud', huge+'_π 1.'+huge+'_π 0x'+('f'*8192)+'_π',
       'user-defined-literal '+huge+'_π _π integer '+huge+'\n'
       +'user-defined-literal 1.'+huge+'_π _π floating 1.'+huge+'\n'
       +'user-defined-literal 0x'+('f'*8192)+'_π _π integer 0x'+('f'*8192)+'\n')

# Concrete declaration/template lexical trace; no claim of PA2 semantics/ELF.
trace = 'template<class T> struct box { static constexpr char16_t value = u\'π\'; }; const char16_t s[] = "\\x3c0" u"😀"; box<int> b;'
assert 'literal u\'π\' char16_t C003' in run(TOOL, trace)
assert run(TOOL, trace) == run(REF, trace)
(ART/'declaration-template-trace.cpp').write_text(trace)

with tempfile.TemporaryDirectory(prefix='pa2-independent-') as tmp:
    tmp = pathlib.Path(tmp)
    float_program = '#include <cstdio>\n'+'\n'.join(float_decls)+'\nint main() { '+' '.join(float_checks)+' }'
    floats_expected = [line.split()[-1] for line in float_expected]
    for host in ('g++', 'clang++'):
        for name, program, expected_bytes in [('encodings', '\n'.join(portable), encoding_expected),
                                              ('floats', float_program, floats_expected)]:
            path = tmp/(name+'.cpp'); path.write_text(program)
            exe = tmp/(host+'-'+name)
            subprocess.run([host, '-std=c++11', '-O2', path, '-o', exe], check=True)
            output = subprocess.check_output([exe], text=True).splitlines()
            assert output == expected_bytes, (host, name)
        subprocess.run([host, '-std=c++11', '-fsyntax-only', ART/'declaration-template-trace.cpp'], check=True)
print('PA2 independent audit controls passed: 300 encoding/UD families, 150 floats, 12 prefix conflicts, 5 raw/provenance cases, 3 unbounded UD literals; GCC/Clang payload execution and declaration/template qualification')
