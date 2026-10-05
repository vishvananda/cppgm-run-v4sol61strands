# PA5 implementation ledger

Stage base commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb
Last reviewed commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb

## Design / spec alignment
- Loop 10 began at HEAD 5e260a5d9ad20d888c2bc6f42ede78f63c7d1f04,
  68/188. Final 77/188: original failures 120 -> 111, no regressions or coverage
  changes. Increments: 12775e55 (indexed environments), 2ab39a96 (lookup rules).
- Owner `syntax`: namespace definitions/reopening/aliases, inline and unnamed
  visibility, using declarations/directives, enum bodies/opaque declarations/
  underlying types/enumerators, typedef/alias scope propagation, qualified
  category decisions including value calls, casts and qualifier-only shadowing.
- Data flow: immutable source -> streaming PostCursor -> bounded common-prefix
  factoring -> TU node/edge/literal + indexed scope arenas -> explicit AST view.
  Names use interned IDs; scope IDs and resolved name/category/terminal facts
  survive parser destruction. No rendered-name keys, TU searches, grammar replay,
  copied token streams, speculative abandoned nodes or process-global caches.
- Scope indexes have lexical parents and nominated edges; namespace ancestry
  controls unqualified using-directive placement (N3485 7.3.4/2,4, local
  doc/n3485.txt:9459-9494). Qualified lookup does not search lexical parents.
  Lookup visits only language-relevant parents/nominations; generation stamps
  avoid per-query TU-sized sets. Import membership is indexed/deduplicated.
  Unscoped enumerators bind directly, not one new lookup edge per enum.
- Work/storage: O(tokens + graph + required lexical/nominated visits), with
  common-namespace ancestry work only for matching nominated candidates.
  Geometric TU-owned arenas; graph release is per TU. No canonical semantic
  types/overload resolution invented at this syntax-only stage. Retain this
  graph/environment surface for PA6+, rather than copying/reparsing it.

## Performance evidence
`$RALPH_ARTIFACT_DIR/pa5/namespaces/{final-ordinary,final-scale}` contains frozen
hashes/flags, CPU0 Xeon E5-2696 v4, GCC/Clang qualification of identical bytes,
full reference AST parity, warmup then 3 interleaved pinned instructions:u /
cycles:u samples, raw IPC, wall/user/system/RSS and 100% event running times.
Separate branch/cache triples and sampled profiles retained; no small timing
improvement claimed. Earlier measurements in `pa5/certified` remain historical.
- Ordinary 2k -> 20k: 830.38M -> 8272.03M instructions (~9.96x). At 20k:
  student 1.28s [1.24,1.30], 277.7MiB, 3577.79M cycles, IPC2.31;
  GCC 1.60s [1.59,1.62], 270.4MiB, 7354.63M instructions / 5147.94M cycles;
  Clang 3.10s [2.99,3.26], 135.6MiB; reference 1.06s [0.99,1.11], 160.6MiB.
  Versus turn-start frozen ordinary parser: +9.53% instructions, +39.6% RSS,
  1.05s -> 1.28s; retained environments and wider typed graph facts cost real
  memory/work, not a hidden speedup. Still bounded host-relative broad costs.
- Namespace/enum 2k -> 20k: 1266.51M -> 12726.23M instructions (~10.05x),
  46.6 -> 540.1MiB RSS. Large: 2.7M tokens, 2.38M nodes, 340001 scopes,
  1.04M category queries, max lookahead 3; 64.2MB AST view measured separately.
  Student 2.09s [2.05,2.29], 5954.87M cycles, IPC2.14;
  GCC 3.04s [3.01,3.31], 768.5MiB, 10185.996M instructions / 9169.50M cycles;
  Clang 8.24s [7.97,8.27], 255.8MiB; reference 1.72s [1.68,1.76], 282.6MiB.
  Supported scale is linear, not a whole-TU lookup scan. Profile dominated by
  cursor/arena allocation and explicit view work, not unrelated-scope searches.
- Rich 2k mix: student .30s / 67.1MiB / 1948.54M instructions, GCC .95s /
  114.4MiB; Clang .99s / 106.4MiB. Largest accepted fixture measured separately;
  sub-tick timing remains inconclusive. Compiler text 359224 bytes.
- 100k unary/assignment graph controls pass with ASan/UBSan, lookahead2; parsing
  .034/.094s in observations. Explicit deeply indented AST output costs
  73.3/146.1s: output bytes grow quadratically with depth, not parser work.
  A combined command timed out during this huge view; graph-only reruns pass.
