# PA5 implementation ledger

Stage base commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb
Last reviewed commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb

## Active extension (loop 10)
- Turn HEAD: 5e260a5d9ad20d888c2bc6f42ede78f63c7d1f04; inherited stage/review
  markers above preserved. Baseline 68/188.
- Owner `syntax`: retained indexed scope environments, namespace/using and enum
  declarations, qualified category decisions. Data flow is interned IDs -> scope
  bindings/edges -> structured name nodes, never joined-string lookup or TU scan.
  Target O(tokens + graph + language-required scope/dependency visits), geometric
  storage; validate fixture ASTs, portable GCC/Clang cases, scope-isolation and
  repeated pinned PMU/latency/RSS scaling.

## Design / completed group
- Turn start 0/188; final 68/188 (120 original failures remain, coverage unchanged).
  Owner `syntax`: ordinary declarations/declarators/type-ids, expressions,
  statements, initialization, scoped category facts, driver/TU ownership.
- Data flow: immutable PP buffers -> structured PostCursor -> bounded lookahead
  -> TU node/edge/literal arenas -> iterative AST view. Interned identifiers,
  direct indexed category bindings and undo scopes; no TU lookup scan, parser
  replay, token-vector copies or string-key semantic signatures. Literal bytes
  are captured before borrowed cursor advancement; source identities retained.
- Factored declaration/expression prefixes once; nested/function/member-pointer
  declarators, typed exception suffixes/handlers, aliases/decltype, linkage,
  casts/traits, lambda capture identities, allocation and attributes. Unary and
  right-associative assignment chains use explicit work stacks. Graph is the
  future semantic surface, not an intermediate to copy/reparse.
- New operator IDs carry token/type/suffix identities. Rendering compact spelling
  is dump-only. Scope restoration covers parameters, branches/loops and TUs.
  New allocation declarators do not consume initialization as parameter clauses.
- Cost: O(consumed tokens + graph + emitted view), O(TU graph + scope undo +
  grammar-depth work) storage. Normal lookahead <=3, 100k chain controls <=2.
  No whole-TU search/cache or speculative abandoned nodes; direct graph control
  checks every node is reachable exactly once, stable views and typed values.

## Performance evidence
`$RALPH_ARTIFACT_DIR/pa5/certified`: frozen hashes/flags, CPU0 Xeon E5-2696 v4,
identical bytes GCC/Clang/reference-qualified (fixture syntax-only), exact AST
parity; warmup + 3 interleaved pinned instructions:u/cycles:u runs, raw IPC,
wall/user/system/RSS, enabled/running=100%; separate branch/cache triples/profile.
- 2k->20k functions: 757.9M->7552.4M instructions (~9.97x), 28->199MiB RSS;
  20k tokens=1,440,012 nodes=1,600,017 queries=360,001 lookahead=3. Large input
  .819s parse/.258s view in one sample; 48.6MB AST is measured separately.
- 20k median/range: student 1.09s [1.09,1.11], 199MiB, 3147.5M cycles,
  IPC2.40; GCC 1.66s [1.62,1.71], 270MiB, 7354.6M instructions/5156.1M cycles;
  Clang 3.07s [3.01,3.08], 136MiB, 7175.6M/10287.1M; reference 1.07s
  [1.06,1.08], 161MiB, 7734.4M/3202.1M. Supported costs remain constant-factor.
- Rich 2k lambda/allocation/exception mix: student .25s [ .25,.27], 52MiB,
  1787.6M instructions/750.7M cycles/IPC2.38; GCC .96s/114MiB/2908.2M;
  Clang 1.01s/106MiB/2005.1M; reference .24s/43MiB/1869.3M.
- Against frozen ordinary-parser increment: +~9.7% instructions, +~29.6% RSS
  for literal/source/structured-node preservation and explicit syntax handling;
  profile shows cursor/lexing/arena growth and explicit output, no search blowup.
  Full host work includes semantic checks absent at PA5; no general hosted-C++
  parity claimed. Earlier noisy small-input timings retained and inconclusive.
- Largest accepted fixture is operator-ID family: student 3.24M instructions,
  2.80M cycles, IPC1.16; reference 3.29M/2.85M. Wall granularity inconclusive.
  Compiler text=341,096 bytes. Generated-program runtime/text/object size N/A at
  syntax-only PA5. No prior target reclassified or performance limit waived.
- PA24/26 graph/MIR choices must anticipate PA33/34 per-workload <=1.25x GCC
  executable instructions; mandatory future gate is not satisfied early.

## Validation / handoff ledger
- Commits include ordinary-parser foundation, factored ambiguity, structured names,
  ownership/scopes/operator and allocation extensions; intended changes committed.
- `make test-report-through-pa4`: 205/205 pass. `make test-pa5`: 68/188,
  exit2 (expected incomplete), no accepted-case AST mismatches. File audit:
  46 files pass. No required fixtures/reference/comparison/coverage edits.
- Explicit PA1-4 checks/audits pass (including typed provenance, 6,432 PA3 oracle
  rows, PA4 interactions and inherited scale input controls). PA5 13 families,
  portable integration file, 6 rejections, driver/TU resets pass with GCC/Clang;
  graph and 100k unary/assignment checks pass, including Clang ASan/UBSan.
  Sanitized driver runs the same explicit semantic/rejection controls.
- Two personal reference defects (implicit substatement scopes and new int*())
  have reducers, local N3485 clause/line proof and exact bundle revision in
  `student.tests/pa5/reference-notes.md`; required outputs are unchanged.
- **Unfinished implementation:** namespace/using entities; class/enum bodies,
  bases/members/special functions; template parameters/argument lists, angle
  splitting and dependent/qualified category lookup. Seven host-qualified
  capability probes record each current gap in `remaining-capabilities.json`.
  Ordinary-group failures now depend on these missing environments, not more
  local expression fixes. Full-stage validation remains non-passing.
- **Boundary:** extend next with stable scope/entity handles and qualified
  category indexing shared by namespace/class/template owners. Complete-class
  visibility/dependent template facts cannot be added coherently as lexical
  shortcuts to the completed ordinary group; they need their own retained
  environments and single-parse body discipline. Stop here after finishing,
  testing and measuring the related group, not at minimum progress.
- **Independent audit questions (not waived):** source locations of structural
  wrappers; deep mixed nesting/conditional chains; full retained-graph extension
  without copying at PA6; complete-class/deferred-body and qualified lookup
  complexity once implemented; general hosted capability and emitted-code
  quality at their owning stages. No independent whole-stage certification.
