#!/usr/bin/env python3
"""Fixed portable syntax-only workload; no host headers or later semantics."""
import sys
n = int(sys.argv[1])
print('typedef unsigned long word;')
print('using pointer = const word*;')
for i in range(n):
    print('word f%d(word x, word y) {' % i)
    print('  word z = x * 3 + y;')
    print('  for (word k = 0; k < 8; ++k) { z ^= (x << k) + y; }')
    print('  if (z > x) return static_cast<word>(z); else return (word)(x + y);')
    print('}')
