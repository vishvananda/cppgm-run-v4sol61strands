# PA5 compact implementation plan

Stage base commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb
Last reviewed commit: 615f033c0a6c4fd106edc7b922173196397a48a3

## Current boundary
Checkpoint audit 12 (2026-10-05) reviewed the entire stage base..code-tip range,
including all three accepted handoffs. Detailed commit/invariant/evidence review
and the single audit ledger row are in `pa5/audit.md`. This is **not** full-stage
acceptance: PA5 remains 115/188, the same 73 failing test identities, no lost
coverage or new failures. PA1–PA4: 205/205. File audit: 49 files, pass.

Implemented owner `dev/src/syntax`: streaming integrated parser and TU-owned
node/edge/literal/scope arenas, explicit AST view; ordinary declarations,
expressions/statements, namespaces/using/enums, classes/bases/members/special
members and single-parse complete-class regions. Compact interned names and
flat scope indexes survive parser destruction. Category facts are syntax facts,
not completed type/overload semantics. Handout spelling hints remain fallback
only; known declarations override them.

Audit fixes (committed code tip above):
- `scopes.cpp:using_declaration` propagates owned class/enum scope identity
  through aliases, including elaborated and definition forms.
- `parser.cpp:error/require/parse` and grammar callers propagate compact sticky
  rejection, not rendered C++ exceptions; `driver.cpp:emit_ast` renders lazily.
- `tree.cpp:dump` batches indentation with a reusable depth-bounded view buffer,
  removing profile-attributed per-depth stream sentries without changing output.

## Validation / performance retained
Artifacts: `$RALPH_ARTIFACT_DIR/pa5/audit12/` (full paths and raw observations in
`audit.md`). Portable ordinary 13, scope 23, class 30 families + integrations,
malformed grammar/driver/TU controls pass. Retained graph/index tests pass;
ASan/UBSan checks all 188 fixtures and 158 deterministic source prefixes without
signals, hangs or sanitizer errors. Same-byte GCC/Clang qualification and exact
reference comparisons retained, including four added alias interactions.

Frozen final binary SHA256:
`2c682b004636e1ca07896e95168bc865211f8b9f87b30c6247a43609518a3ef6`.
CPU0 Xeon E5-2696 v4, warmup, three interleaved pinned grouped user-mode
instructions/cycles samples, 100% running; separate branch/cache passes and
symbolized `perf record` profiles. Full compiler tables include same-input GCC,
Clang/reference observations, IPC, latency/RSS and raw ranges. Final large
student/GCC instruction ratios: ordinary 0.990x, scopes 1.171x, classes 0.233x,
parameters 1.500x. This is syntax-vs-host frontend work, not semantic parity.
Rendering repair reduces instructions 11–24% and cycles 10–19% across larger
fixed inputs; absolute RSS remains a bounded broad factor. Wall spread prevents
precise speedup claims. 2k->20k work remains linear. Raw slowest-test sweep is
startup-dominated; exact tests and amplified host-qualified conditional work are
measured separately. No counters/timeouts are substituted for budgets.

No prior numeric target was reclassified. Historical plan/evidence remain in
Git and artifacts. No fixtures, reference outputs, coverage or comparison rules
changed. PA33/34 per-workload <=1.25x GCC executable instructions is mandatory at
its owning stages and retained; PA5 emits no native code/runtime/text/object.

## Broadly grouped remaining work
1. **Template/dependent grammar and retained environments:** all parameter kinds,
   packs, template/operator/literal-operator IDs, nested-angle versus relational/
   shift expression boundaries, parameter redeclaration/category resets,
   dependent qualified operands, explicit instantiation/specialization and
   interactions with class completion, declarators, lambdas and expressions.
   Existing 11 host-qualified capability probes cover these gaps; seven still
   fail. Retain parsed structure; do not add speculative replay/angle heuristics.
2. **Full-stage integration and scale qualification:** finish the unchanged 188
   fixtures, broaden hosted-header and deep mixed-class/template controls, graph
   ownership, portable reference disagreements with rule proofs, and repeated
   same-input host PMU/RSS comparisons. Current failed hosted/dependent probes
   are missing capabilities, not passing performance evidence.

Avoidable fragmentation: the three handoffs split ordinary syntax, scope facts
and class completion, requiring repeated namespace/default/qualified ownership
repairs and repeated benchmark setup. Keep template/dependent grammar and its
cross-family controls cohesive rather than splitting tiny fixture-driven edits.
Future semantic demand, canonical types, overloads, lowering, ABI and ELF remain
owned by later PAs; PA5 must preserve the graph for them, not duplicate/reparse it.
Ralph owns external acceptance and advancement; this record claims only the
verified checkpoint-preservation boundary, not whole-stage completion.
