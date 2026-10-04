#!/usr/bin/env python3
"""Explicit PA1 controls; no course fixture changes or reference dependence."""
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / 'dev/pptoken'


def render(tokens):
    result = b''
    for kind, spelling in tokens:
        data = spelling.encode() if isinstance(spelling, str) else spelling
        if kind == 'eof':
            result += b'eof\n'
        else:
            result += kind.encode() + b' ' + str(len(data)).encode() + b' ' + data + b'\n'
    return result


NL = ('new-line', '')
WS = ('whitespace-sequence', '')
EOF = ('eof', '')
I = lambda s: ('identifier', s)
P = lambda s: ('preprocessing-op-or-punc', s)
S = lambda s: ('string-literal', s)
O = lambda s: ('non-whitespace-character', s)
N = lambda s: ('pp-number', s)

cases = [
    (b'', [EOF]),
    (b'\xef\xbb\xbf', [EOF]),
    (b'a', [I('a'), NL, EOF]),
    (b'a\\\n', [I('a'), NL, EOF]),
    (b'\\\n', [NL, EOF]),
    (b'\n\\\n', [NL, NL, EOF]),
    (b'??/\n', [NL, EOF]),
    (b'foo\\\nbar', [I('foobar'), NL, EOF]),
    (b'?\xcf\x80', [P('?'), I('π'), NL, EOF]),
    (b'???=????=', [P('?'), P('#'), P('?'), P('?'), P('#'), NL, EOF]),
    (b'??/u03C0', [I('π'), NL, EOF]),
    (b'\\u0040', [O('@'), NL, EOF]),
    (b'\\u{bad}', [O('\\'), I('u'), P('{'), I('bad'), P('}'), NL, EOF]),
    (b'"\\\\u{"', [S('"\\\\u{"'), NL, EOF]),
    ('\u0300a\u0300'.encode(), [O('\u0300'), I('a\u0300'), NL, EOF]),
    ('\u00a7\u00a8\U000efffd\U000efffe'.encode(),
     [O('\u00a7'), I('\u00a8\U000efffd'), O('\U000efffe'), NL, EOF]),
    (b'8E0e4x0e-7968ecf 1p+3', [N('8E0e4x0e-7968ecf'), WS, N('1p'), P('+'), N('3'), NL, EOF]),
    (b'..42 ... .1 .*', [P('.'), N('.42'), WS, P('...'), WS, N('.1'), WS, P('.*'), NL, EOF]),
    (b'<::x <::: <::>', [P('<'), P('::'), I('x'), WS, P('<:'), P('::'), WS, P('<:'), P(':>'), NL, EOF]),
    (b'/*a\nb*/x //c\\\ncontinued', [WS, I('x'), WS, NL, EOF]),
    (b'/**//**/ \t//x', [WS, NL, EOF]),
    (b'%: /*x*/ include\\\n <a/*b*/c>', [P('%:'), WS, I('include'), WS, ('header-name', '<a/*b*/c>'), NL, EOF]),
    (b'#define include "x"', [P('#'), I('define'), WS, I('include'), WS, S('"x"'), NL, EOF]),
    (b'#include X "a"', [P('#'), I('include'), WS, I('X'), WS, S('"a"'), NL, EOF]),
    (b'x #include <a>', [I('x'), WS, P('#'), I('include'), WS, P('<'), I('a'), P('>'), NL, EOF]),
    (b'#include "a\\b"', [P('#'), I('include'), WS, ('header-name', '"a\\b"'), NL, EOF]),
    (b'%:%:include <a>', [P('%:%:'), I('include'), WS, P('<'), I('a'), P('>'), NL, EOF]),
    (b'R"(??=\\u03C0\\\n)"', [S('R"(??=\\u03C0\\\n)"'), NL, EOF]),
    (b'u8R"??=(x)??="_tag', [('user-defined-string-literal', 'u8R"??=(x)??="_tag'), NL, EOF]),
    (b'R"(\\UFFFFFFFF)"', [S('R"(\\UFFFFFFFF)"'), NL, EOF]),
    (b'R"aa(x)aaa"y)aa"', [S('R"aa(x)aaa"y)aa"'), NL, EOF]),
    (b'u8\'x\'_t', [I('u8'), ('user-defined-character-literal', "'x'_t"), NL, EOF]),
    (b'"a"R"(b)"R', [('user-defined-string-literal', '"a"R'), ('user-defined-string-literal', '"(b)"R'), NL, EOF]),
    (b"'\\1234'", [('character-literal', "'\\1234'"), NL, EOF]),
    (b'"\\x123z"', [S('"\\x123z"'), NL, EOF]),
    (b'"\\u03C0"', [S('"π"'), NL, EOF]),
    (b'and andromeda delete deleted new newer', [P('and'), WS, I('andromeda'), WS,
        P('delete'), WS, I('deleted'), WS, P('new'), WS, I('newer'), NL, EOF]),
    (b'\x00', [O(b'\x00'), NL, EOF]),
]

rejections = [b'/*', b'"', b"''", b"'x\n'", b'"\\8"', b'"\\xZ"',
              b'R"12345678901234567()12345678901234567"', b'R"a b(x)a b"',
              b'R"(x)', b'#include <x', b'#include ""',
              b'\\U00110000', b'\\uD800', b'\\UFFFFFFFF',
              b'\xc0\x80', b'\xe0\x80\x80', b'\xed\xa0\x80', b'\xf4\x90\x80\x80',
              b'\xe2\x82', b'\x80', b'\xff', b'R"(\xff)"', b'"\\\xc4\xa1"', b'"\\\xc5\xa1"']

for n, (source, expected) in enumerate(cases):
    result = subprocess.run([TOOL], input=source, capture_output=True)
    assert result.returncode == 0, (n, source, result.stderr)
    assert result.stdout == render(expected), (n, source, result.stdout, render(expected))
for source in rejections:
    result = subprocess.run([TOOL], input=source, capture_output=True)
    assert result.returncode == 1, (source, result.returncode, result.stdout)

# Qualify portable phase interactions with both hosts. Semantic compilation is
# validation only: neither host is involved in implementing student output.
with tempfile.TemporaryDirectory() as tmp:
    source = pathlib.Path(tmp) / 'portable.cpp'
    source.write_text('''// line-comment splice removes the next line\\
not tokens
namespace n { using size_t = unsigned; }
template<class T> struct box { static const int value = 7; };
int π = box<::n::size_t>::value;
const char* raw = u8R"??=(??/ \\u03C0
)??=";
const char* escaped = "\\\\u{";
int tri = 1 ??! 2;
int spl\\
ice = 0;
int main() { return π + splice + tri + raw[0] + escaped[0]; }
''')
    for host in ('g++', 'clang++'):
        subprocess.run([host, '-std=c++11', '-trigraphs', '-fsyntax-only', str(source)], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    result = subprocess.run([TOOL], input=source.read_bytes(), capture_output=True)
    assert result.returncode == 0, result.stderr
print(f'PA1 personal controls passed: {len(cases)} exact cases, {len(rejections)} rejections; GCC/Clang portable qualification')
