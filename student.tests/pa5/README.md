# PA5 explicit controls

Run from the checkout root:

```sh
python3 student.tests/pa5/check.py
sources=$(sed -n 's/^FRONTEND_OBJ_BASENAMES_cppgm++ := //p' dev/frontend_source_sets.mk | awk '{for(i=1;i<=NF;i++)printf "dev/src/%s.cpp ",$i}')
g++ -std=c++11 -O2 -Idev/src student.tests/pa5/graph.cpp $sources -o "$RALPH_ARTIFACT_DIR/pa5/graph"
"$RALPH_ARTIFACT_DIR/pa5/graph" student.tests/pa5-portable.cpp
# Also build graph.cpp with Clang ASan/UBSan (-O1 -g -fno-omit-frame-pointer).
python3 student.tests/pa5/measure.py "$RALPH_ARTIFACT_DIR/pa5/final"
```

`check.py` qualifies portable families with both GCC and Clang, compares the
explicit AST view to the reference, and separately checks invalid grammar,
driver rejection and TU resets. One qualified file covers the completed group
without requiring class/template/namespace definitions. A namespace-qualified
expression case is syntax-only: it intentionally references undeclared names.
`graph.cpp` observes the actual graph, not only text: node/edge ownership,
compact name identities, source bounds, captured literal records, member-pointer
and capture syntax, bounded lookahead, stable repeated views. Source file IDs
are zero-based. The grammar permits semantically invalid course cases; personal
portable controls do not confuse those with host-valid C++11.

`measure.py` freezes binary/input hashes and flags, qualifies identical inputs,
checks full AST parity, warms each variant outside measurements, then interleaves
three CPU-pinned user-mode instructions/cycles samples with wall/user/system/RSS.
It reports median/range/IPC and saves raw enabled/running percentages. It includes
10x translation-unit scaling, a richer declaration/expression mix and the largest
currently accepted fixture. Separate branch/cache passes and a sampling profile
investigate the slowest supported large input. Unbounded unary/assignment chains
are additional student-only work/depth controls, not host-parity timing claims.

No compiler executable/backend/LowIR is emitted at PA5. Generated-program
runtime/text/object size and the mandatory PA33/34 ≤1.25x GCC executable
instruction gate are future-stage checks, not waived. Compiler latency, RSS,
compiler binary size and AST view size are reported separately.

`coverage.py` separately qualifies seven major remaining namespace/class/enum/
template/dependent/special-member capability probes with GCC, Clang and the
reference. It records current student failures; those observations are neither
passing controls nor a substitute for implementing the remaining PA5 grammar.
`reference-notes.md` documents two reduced personal oracle disagreements with
C++11 rule proofs and the unchanged bundle revision. No required output changed.
