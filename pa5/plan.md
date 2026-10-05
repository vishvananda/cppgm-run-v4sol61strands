# PA5 implementation ledger

Stage base commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb
Last reviewed commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb

## Design / spec alignment
- Loop 11 entry HEAD: 10fc90a7e0d9c1a312bd6a45113294b7501fe031, 77/188.
  Final 115/188: original failures 111 -> 73, 38 resolved, zero newly failing
  tests, unchanged 188-case coverage. First increment: 0cfee266 (class grammar,
  scopes and single-parse complete-class regions); final increment adds compact
  scope indexes, declaration-prefix visibility and graph/literal ownership.
- Owner `syntax`: extends prior ordinary + namespace/enum/using categories with
  class/struct/union heads, forwards/anonymous forms, bases/access, members and
  bit-fields, constructors/destructors/conversions, qualified member definitions,
  operator calls/member pointers, default/delete, constructor initializers,
  function-try blocks, contextual override/final and omitted attributes/alignment.
- Data flow: immutable source -> streaming PostCursor -> bounded common-prefix
  factoring -> TU-owned structured node/edge/literal + indexed scope arenas ->
  explicit AST view. IDs, source identity and scope/category facts survive parser
  destruction; no rendered-name lookup keys, TU lookup scans, abandoned nodes,
  token-vector pipeline, copied environments or process-global mutable caches.
- `classes.cpp:defer_body/defer_expression/finish_bodies` retains compact tokens
  only for complete-class bodies/defaults/noexcept/member initializers. Balanced
  boundary collection is not grammar parsing. Each region is parsed once after
  completion; nested/local-class completion detaches queues safely. Literal bytes
  remain in the TU arena. `extended_expression.cpp:attributes` preserves unique
  literal ownership even for omitted attribute views.
- `scopes.cpp:lookup` uses direct indexed names, lexical parents and nominated/
  base edges; namespace common-ancestor rules are preserved. Generation stamps
  bound diamond/cyclic visits to once/query. `index.h:SyntaxIndex` stores dense
  entries + open-addressed compact-ID slots, geometric growth, no entry allocation
  or all-identifiers-sized per-scope table; expected amortized O(1) indexing.
- Default arguments freeze parameter point-of-declaration via one compact prefix
  overlay on the original function index, not one ancestor frame per parameter.
  Later parameters are filtered by declaration order; default-local scopes remain
  visible. Class bindings share the completed class index. Work/storage is
  O(tokens + graph + language-relevant scope/edge visits); no parameter-count
  factor in lookup. Complete-class storage is released on completion, graph on TU
  release. Full types, candidate ambiguity and overload semantics remain PA6+;
  retain this structured graph rather than reparse/copy it at that boundary.

## Performance evidence
Artifacts: `$RALPH_ARTIFACT_DIR/pa5/classes/{index-ordinary,index-class,index-parameters-qualified}`.
Frozen binary SHA256 237141f10d62c502bb516d82854e1bc692bad1d416a92f49b5ee71bec6b2eddc;
flags/input hashes/versions in manifests. CPU0 Xeon E5-2696 v4; identical bytes
qualified by GCC/Clang and full reference AST parity before measurements.
Warmup then three interleaved pinned `{instructions:u,cycles:u}` runs, IPC,
wall/user/system/RSS, 100% running times. Raw ranges and all observations retained.
Separate branch/cache triples + symbolized cycles profiles; no counter multiplexing.

| Workload / compiler | Instructions M | Cycles M | IPC | Wall median [range] s | RSS MiB |
|---|---:|---:|---:|---|---:|
| ordinary20k student | 8546.13 | 3708.24 | 2.30 | 1.54 [1.37,1.57] | 299.5 |
| ordinary20k entry | 8271.91 | 3588.35 | 2.31 | 1.39 [1.31,1.50] | 277.6 |
| ordinary20k GCC | 7354.64 | 5034.68 | 1.46 | 1.95 [1.86,2.01] | 270.4 |
| ordinary20k Clang | 7186.20 | 10105.08 | 0.71 | 3.42 [3.03,3.68] | 135.6 |
| ordinary20k reference | 7734.19 | 3232.18 | 2.39 | 1.16 [1.08,1.28] | 160.9 |
| classes20k student | 14863.01 | 6765.12 | 2.20 | 2.86 [2.79,2.89] | 561.8 |
| classes20k GCC | 55146.02 | 88954.78 | 0.62 | 30.86 [30.30,31.28] | 2067.4 |
| classes20k Clang | 17146.70 | 30082.83 | 0.57 | 10.38 [10.10,10.50] | 355.8 |
| classes20k reference | 14930.54 | 6227.92 | 2.40 | 2.27 [2.13,2.28] | 271.9 |
| parameters60k student | 3225.00 | 1411.57 | 2.28 | 0.65 [0.60,0.66] | 169.4 |
| parameters60k GCC | 1670.95 | 791.35 | 2.11 | 0.33 [0.31,0.33] | 119.5 |
| parameters60k Clang | 2109.46 | 1470.12 | 1.43 | 0.51 [0.50,0.53] | 131.2 |
| parameters60k reference | 3653.76 | 1605.30 | 2.28 | 0.67 [0.59,0.67] | 111.8 |

