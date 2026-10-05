# PA5 compact implementation and final-audit plan

Stage base commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb
Last reviewed commit: 6e277374e823970a07b052fd9201beab4db0f82d

## Final boundary — audit 14, October 5, 2026

Entry HEAD: `112d17ee4c6dbdbb69cf3969786c7185475553c7`.
Independent review reconstructed the complete stage from spec, assignment,
grammar, every stage commit and combined source; the checkpoint conclusions
were not used as a substitute. Code review covers base exclusive through the
reviewed commit inclusive. Previous reviewed marker `615f033c` and checkpoint
12 remain preserved in `pa5/audit.md`; loop 13 evidence remains unchanged.

Full-stage PA5 syntax is implemented and independently reviewed. Course tests,
references, grammar, harness, coverage and comparison rules are unchanged:
PA5 **188/188**, PA1–PA4 **205/205**, through PA5 **393/393**; file audit
**50 files, pass**. Stage advancement belongs to Ralph, not this ledger.

## Final design / Spec Alignment

Owner: `dev/src/syntax`. Immutable source/identifier IDs and PA1–PA4 streaming
preprocessor/posttoken cursor feed one TU-owned structured node/edge/literal/
scope graph. Indexed syntax categories guide grammar choices; they are not
canonical semantic types, deduction, overload results or instantiated bodies.
The deterministic AST text is an explicit output view, never phase transport.

Ordinary declarations/expressions/statements, namespaces/enums/using,
classes/bases/member/special/complete-class contexts and template/dependent
syntax share declarators, qualified names and single-parse ambiguity factoring.
Templates retain parameter environments and structured clauses/arguments,
including packs/defaults/template-template parameters, specialization and
explicit-instantiation syntax. Only entity categories are published; parameters
do not leak or overwrite template categories. Delimiter-aware logical angle
splitting preserves expression shifts and source identity.

Complete-class deferral stores compact tokens only for required regions, parses
that grammar once, restores input/scope/angle state on rejection and releases
region storage. Qualified out-of-class member templates use immutable shared
name-index overlays, not binding copies or mutation of published scope chains.
Sticky compact TU-local grammar failure renders only at the driver boundary.
Arenas grow geometrically; IDs/non-owning edges avoid per-node ownership and
recursive destruction. Lookahead observed 2–3, controls enforce the established
≤4 bound. Work tracks consumed tokens, produced nodes and actual scope edges;
no TU scan per name, tree copy, textual lookup key or global retry.

## Audit findings and completed changes

Repair commit `6e277374` fixes qualified member-template parameter visibility,
shared `decltype`-qualified/`typename`/explicit-`template` syntax, dependent
statement declaration versus conversion ambiguity, structured callee retention,
and enum/lambda-return/`sizeof...` rendering interactions. Owners and regressions
are recorded in `pa5/audit.md`. The final evidence/docs commit consolidates the
review; no course reference correction or target reclassification was needed.

## Validation and performance

Delivery required gates pass: `perl scripts/cppgm_file_audit.pl --stage pa5
--paths dev/src` and `make test-report-through-pa5` (393/393), rerun after
consolidation (`consolidated-fileAudit.log`, `consolidated-through.log`,
`consolidated-gates.exit`: 0). Independent
controls: ordinary 13, scope 23, class 30, template 32 families, six grammar
rejections per group; **42** GCC/Clang-qualified exact reference interaction
comparisons; 11 inherited/new capability probes; **211 prefixes + 7** deferred
rejection reducers. Final ASan/UBSan: all 188 fixtures and family controls;
**223** accepted-fixture/interaction graphs verify unique ownership, locations,
acyclic scopes, shared overlays, parser-death survival and stable dumps.
Compact-index oracle **171529 keys**; inherited PA1–PA4 controls pass.

Frozen protocol, same-input host/reference/A/B medians/ranges, phase/work and
size evidence: `student.tests/pa5/loop14-evidence.md`, artifacts
`$RALPH_ARTIFACT_DIR/pa5/loop14/`. Final2 uses CPU3, three interleaved repeats,
primary instructions/cycles + per-run IPC, separate branch/cache passes,
100% running; noisy ordinary/parameter timings investigated with five isolated
repeats and accumulated profiles. Every observation is preserved.

Student/GCC compiler-instruction ratios: ordinary **1.061×**, scopes **1.243×**,
classes **0.247×**, parameters **1.608×**, templates **0.705×**. Final correctness
repairs add ~0.17–0.19% instructions, not a claimed speedup; timing ranges overlap
entry. Compiler text/data/bss **425172/4128/1568 bytes**. Profiled inherited
conditional preprocessing remains **3.382× GCC**, separately disclosed with its
prior owner repairs and proportional scaling. Parameter/arena/cursor constant
factors remain tuning opportunities, not a waived nonlinear ownership defect.
All historical measurements/targets remain; timeouts are hang guards, not budgets.

## Ledger and retained obligations

- Complete: accumulated whole-stage source/capability/ownership/cost review,
  all unaudited loop-13 handoffs (`d59b9532`, `ef9d937d`, `112d17ee`), final repairs
  and validation. No unaudited PA5 code handoff remains through the review tip.
- No known required PA5 C++11 syntax, correctness, self-containment, timeout,
  file-audit or stage-due architecture defect remains. Vendor extensions/builtin
  transforms, `__type_pack_element`, `unsigned __int128`, `_Float32` remain outside
  this grammar; their hosted-header gaps and same-source comparisons stay in
  `student.tests/pa5/reference-notes.md`. They are not counted as portable passes.
- Future owners must consume this graph directly: canonical semantic identity,
  deduction/instantiation, demanded work, effects/alias facts, typed LowIR,
  per-function MIR and direct ELF. A source-to-ELF/demanded-template trace and
  generated runtime/text/object measurements are unavailable at PA5, not certified.
- Keep stage-due supplemental controls active thereafter. Preserve optimization
  legality/profitability/invalidation and work/growth budgets at their owning PAs,
  PA24/26 call/EH constraints, PA32 dataflow/inlining and PA33 allocation/cleanup.
  PA33/PA34's per-workload **≤1.25× GCC O2/O3 executable-instruction gate** remains
  mandatory; PA34 whole-self runtime is separate. Frontend ratios do not waive it.
