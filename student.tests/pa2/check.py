#!/usr/bin/env python3
"""PA2 behavior/property controls and GCC/Clang C++11 qualification, explicit only."""
import os
import pathlib
import random
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = pathlib.Path(os.environ.get('PA2_TOOL', ROOT / 'dev/posttoken'))

def run(source):
    result = subprocess.run([TOOL], input=source.encode(), capture_output=True)
    assert result.returncode == 0, (source[:100], result.stderr)
    return result.stdout.decode()

def check(source, expected):
    actual = run(source)
    assert actual == expected + 'eof\n', (source[:150], actual[:300], expected[:300])

def hexbytes(value, width):
    return value.to_bytes(width, 'little').hex().upper()

cases = [
    ('# ## %: %:%: @', 'invalid #\ninvalid ##\ninvalid %:\ninvalid %:%:\ninvalid @\n'),
    ('"a" \'\' "b";', 'literal "a" array of 2 char 6100\ninvalid \'\'\nliteral "b" array of 2 char 6200\nsimple ; OP_SEMICOLON\n'),
    ('"a"_π /*x*/ "b"_π;', 'user-defined-literal "a"_π "b"_π _π string array of 3 char 616200\nsimple ; OP_SEMICOLON\n'),
    ('"a"_x u"b" "c"_x;', 'user-defined-literal "a"_x u"b" "c"_x _x string array of 4 char16_t 6100620063000000\nsimple ; OP_SEMICOLON\n'),
    ('u8"a" u"b" U"c";', 'invalid u8"a" u"b" U"c"\nsimple ; OP_SEMICOLON\n'),
    ('"a"_x "b"_y;', 'invalid "a"_x "b"_y\nsimple ; OP_SEMICOLON\n'),
    ('"\\x100" "b";', 'invalid "\\x100" "b"\nsimple ; OP_SEMICOLON\n'),
    ('"\\x3c0" u"";', 'literal "\\x3c0" u"" array of 2 char16_t C0030000\nsimple ; OP_SEMICOLON\n'),
    ('u"\\xD800";', 'literal u"\\xD800" array of 2 char16_t 00D80000\nsimple ; OP_SEMICOLON\n'),
    ('U"\\xFFFFFFFF";', 'literal U"\\xFFFFFFFF" array of 2 char32_t FFFFFFFF00000000\nsimple ; OP_SEMICOLON\n'),
    ('U"\\x100000000000000000000000000";', 'invalid U"\\x100000000000000000000000000"\nsimple ; OP_SEMICOLON\n'),
    ('"\\7777" u"";', 'literal "\\7777" u"" array of 3 char16_t FF0137000000\nsimple ; OP_SEMICOLON\n'),
    ('"\\u0022\\u005c";', 'literal ""\\" array of 3 char 225C00\nsimple ; OP_SEMICOLON\n'),
    ('"\\u005c\\u006e";', 'literal "\\n" array of 3 char 5C6E00\nsimple ; OP_SEMICOLON\n'),
    ('"\\x4\\u0031";', 'literal "\\x41" array of 3 char 043100\nsimple ; OP_SEMICOLON\n'),
    ('"\\\\u03C0";', 'literal "\\\\u03C0" array of 7 char 5C753033433000\nsimple ; OP_SEMICOLON\n'),
    ('u"😀";', 'literal u"😀" array of 3 char16_t 3DD800DE0000\nsimple ; OP_SEMICOLON\n'),
    ('R"d(\\u03c0\\n??=)d";', 'literal R"d(\\u03c0\\n??=)d" array of 12 char 5C75303363305C6E3F3F3D00\nsimple ; OP_SEMICOLON\n'),
    ("'π' u'😀' U'😀' '\\xD800' '\\x110000' '\\x100000000' 'ab'",
     "literal 'π' int C0030000\ninvalid u'😀'\nliteral U'😀' char32_t 00F60100\ninvalid '\\xD800'\ninvalid '\\x110000'\ninvalid '\\x100000000'\ninvalid 'ab'\n"),
    ('operator""sv; operator""_x;', 'simple operator KW_OPERATOR\nliteral "" array of 1 char 00\nidentifier sv\nsimple ; OP_SEMICOLON\nsimple operator KW_OPERATOR\nliteral "" array of 1 char 00\nidentifier _x\nsimple ; OP_SEMICOLON\n'),
    ('operator u""sv; operator R"()"sv;', 'simple operator KW_OPERATOR\ninvalid u""sv\nsimple ; OP_SEMICOLON\nsimple operator KW_OPERATOR\ninvalid R"()"sv\nsimple ; OP_SEMICOLON\n'),
    ('1.0_f+', 'user-defined-literal 1.0_f _f floating 1.0\nsimple + OP_PLUS\n'),
    ('0_lπ 1.0_fπ', 'user-defined-literal 0_lπ _lπ integer 0\nuser-defined-literal 1.0_fπ _fπ floating 1.0\n'),
    ('0x1e_foo 1_e3 1.0_e3 1e3_foo', 'user-defined-literal 0x1e_foo _foo integer 0x1e\nuser-defined-literal 1_e3 _e3 integer 1\nuser-defined-literal 1.0_e3 _e3 floating 1.0\nuser-defined-literal 1e3_foo _foo floating 1e3\n'),
]
for source, expected in cases:
    check(source, expected)
invalids = ['09', '09_x', '0x', '1e', '1.e', '1e+', '1e-', '1.0e_x', '1..', '1.._x', '1e+_x',
            '0X_x', '0xG', '1_f', # last is a valid UD integer, removed below
            '1f', '1LLuU', '1lL', '1u_x', '0x1p2', '1.0ff', '1.2_x.y']