- Ordinary/class 2k -> 20k instructions: 857.78M -> 8546.13M (~9.96x),
  1491.01M -> 14863.01M (~9.97x). Parameter 2k/20k/60k: 109.21M/1076.34M/
  3225.00M; linear supported growth, not quadratic prefix lookup. The attempted
  100k function exceeds Clang's parameter limit; failed run retained, not counted
  as portable parity. Initial small parameter timing was unstable; use the larger
  qualified run, not a speedup claim. Harness timeouts are not budgets.
- Ordinary versus frozen entry: +3.32% instructions, +3.34% cycles, +7.85% RSS;
  wall ranges overlap, timing delta inconclusive. Feature/ownership work has real
  cost. GCC comparisons include semantics while PA5 only parses, not equal whole-
  compiler capability. Student/GCC ordinary wall/RSS 0.79x/1.11x; class
  0.09x/0.27x; parameter 1.97x/1.42x, bounded broad cost rather than tuning proof.
- Profiles: 312/613/139 samples, zero lost; useful parser/lexer/output call stacks.
  peek + output dominate, lookup not a growing scan. Unknown kernel-symbol samples
  1.39%/2.19%/3.54% are unresolved attribution, not an optimization conclusion.
  Student branch misses 0.560%/0.734%/0.405%; CPU-specific cache miss ratios
  47.00%/40.18%/46.68%, raw host passes retained; ratios alone are not a budget.
- Telemetry (20k class): 2.46M tokens, 2.76M nodes, 1.06M queries, 460001 scopes,
  lookahead3, parse2.30s/dump0.50s. Parameters60k: 480022 tokens, 660029 nodes,
  240008 queries, 120006 scopes, lookahead2, parse0.52s/dump0.13s.
- Compiler text390048 bytes; AST view sizes separate in summaries. Generated-
  program runtime/text/object size N/A at syntax-only PA5. Mandatory PA33/34
  per-workload <=1.25x GCC executable-instruction gate remains unwaived; plan
  bounded demand, compact shared graph and per-function release before PA24/26.
  Earlier evidence remains under `pa5/{certified,namespaces,inherited-scale}`;
  pre-index loop11 runs are historical, not discarded or reclassified targets.

## Validation / handoff ledger
- Required `make test-pa5`: 115/188, exit2 (incomplete, 73 failures).
  `make test-report-through-pa4`: 205/205, exit0. File audit: 49 files, exit0.
  Logs: `pa5/classes/{stage-index,prior-index}.log`; final required reruns also
  recorded as `stage-handoff`, `prior-handoff`, `file-audit-handoff` there.
- Inherited PA1-4 personal + independent controls rerun/pass, including PA3's
  6432 oracle/GCC/Clang/reference rows. Logs `pa5/classes/pa*-*final.log`.
- PA5 ordinary integration +13 families/6 rejections/reset pass; namespace/enum
  +23/6 pass; classes integration +26 GCC/Clang-qualified portable families/6
  grammar rejections pass. Retained graph controls pass after parser destruction
  (class481 nodes/432 tokens/128 queries/lookahead3). Flat-index171529-key oracle
  passes. Clang ASan/UBSan graph/index and all three driver controls pass.
- Same-source capability observations retain original seven probes + four new
  families: namespace/enum/class/qualified-special pass; template/dependent/
  non-type/template-template/nested-angle/specialization/dependent-conversion fail.
  GCC/Clang accept all; reference also rejects dependent conversion. Evidence:
  `pa5/remaining-capabilities.json`. This is coverage of gaps, not passing tests.
- Reduced reference disagreements and C++11 proofs are in personal
  `reference-notes.md`: inherited types, qualified member scope, parameter
  expression versus typedef in defaults, plus inherited observations. Bundle
  revision/hash and required fixtures/comparisons unchanged; no oracle revision.
- **Unfinished implementation:** template parameters/arguments including packs,
  template-template/non-type parameters, dependent qualified/category facts,
  nested-angle splitting versus relational/shift expressions, instantiation/
  specialization syntax and interactions with expression/declarator ambiguity.
- **Boundary:** the class/member/complete-class ownership group is finished,
  including integration-discovered special/conversion/function-try wrappers,
  default visibility and attribute literal ownership. Further related fixture
  progress needs retained template patterns and dependent argument/context
  discipline, not a class-local repair. Extending the collector with speculative
  angle guesses or reparsing would violate the spec. This separate substantial
  grammar/environment group is the next increment, not waived unfinished work.
- **Independent audit questions (not waived):** broader hosted-header capability,
  class/base lookup ambiguity and overload checking at semantic stages, deeply
  nested complete-class/dependent combinations, retained source-wrapper fidelity,
  hosted memory/counter costs and unknown sampling attribution. No whole-stage
  certification or advancement requested; Ralph owns independent acceptance.
