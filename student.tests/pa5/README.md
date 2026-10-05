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

`coverage.py` separately qualifies the inherited seven major remaining namespace/class/enum/
template/dependent/special-member capability probes with GCC, Clang and the
reference. It records current student failures; those observations are neither
passing controls nor a substitute for implementing the remaining PA5 grammar.
`reference-notes.md` documents reduced personal oracle disagreements with
C++11 rule proofs and the unchanged bundle revision. No required output changed.

## Namespace / enum extension (loop 10)

```sh
python3 student.tests/pa5/scopes.py
sources=$(sed -n 's/^FRONTEND_OBJ_BASENAMES_cppgm++ := //p' dev/frontend_source_sets.mk | awk '{for(i=1;i<=NF;i++)printf "dev/src/%s.cpp ",$i}')
g++ -std=c++11 -O2 -Idev/src student.tests/pa5/scopes.cpp $sources -o "$RALPH_ARTIFACT_DIR/pa5/scopes-graph"
"$RALPH_ARTIFACT_DIR/pa5/scopes-graph" student.tests/pa5/scopes-portable.cpp
# Also qualify the retained graph and both driver suites with Clang ASan/UBSan.
python3 student.tests/pa5/measure-scopes.py "$RALPH_ARTIFACT_DIR/pa5/scopes-measure"
```

`scopes.py` adds a portable integration source, 23 portable families and six
syntax rejections. It covers namespace reopening, inline/unnamed visibility,
alias and enum qualifier shadowing, using cycles/transitivity/common-ancestor
placement, namespace-only lookup, enum bodies/underlying types/opaque forms,
typedef/alias scope propagation and qualified expression/declaration decisions.
Reference AST equality is checked for reliable observation families; portable
host acceptance and graph invariants remain independent controls. There are no
course reference changes. `scopes.cpp` destroys the parser before validating
retained scope IDs, indexed bindings, nominated edges, node reachability and
repeatable views.

`measure-scopes.py` freezes a 2k/20k identical-byte namespace/enum/using workload,
qualifies GCC/Clang/reference and exact AST views, then warms and interleaves
three pinned instructions/cycles plus latency/RSS samples for all variants.
Separate branch/cache runs and a sampling profile investigate the large input.
It reports compiler binary and AST sizes separately; emitted executable runtime
and code size do not exist at PA5. Run ordinary `measure.py` with the intended
frozen parent `base-parser` to retain ordinary-group A/B evidence as well.

Loop-10 final evidence is in `$RALPH_ARTIFACT_DIR/pa5/namespaces/final-ordinary`
and `final-scale`. Earlier failed experiments and pre-fix measurements are
retained. The inherited 100k chains are graph-only scale controls: deeply
indented dump bytes grow quadratically with depth. A combined full-view run hit
a command timeout; explicit sanitized graph-only reruns passed for both chains.


## Classes / complete-class regions (loop 11)

```sh
python3 student.tests/pa5/classes.py
python3 student.tests/pa5/coverage.py
sources=$(sed -n 's/^FRONTEND_OBJ_BASENAMES_cppgm++ := //p' dev/frontend_source_sets.mk | awk '{for(i=1;i<=NF;i++)printf "dev/src/%s.cpp ",$i}')
g++ -std=c++11 -O2 -Idev/src student.tests/pa5/classes-graph.cpp $sources -o "$RALPH_ARTIFACT_DIR/pa5/classes/classes-graph"
"$RALPH_ARTIFACT_DIR/pa5/classes/classes-graph" student.tests/pa5/classes-portable.cpp
g++ -std=c++11 -O2 -Idev/src student.tests/pa5/index.cpp -o "$RALPH_ARTIFACT_DIR/pa5/classes/index"
"$RALPH_ARTIFACT_DIR/pa5/classes/index"
# Rebuild both controls with Clang ASan/UBSan as above, not stale binaries.
python3 student.tests/pa5/measure-classes.py "$RALPH_ARTIFACT_DIR/pa5/classes/index-class"
python3 student.tests/pa5/measure-classes.py "$RALPH_ARTIFACT_DIR/pa5/classes/index-parameters-qualified" --parameters
```

`classes.py` qualifies integration + 26 portable families with GCC/Clang and
compares full AST views wherever the reference is reliable; graph/category
controls independently cover the documented reference disagreements. It also
checks six grammar rejections. `classes-graph.cpp` validates retained class/base/
member scope facts, node/literal reachability and single ownership after parser
destruction. `index.cpp` checks 171529 flat-index keys against an independent map,
including zero and maximum IDs, repeated insertions and geometric rehashing.

`measure-classes.py` uses the same warm/interleaved three-repeat protocol as the
ordinary harness, with branch/cache triples and symbolized profiles. The default
2k/20k mix covers inherited types, nested late types, bodies, default arguments,
noexcept, member initializers, special members and bit-fields. `--parameters`
uses 2k/20k/60k parameters to catch environment-chain or visible-binding-copy
costs. The initially attempted 100k single-function case exceeds Clang's parameter
limit and is retained as a failed capability experiment, not a host-qualified
cost comparison. Both modes require a frozen turn-start binary at
`$RALPH_ARTIFACT_DIR/pa5/classes/base-parser` (recorded but not run on unsupported
class inputs). For ordinary measurements, run `measure.py` in a sibling artifact
directory; it compares that binary on supported ordinary inputs.

`coverage.py` retains the seven original probes and adds template-template,
nested-angle, specialization/instantiation and dependent-conversion observations.
The latter also fails in the reference and is reported separately; required
course comparisons remain unchanged. Passing class-only special-member probes
does not imply template-qualified special members are implemented.

## Checkpoint audit controls

`classes.py` also qualifies aliases of class definitions, elaborated class types,
scoped enum definitions and alias-chain/base interactions with GCC, Clang and
exact reference AST comparisons. Alias bindings retain the owned/resolved scope
ID, not a rendered name. `python3 student.tests/pa5/rejection.py` checks 158
fixed source prefixes for clean success/rejection without signals or hangs;
set `PA5_TOOL` to a sanitizer build to inspect partially built scopes and
complete-class queues. Syntax rejection now propagates a compact parser-local
failure; only the explicit driver renders its location/expectation.

Audit performance evidence (including pre/post rendering fixes and raw spread)
is under `$RALPH_ARTIFACT_DIR/pa5/audit12`; `pa5/audit.md` identifies the frozen
binary and protocol. Historical measurements remain in the artifact tree and
git history; no prior target or stage-due control was weakened.