- Runtime/text/object size of generated programs N/A at PA5; compiler size and
  AST view size are separate. Full host semantic/header parity is not claimed.
  PA24/26 graph/MIR choices must anticipate mandatory PA33/34 per-workload
  <=1.25x GCC executable instructions. No gate or prior target waived/reclassified.

## Validation / handoff ledger
- `make test-report-through-pa4`: 205/205, exit0. `make test-pa5`: 77/188,
  exit2 (incomplete), 111 failures, no accepted-case AST mismatch. File audit:
  47 files pass. No required fixtures, reference outputs or comparisons changed.
- Explicit inherited PA1-4 behavior/cursor/provenance controls pass with hosts;
  PA2/3/4 independent controls pass (6432 PA3 oracle rows). Supplemental PA4
  frozen scale inputs executed explicitly (21k/85k/340k conditionals,
  1k/4k/16k replacements, 2k/8k headers), expected token counts confirmed.
- PA5 ordinary portable file + 13 families/6 rejections/driver reset pass;
  namespace/enum integration + 23 families/6 rejections pass with GCC/Clang.
  Retained graph, no-abandoned-node, stable-view and scope-identity controls pass
  after parser destruction. Clang ASan/UBSan graph and both driver suites pass.
- Independent capability observations (`pa5/remaining-capabilities.json`) now
  pass namespace and enum probes; class/special-member/template/dependent/
  non-type-template probes still fail although GCC/Clang/reference accept.
  Same-source gaps grouped by capabilities, not fixture-specific repairs.
- Inherited two personal reference disagreements remain documented in
  `student.tests/pa5/reference-notes.md`; no bundle revision this turn. New
  portable controls use host qualification plus structured invariants where
  the reference's category handling is not reliable; no course oracle weakened.
- **Unfinished implementation:** classes/bases/members/special functions;
  complete-class late visibility with single-parse deferred member bodies;
  template parameters/arguments, angle splitting and dependent qualified facts.
  Overload/name ambiguity beyond syntax categories belongs to later semantics.
- **Boundary:** namespace/enum/using and their qualified category behavior are
  coherently finished and measured, including newly found common-ancestor and
  namespace-only lookup defects. Remaining course failures require class or
  template grammar/environments. Inline-member late types and dependent template
  arguments need distinct retained body/context discipline; lexical shortcuts
  would violate single parsing/lookup architecture. That new ownership group is
  the next increment, not more local fixes within this completed group.
- **Independent audit questions (not waived):** full namespace lookup candidate
  ambiguity/overload behavior at semantic stages; structural wrapper locations;
  deep mixed/conditional nesting; future complete-class/dependent lookup costs;
  hosted-header capability/representation costs beyond these supported inputs.
  Earlier PA evidence remains preserved. This is an incomplete implementation
  handoff to Ralph, not whole-stage certification or permission to advance.

## Loop 11 active ownership
- Entry HEAD: 10fc90a7e0d9c1a312bd6a45113294b7501fe031; 77/188,
  111 failures. Stage/review markers above are preserved.
- Owner `syntax/classes`: class heads/bases, member declarations, special
  members, member function scopes and complete-class deferred bodies. Data flow
  remains the streaming cursor and indexed TU graph; only complete-class regions
  are retained as compact tokens and parsed once after member declarations.
  No source/grammar replay, TU scans or string lookup keys. Expected work/storage
  O(tokens + nodes + required scope/base visits); validate required ASTs,
  portable GCC/Clang cases, retained graph facts, repeated PMU/latency/RSS controls.
- Separate unfinished owner `syntax/templates`: template parameters/arguments,
  angle splitting and dependent environments. No waiver of inherited controls,
  architecture review or the later PA33/34 executable instruction gate.
- First class increment: required 77 -> 115/188, no coverage edits. Class/member
  environment + base edges, special/ordinary function scope qualification, access,
  bit-fields, specifiers/attributes, declarator common-prefix fixes completed.
  Compact complete-class regions defer bodies/defaults/noexcept/member initializers
  until all nested member declarations exist; each region's grammar runs once.
  Detached region queue preserves nested local-class completion without replay.
  Explicit class portable integration + 22 families/6 rejections pass; reference
  inherited-type and qualified-parameter scope gaps are separately retained.
