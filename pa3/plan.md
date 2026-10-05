# PA3 full-stage — final independent audit

Stage base: `99a5fe4c45fac2edcaef92d5984d07a03cd676ba` (PA2 audit).
Independently reviewed stage commits: `3c0beafa`, `e3d2ea6c`, `0d170da6`,
`84bdd387`, plus the changes committed with this final audit. Entry markers are
preserved here; implementation checkpoints were not acceptance reviews.

## Final Spec Alignment / design
Immutable source -> shared streaming `Lexer` -> one-lookahead typed `PostCursor`
-> explicit precedence stacks of source-ranged constant facts -> value renderer.
PA3 has no declarations/templates/LowIR/ELF; it does not manufacture downstream
representations. Each logical expression is parsed once. PA2 types determine
signed/unsigned 64-bit promotion; identifiers use canonical IDs. `defined` has
an ID/context callback for the future macro table, not rendered-name lookup.

`ControllingExpression::parse/reduce` validates every operand's grammar/literal
and reduces typed values immediately. Deferred arithmetic-domain validity is
selected by lazy `&&`, `||`, `?:`; conditional signedness uses both arms. There
is no retained node tree, second evaluation traversal, recursive native parser,
full-TU token vector, text-semantic roundtrip or host/reference delegation.
Storage/work is O(bytes + tokens + interned spellings), with stacks bounded by
live expression nesting, not the total number of reduced operators. Error
recovery drains only the current logical line; phase-1/2/3 exceptions remain
fatal. Bulk vectors/slabs have explicit invocation owners and no hot per-node
allocation. Full invariant evidence and capability coverage: [audit](audit.md).

## Findings and cohesive changes
1. Expected syntax/domain rejection used C++ exceptions; malformed-line sampling
   found exception unwinding dominant. Status returns and typed deferred-error
   facts remove this cost without swallowing lexical/resource exceptions.
2. Retaining a typed tree then evaluating it again made flat-chain memory grow
   unnecessarily. Parse-time reduction discards dead facts, preserves source
   ranges, grammar validation, both-arm types and lazy errors, and retains only
   explicit compact stacks. A 900k-term chain has two live facts/one operator.
3. Impossible identifier/literal-prefix probes preceded single punctuation.
   `Lexer::next` now filters safe single terminals first, retaining phase
   translation and multi-character maximal munch. Inherited controls rerun.
4. Added independent typed oracle and frozen focused PMU/profile controls.
   Telemetry distinguishes logical nodes, live facts, valid reductions and
   deferred domain errors; earlier raw measurements remain intact.

## Performance evidence / budgets
Details, all medians/ranges, comparison limitations and raw paths:
[performance](performance.md), final `audit-handoff` appendix. CPU 0, warmup +
three interleaved same-session repeats, frozen inputs/binaries/flags/hosts,
user-mode instructions/cycles/IPC, wall/user/system/RSS, separate branch/cache
passes and `perf record` attribution. Raw files live under
`/home/vishvananda/work/private/v4sol61strands/artifacts`.

Final focused before -> after: malformed instructions 2237.869M -> 402.910M,
wall 0.23 -> 0.05s; 900k chain RSS 120412 -> 5780 KiB, wall 0.32 -> 0.18s;
full triple instructions 7368.594M -> 5785.189M. Full-triple wall ranges overlap
(0.87–0.97 vs 0.68–1.10s): instruction/cycle benefit, not proven latency win.
Final fixed PA3 instructions are 0.895–0.973x entry, 0.760–1.710x GCC. Nested
cycles/latency regress versus entry despite lower instructions; final 0.32s
median vs GCC 0.13s (2.46x) is bounded, profiled and disclosed, not GCC parity.
Punctuation filtering improved that owning path versus its immediate precursor.
No growing per-query cost, timeout-as-budget or numeric target reclassification.
Inherited compiler controls remain measured on identical input. Host directives,
PA2 lexical-vs-semantic work and course promotion differences are disclosed.
Generated-program runtime/text/object size is N/A for token/value stages.
Mandatory PA33/34 per-workload <=1.25x GCC instructions remains unchanged.

## Validation
Required: file audit **33 files PASS**; through-PA3 **100/100 PASS** (PA3 20,
PA1–2 80). Personal PA1, PA2, independent PA2, PA3 and independent PA3 controls
pass. Independent oracle: 6432 GCC/Clang/reference rows plus 39 interaction/
boundary cases; inherited PA3 control: 1200 native/PP-qualified rows and 100k
depth/chains. Clang implementation and ASan+UBSan pass both PA3 controls.
Future backend host qualification passes; 8/8 deliberate failures detected.
Stage PA3 native and instruction-gate controls correctly report N/A, not PASS.
No fixtures, references, comparison rules or required coverage changed.
Final rerun evidence and acceptance ledger are in `audit.md`.

## Ledger / handoff
- Completed: all PA3 syntax, typed promotion, lazy-domain handling, phase/line
  ownership, inherited shared surfaces, independent architecture review and
  performance investigation. Previous `84bdd387` handoffs (whole grammar/type,
  ownership, inherited controls and PMU investigation) are resolved here.
- Open PA3 defects / unaudited handoffs: none known after independent review.
- Future assignment work: PA4+; macro-table callback, declarations/templates,
  typed lowering/backend and their due budgets remain future work, not waived.
  Clang rejection of a GCC-expanded hosted TU is preserved as a host-envelope
  qualification gap, not claimed student hosted-C++ semantic support at PA3.
- Ralph owns external reruns, acceptance and advancement; this is the model-owned
  audit handoff, not a modification of Ralph goal/state files.