invalids.remove('1_f')
check(' '.join(invalids), ''.join('invalid ' + x + '\n' for x in invalids))
check('9' * 300 + '_x 0x' + 'F' * 300 + '_x',
      'user-defined-literal ' + '9'*300 + '_x _x integer ' + '9'*300 + '\n' +
      'user-defined-literal 0x' + 'F'*300 + '_x _x integer 0x' + 'F'*300 + '\n')

# Independent arithmetic oracle for all builtin integer candidate tables.
rng = random.Random(20261004)
values = [0, 1, 127, 255, 32767, 65535, 2**31-1, 2**31, 2**32-1, 2**32, 2**63-1, 2**63, 2**64-1, 2**64]
values += [rng.randrange(2**64) for _ in range(35)]
suffixes = ['', 'u', 'U', 'l', 'L', 'll', 'LL', 'uL', 'Lu', 'ull', 'LLU']
count = 0
for base in [8, 10, 16]:
    for value in values:
        prefix = str(value) if base == 10 else ('0' + format(value, 'o') if base == 8 else '0x' + format(value, 'X'))
        for suffix in suffixes:
            source = prefix + suffix
            lower = suffix.lower()
            unsigned = 'u' in lower
            longs = lower.count('l')
            candidates = []
            if longs == 0:
                if not unsigned: candidates.append(('int', 4, 2**31-1))
                if unsigned or base != 10: candidates.append(('unsigned int', 4, 2**32-1))
            if longs < 2:
                if not unsigned: candidates.append(('long int', 8, 2**63-1))
                if unsigned or base != 10: candidates.append(('unsigned long int', 8, 2**64-1))
            if not unsigned: candidates.append(('long long int', 8, 2**63-1))
            if unsigned or base != 10: candidates.append(('unsigned long long int', 8, 2**64-1))
            candidate = next((c for c in candidates if value <= c[2]), None)
            expected = ('literal ' + source + ' ' + candidate[0] + ' ' + hexbytes(value, candidate[1]) + '\n'
                        if candidate else 'invalid ' + source + '\n')
            check(source, expected); count += 1

# Maximal sequence survives many chunks, late encoding, invalidity and recovery.
chunks = 20000
source = ' '.join(['"a"']*chunks) + ' u"😀";'
check(source, 'literal ' + source[:-1] + ' array of ' + str(chunks+3) + ' char16_t ' + '6100'*chunks + '3DD800DE0000\nsimple ; OP_SEMICOLON\n')
source = ' '.join(['u"a"']*chunks) + ' U"b"; 2'
check(source, 'invalid ' + source[:source.index(';')] + '\nsimple ; OP_SEMICOLON\nliteral 2 int 02000000\n')
for source in ['"', '"\\x"', '"\\8"', '/*', "'a\n'", '"\\uD800"']:
    result = subprocess.run([TOOL], input=source.encode(), capture_output=True)
    assert result.returncode != 0, source

# Portable cases: both hosts check C++11 type/value and string encodings; no
# agreement-based oracle for course-defined non-ASCII char or rejection rules.
with tempfile.TemporaryDirectory(prefix='pa2-controls-') as tmp:
    tmp = pathlib.Path(tmp)
    portable = r'''
#include <type_traits>
#include <cstring>
static_assert(std::is_same<decltype(2147483648), long>::value, "decimal promotion");
static_assert(std::is_same<decltype(0xffffffff), unsigned int>::value, "hex promotion");
static_assert(std::is_same<decltype(1LL), long long>::value, "long long");
static_assert(0xffffffffffffffffULL == 18446744073709551615ULL, "uint64 limit");
constexpr char16_t s[] = "\x3c0" u"😀";
static_assert(sizeof(s) == 8 && s[0] == 0x3c0 && s[1] == 0xd83d && s[2] == 0xde00, "concat encoding");
constexpr char q[] = "\u0022\u005c";
static_assert(q[0] == '"' && q[1] == '\\' && sizeof(q) == 3, "UCN provenance");
constexpr char16_t numeric[] = u"\xD800";
static_assert(numeric[0] == 0xd800, "numeric code unit, not scalar");
constexpr char raw[] = R"x(\u03c0??=)x";
static_assert(sizeof(raw) == 10 && raw[0] == '\\', "raw reversal");
constexpr long double operator""_probe(long double x) { return x; }
int main() { return 1.25_probe != 1.25L || 1.25f != 1.25; }
'''
    path = tmp / 'portable.cpp'; path.write_text(portable)
    for host in ['g++', 'clang++']:
        exe = tmp / host
        subprocess.run([host, '-std=c++11', '-trigraphs', '-O2', path, '-o', exe], check=True)
        subprocess.run([exe], check=True)
    sources = list((ROOT / 'dev/src/preprocess/lex').glob('*.cpp'))
    sources += [p for p in (ROOT / 'dev/src/preprocess/post').glob('*.cpp') if p.name != 'render.cpp']
    cursor = tmp / 'cursor'
    subprocess.run(['g++', '-std=c++11', '-O2', '-I'+str(ROOT/'dev/src'), ROOT/'student.tests/pa2/cursor.cpp', *sources, '-o', cursor], check=True)
    subprocess.run([cursor], check=True)
print(f'PA2 personal controls passed: {len(cases)} exact families, {count} integer table cases, maximal sequences, rejections; GCC/Clang qualification')
