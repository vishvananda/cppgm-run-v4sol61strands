# PA5 final whole-stage audit 14 — October 5, 2026

## Disposition and independent review boundary

**PA5 full-stage syntax is implemented, repaired and independently reviewed.**
Required delivery gates pass: fileAudit **50 files**, cumulative **393/393**
(PA5 188, earlier PAs 205). No known required stage-due correctness,
self-containment, timeout, file-audit or architecture defect remains. Ralph owns
acceptance/advancement; this is the model-owned final handoff, not a goal update.

- Stage base: `08275a628da86ffd921633806a3f9ca72fcaaaeb`.
- Previous independently reviewed code: `615f033c0a6c4fd106edc7b922173196397a48a3`.
- Turn-entry HEAD: `112d17ee4c6dbdbb69cf3969786c7185475553c7`.
- Final reviewed code: `6e277374e823970a07b052fd9201beab4db0f82d`.
- Read `spec.md`, `pa5/README.md`, `pa5/parsing.md`, `pa5/pa5.gram`, reference
  policy, plan, historical audit, all stage commits/diffs and combined source.
  Reconstructed ownership and grammar interactions from source, rather than
  substituting checkpoint conclusions. Review covers **base exclusive through
  final reviewed code inclusive**, including all earlier ownership groups.
- The historical audit 12 below is preserved verbatim: its 115/188 disposition,
  failures and performance observations are historical, not current results.
  Loop-13 observations remain in `student.tests/pa5/loop13-evidence.md`.

Artifacts below are relative to `$RALPH_ARTIFACT_DIR/pa5/loop14/`.
`final-range.txt` and `final-source.diff` retain the complete stage range/source
view. Raw performance observations are not committed generated output.

### Handoffs since the previous checkpoint — now reviewed

| Commit | Independent accumulated review |
|---|---|
| `3d2ef351` | Historical checkpoint record; no implementation change or retroactive full-stage claim. |
| `d59b9532` | Template parameter/argument environments, dependent names, angle splitting, class/ordinary integration and single-parse retained graph; all 73 checkpoint fixture failures resolved. |
| `ef9d937d` | Factored type/initializer syntax, deferred context restoration and rejection; verified parent/context restoration rather than relying on accepted fixture output. |
| `112d17ee` | Implementation evidence/control handoff and outstanding audit questions; entire inherited surface re-reviewed, not only newly added syntax. |
| `6e277374` | Final independent-review repairs and permanent interaction/graph/measurement controls described below. |

No unaudited PA5 code handoff remains through the final reviewed code marker.

## Reconstructed design / final Spec Alignment

Production path: `dev/src/syntax/driver.cpp:emit_ast` creates TU-local source,
identifier, preprocessor, posttoken and graph owners; PA1–PA4 stream immutable
source identities and compact tokens through
`preprocess/engine/preprocessor.cpp:Preprocessor::next` and
`preprocess/post/cursor.cpp:PostCursor::next`; `syntax/parser.cpp:parse` and
shared declaration/expression/statement/name productions populate indexed scope
categories and TU-owned node/edge/literal arenas. `syntax/templates.cpp:
template_declaration` retains clauses, parameter environments and structured
bodies; `syntax/classes.cpp:finish_bodies` completes only required class regions.
`syntax/tree.cpp:dump` renders the retained graph deterministically. Output text
is a view, never a phase input or lookup key.

A nontrivial out-of-class member-template declaration follows interned qualifier
IDs through `names.cpp:qualified_raw`, category resolution in `scopes.cpp:lookup`,
shared retained template frames in `scopes.cpp:qualify_scope`, and the common
`parser.cpp:declarator`/`templates.cpp:template_clause` graph constructors. Its
body consumes that environment without source replay. Class member defaults,
`noexcept`, inline bodies and lambdas use language-required completion queues;
parsed template bodies remain structured nodes. AST/control checks ensure the
rendering wrappers do not become abandoned alternative trees.

PA5 deliberately supplies syntax categories, **not** canonical semantic types,
overload sets, deduction, substitution or instantiated semantics. There is no
LowIR, MIR, ABI lowering, native executable/object or ELF writer yet. Therefore a
demanded-template source-to-ELF trace is **unavailable, not certified**. Later
assignments must consume this graph directly, add canonical/demand/effect facts
and retain per-function work/release boundaries; syntax text cannot serve as a
handoff. This boundary follows the assignment, not a waiver of later spec duties.

## Findings, full ownership-path repairs and regression proof

Repair commit `6e277374` changes grammar/scope/view owners together, not fixture
answers. Personal comparisons qualify portable source using GCC and Clang in
C++11 mode before exact reference AST comparison; host agreement alone does not
justify altering course references.

1. **Qualified member-template frames were lost.** Switching an out-of-class
   declarator to its qualified class scope discarded inner template parameters.
   `scopes.cpp:qualify_scope` now retains template-frame lookup order using
   compact overlays with `names_owner`. `parser.cpp:declarator` and
   `classes.cpp:special_member` share the owner. `scopes.cpp:lookup` follows the
   shared index; published parameter/class chains are neither copied nor
   reconnected. Graph assertions verify overlays contain no copied bindings and
   parent chains remain acyclic after parser destruction.
2. **Dependent `decltype` paths diverged.** `names.cpp:decltype_name` now owns
   structured `decltype(...)::type`, qualified `typename` and explicit
   `template` suffix facts; declarations, expressions, class bases and template
   arguments consume that same production. `tree.cpp:compact/dump` retains
   qualifier and template facts without rendered-name lookup.
3. **`typename` statement ambiguity rejected valid declarations.**
   `statement.cpp:statement` now routes dependent type-led declarations through
   the established single-parse `ambiguity.cpp:ambiguous_statement` owner.
   Parenthesized declarations, pointers/references, function declarations and
   functional conversions retain their parsed common prefix. Factored callee
   payloads stay structured rather than being rebuilt from text.
4. **AST cross-family facts differed.** Lambda trailing returns, class-member
   enums and `sizeof...` in `noexcept` now preserve their graph facts and explicit
   rendering conventions across expression/class/declaration/view owners.
5. **Rejection paths remain clean.** Added reducers cover malformed dependent
   names and deferred/template interactions; sticky rejection forbids later
   cursor advancement or incomplete publication, and completion restores the
   detached input/scope/angle context even on failure.

Permanent regressions: `student.tests/pa5/interactions.py` (**42** exact
host-qualified reference comparisons), `interactions-graph.cpp`, extended
`rejection.py`, and reproducible frozen `audit-measure.py`. Intermediate
failures, late parenthesized-typename reductions and repaired outputs remain in
artifacts; final ordinary/sanitized delivery interaction logs both report 42.
No fixture, reference, grammar, harness, required behavior, coverage or comparison
rule changed. No reference-correction exception was exercised. Existing reduced
reference discrepancies retain their C++11 rule proofs in
`student.tests/pa5/reference-notes.md`; bundle revision/hash remain unchanged.
`decltype` member-pointer syntax accepted by hosts but absent from this PA's
ptr-operator grammar is recorded as outside scope, not a portable passing probe.

## Architecture audit — every exercised invariant and exact owner

Paths abbreviated below are relative to `dev/src/`; `syntax/` is explicit to
avoid confusing its driver with any later driver surface.

| Spec invariant | Upholding `file:function` and inspected boundary |
|---|---|
| One grammar parse per source region; no later lexer/parser | `syntax/parser.cpp:peek/take/specs/declarator`, `syntax/scopes.cpp:prepare_name`, `syntax/ambiguity.cpp:ambiguous_statement` factor bounded common prefixes and move structured nodes. `syntax/classes.cpp:defer_body/defer_expression/finish_bodies` retain only balanced complete-class regions, parse their grammar once, restore outer state and release token storage. `syntax/templates.cpp:template_declaration` retains the body graph. There is no semantic/lowering replay owner at PA5. |
| Compact canonical keys; no rendered lookup/cache keys | `preprocess/lex/identifiers.cpp:IdentifierTable::intern`; `syntax/index.h:SyntaxIndex::find/insert/grow`; `syntax/scopes.cpp:bind/lookup/qualify_scope`; `syntax/names.cpp:qualified_raw/decltype_name`. IdentifierId/ScopeId/NodeId are the relationships. `syntax/tree.cpp:compact/dump` is view-only. Canonical types/specializations are not yet implemented. |
| Bounded query work, no whole-TU lookup scans | `syntax/scopes.cpp:lookup` indexes the queried identity, applies parameter/qualifier restrictions, and visits actual lexical/base/import edges with query stamps. `common_namespace` walks ancestor paths. `qualify_scope` shares only the retained template-frame path, not all bindings; default-prefix overlays apply an order bound to the existing function index. `syntax/index.h:grow` geometrically rehashes on insertion; `syntax/parser.cpp:declared_name` traverses declarator-local nodes. |
| Cheap filters before expensive semantic work | `syntax/scopes.cpp:lookup` filters parameter order, namespace-only and qualifier category before binding acceptance; `syntax/parser.cpp:type_start`, `syntax/templates.cpp:template_argument` and `syntax/names.cpp:qualified_component` use indexed categories. Deduction/conversion/overload processing is future semantic work, not concealed inside syntax classification. |
| Demand-driven instantiation/mangling/lowering/emission | `syntax/classes.cpp:finish_bodies` processes the detached required completion queue exactly once; `syntax/templates.cpp:template_declaration` retains patterns and publishes only the entity, not parameters. Explicit-instantiation nodes are syntax, not eagerly instantiated bodies. There is no mangling/lowering/emission owner to falsely certify at this stage. |
| Precise worklists, complete keys, direct typed data | `syntax/classes.cpp:finish_bodies` detaches the queue before nested completion, restores all cursor/scope/angle context, and releases it. `syntax/scopes.cpp:hint` caches only immutable spelling-derived TU hints keyed by identifier; environment-dependent category queries are not cached. `syntax/parser.cpp:error/require/parse` implement compact sticky rejection. `syntax/tree.cpp:node/append/literal` construct the single retained graph, with no text roundtrip or semantic copy. |
| Explicit allocation/release boundaries | `syntax/tree.cpp:node/append/literal`, `syntax/index.h:SyntaxIndex` use geometric dense storage and non-owning IDs; no owning shared_ptr/per-node allocation or recursive graph destruction. `syntax/scopes.cpp:qualify_scope` shares indexes. `syntax/classes.cpp:finish_bodies` releases transient deferred regions. `syntax/driver.cpp:emit_ast` destroys parser/graph/cursor/preprocessor/source/identifier state per TU. Process-global tables are bounded read-only grammar metadata, not accumulated TU caches. |
| Counters/timers and trigger attribution | `syntax/driver.cpp:emit_ast` reports tokens/nodes/edges/scopes/category queries/lookahead and parse/dump times; preprocessing expression/macro telemetry retains inherited work ownership. `student.tests/pa5/audit-measure.py` + exact-slowest/inherited scripts freeze same-input host/A/B, primary PMU counts, latency/RSS, output checks and section size. Final/isolated/slow/inherited profile reports and counter running checks are identified in the evidence record. |

### Legality, profitability, invalidation and budgets

No native optimization transform is implemented at PA5. Parser factoring must
preserve grammar preference, source identities and structured facts; category
updates respect declaration/parameter publication and complete-class visibility.
Shared qualified template overlays are legal only over retained parameter indexes
that are complete for the declaration being parsed; they alter lookup order,
not the published owner. There is no environment-sensitive memoization to become
stale after binding publication. Cheap immutable lexical hints are subordinate
to real bindings. Deferred ordering is the explicit class-language necessity,
not speculative global retries.

Budget model is structural: bounded lookahead/common-prefix probes, consumed
source/expansion tokens, produced nodes/edges, actual scope paths/import edges,
geometric growth and required deferred regions. Host ratios, graph lifetimes,
scales, phase counters and profiles verify these paths; timeout/memory hang guards
are not acceptable-work budgets. Current repairs are correctness repairs with a
measured ~0.17–0.19% instruction cost, not an unproved profitability claim.
Earlier batched dump/shared-index/PA4 allocation repairs retain their A/B proof.
Future transforms must separately prove legality and profitability, declare
work/growth caps and precise invalidations, and retain conservative fallbacks;
PA24/26 placement/call/EH, PA32 dataflow/inlining and PA33 allocation/cleanup are
not certified early by these syntax outcomes.

## Performance evidence and investigated costs

Complete host/reference/A/B tables, ranges, binary/input hashes, environment,
flags, affinity, branch/cache counts, profile attribution, phase/work and size
boundaries: **`student.tests/pa5/loop14-evidence.md`**. Raw artifacts:

- `final2/`: final code, CPU3, eight fixed workloads; three interleaved runs per
  variant after warmup, grouped instructions/cycles and per-run IPC; separate
  branch/cache triples and template/parameter student/GCC profiles.
- `isolated2/`: five interleaved repetitions of ordinary20k/parameters60k after
  other measurements finish; accumulated symbolic student/GCC profiles. Initial
  noisy trials in `final/` and `final2/` remain intact, not replaced or discarded.
- `telemetry/`: ten scale/phase observations with byte hashes and work counters.
- `fixture-times.json`, `exact-slowest/`: all fixture observations and top six
  unchanged same-byte host/reference/student comparisons (72 pinned runs).
- `slow-fixture2/`: explicitly corrected **companion input**, not fixture change,
  for startup-dominating specialization interaction; all hosts qualified and
  student/reference AST equality checked before three comparisons/profile.
- `inherited-perf/`, `inherited-perf/profiles/`: conditional/replacement/header
  scale comparisons, host-qualified output, three repetitions and separate
  branch/cache/profile passes at CPU4. Historical class/scope profiles remain
  identified by their own binary/affinity manifests.
- `delivery-manifest.json`: final binary/host hashes/environment and counter
  availability checks. Final2/isolated2/slow-fixture2/inherited-perf comprise
  **280 raw perf files/560 events, all 100.00% running**, no missing event treated
  as zero. Exact-slowest running checks also pass.

Final compiler instruction ratios to GCC: ordinary **1.061×**, scopes **1.243×**,
classes **0.247×**, parameters **1.608×**, templates **0.705×**. Final RSS relative
to GCC ≤~1.59× on these inputs; reference/Clang differences are separately kept.
Parameter cost is profiled (647 student/361 GCC accumulated samples, zero lost),
including bounded peek/lookup, arena growth and memory traffic. Templates20k
has 690 student samples (GCC ~1K), zero lost. No unidentified whole-TU scan,
quadratic prefix-copy or global retry remains. Final2 ordinary's 2.94s wall
outlier is preserved; isolated wall 1.58s [1.50,1.65], entry 1.55 [1.48,1.59]
resolves it without a speedup claim. All fixed A/B instruction deltas are
~+0.17–0.19% from correctness repairs; wall/cycle spreads preclude small benefit
claims. Compiler text/data/bss **425172/4128/1568** versus entry
**425056/4128/1568** bytes; no executable-code inference follows.

Inherited conditional preprocessing remains **3.382× GCC instructions** and
~3.85× wall, not hidden by the PA5 family table. Scale21k/85k/340k costs and fresh
263/68 student/GCC samples with zero lost confirm proportional streaming
translation/expansion/evaluation. The previously identified owning indexing,
scratch allocation, paint/compaction and invocation algorithms were fixed in PA4;
their A/B evidence and residual ~3.35× are retained in `pa4/audit.md` and
`pa4/performance.md`. This is the same bounded constant-factor cost, not a new
regression or budget waiver. Further lexer/cursor/arena tuning remains possible;
no identified prohibited ownership or nonlinear algorithm is left unfixed.

Fixtures are millisecond/startup dominated; exact host semantic failures are
preserved, not portable performance passes. The explicit `template<bool B>`
amplified specialization companion has student/GCC instructions 1.802/1.292G,
cycles 0.865/1.000G, wall 0.39/0.44s, RSS 79356/124832 KiB; both hosts qualify.
The course fixture bytes/reference remain unchanged. Generated runtime,
executable instruction/cycle/IPC and native text/object size are unavailable at
PA5, not passed. The **PA33/PA34 per-workload ≤1.25× GCC O2/O3 executable-
instruction gate** remains mandatory; PA34 whole-self runtime is separate.
No numeric target from prior plans was reclassified; every observation remains.

## Final validation, operational ledger and remaining assignment work

- `delivery-fileAudit.log` and post-consolidation `consolidated-fileAudit.log`: exact required audit command, pass, **50 files**.
- `delivery-through.log`, `delivery-gates.exit`: exact required cumulative
  command, exit **0**, **393/393** (188 PA5 + 205 PA1–PA4). The identical
  post-consolidation rerun is `consolidated-through.log` /
  `consolidated-gates.exit` (exit 0); `consolidated-diff-check.log` is clean.
- `delivery-interactions.log`, `delivery-interactions-san.log`: **42** GCC/Clang
  qualified exact reference AST interactions in ordinary and ASan/UBSan builds.
- `final2-{check,scopes,classes,templates}{,-san}.log`: **13/23/30/32** family
  controls and six rejections per group, driver/TU reset; all pass.
- `final2-coverage.log`: all **11** capability probes now pass, including the
  families incomplete at checkpoint 12; major mixed/dependent scope interactions
  added beyond fixture success.
- `final2-rejection{,-san}.log`: **211 prefixes + 7** deferred/context reducers,
  clean success/failure statuses, no sanitizer diagnostics.
- `final-controls.log`, `final2-ownership.json`: **188** final fixtures sanitized,
  including required failures. `delivery-ownership.json`: **223** retained graphs
  (181 accepted fixtures + 42 interactions), ordinary and sanitizer graph tools;
  every node has one owning path, ranges/locations are present, scope parents
  acyclic, shared overlays copy no bindings, dumps stable after parser death.
- `delivery-index.log`: **171529 keys** pass flat-index oracle. Ordinary/class/
  template graph controls and complete-class parameter/default interactions pass.
- Inherited controls: PA1 52 exact/35 rejection cases; PA2 300 encoding/UD,
  150 float and provenance/prefix cases; PA3 **6432** typed oracle/host/reference
  rows plus 39 boundary interactions; PA4 **30** mixed interaction families and
  scaling/performance controls. Logs retained, no inherited control dropped.

Operational errors are preserved, not silently promoted to success: missing
initial baseline binary repaired before measurement; empty-TU root location
assertion corrected in the personal graph harness; executable/directory name
collision corrected; a `tail` inspection option and nonexistent local files
caused inspection-only errors. The initial fixture amplifier host rejection is
retained as invalid portable timing evidence; corrected companion is labeled.
The first exact-slowest script failed parsing `/usr/bin/time`'s nonzero-exit
status line; its original script/raw files remain in `exact-slowest-initial/`,
then the parser used the final numeric line and completed all 72 observations.
Intermediate correctness probe failures were reduced and fixed in the owners;
final ordinary/sanitized runs and required gates pass.

**Ledger:** implementation and independent review are complete through the
review tip. Cohesive repair commit `6e277374`; the accompanying documentation
commit consolidates this audit, compact plan and frozen performance record.
Required gates passed again after consolidation; final commit/tree status
are checked before handoff. No Ralph state/goal file is changed.

**Remaining assignment work, not a PA5 handoff defect:** semantic canonical
identity/deduction/instantiation, effects/alias facts, typed LowIR, ABI/exceptions,
MIR/backend/direct ELF and generated-code/runtime/size certification at their
owning stages. Hosted vendor builtin/type extensions remain the explicitly
classified grammar boundary in `reference-notes.md`, not a required C++11
fixture gap. Retain stage-due controls and later gates, including the PA33/34
instruction gate; constant-factor tuning may not replace missing capabilities
or authorize text transports/replay/global scans.

---

# Historical checkpoint audit 12 (preserved)

# PA5 checkpoint audit 12 — 2026-10-05

## Boundary and disposition

- Stage base / previous review marker:
  `08275a628da86ffd921633806a3f9ca72fcaaaeb`.
- Entry HEAD: `0cdb0e7c` (ten commits after base, three accepted handoffs).
- Reviewed code tip / **Last reviewed commit**:
  `615f033c0a6c4fd106edc7b922173196397a48a3`.
- This first audit covers **base exclusive through reviewed code tip inclusive**,
  not merely the latest handoff. `spec.md`, `pa5/README.md`, `pa5/parsing.md`,
  `TESTING_AND_REFERENCES.md`, the accumulated plan, commit diffs and combined
  source were inspected. The following documentation-only commit establishes
  the next review baseline; it does not change code.
- Checkpoint preservation is verified; **full-stage acceptance is not claimed**.
  PA5 is still 115/188 with the identical 73 failing test identities. Stage-test
  command exit 2 is not a count of two failed cases. Earlier PAs pass 205/205.
  Missing template/dependent grammar remains assignment work, not waived by
  this audit or by fixture success.

All artifact paths below are relative to
`$RALPH_ARTIFACT_DIR/pa5/audit12/`, i.e.
`/home/vishvananda/work/private/v4sol61strands/artifacts/pa5/audit12/`.
Raw evidence is deliberately not committed as generated output.

## Every accumulated commit and cross-handoff review

| Commit | Reviewed responsibility / interaction |
|---|---|
| `722240e3` | Stage boundary and initial parser/validation plan; marker is genuine, not latest-handoff narrowing. |
| `387ff6d8` | Driver/source-set connection, streaming cursor, graph arenas, expression/declaration/statement parser and AST adapter. Shared preprocessing remains own implementation. |
| `da473ec1` | Common-prefix statement factoring; structured qualified names/captures/declarators rather than rewind or rendered lookup keys. |
| `edbe1f60` | Ordinary grammar, literal operators, new/delete ownership and portable/graph controls. Literal borrowing ends before cursor advancement. |
| `5e260a5d` | Ordinary-syntax boundary, remaining-family coverage and raw PMU records; no full-stage completion claim. |
| `12775e55` | Namespace/enum/using environments, qualified category resolution and retained scope identity, integrated with prior ambiguity machinery. |
| `2ab39a96` | Using-directive common-namespace placement and qualifier/namespace-only facts; category overrides and graph survival. |
| `10fc90a7` | Scope-group boundary and cost/remaining work record; inherited controls preserved. |
| `0cfee266` | Class/bases/member/special-member syntax and deferred complete-class regions, including local/nested class queue interactions. |
| `0cdb0e7c` | Compact flat indexes, default-parameter visibility overlay, graph/attribute-literal ownership and performance scaling. |
| `615f033c` | Audit repairs described below plus alias and clean-rejection regressions. |

`range.txt`, `combined.diff`, `commits.diff`, `namespace-commit.diff` retain
inspection inputs. Reviewed interactions include qualification across namespace
aliases and bases, aliases of class definitions/elaborated types, inherited and
hidden type categories, out-of-class operator/special-member declarations,
complete-class defaults/noexcept/member initializers, default-local lambdas,
nested/local class completion, literal retention and omitted attribute views.

## Findings and repairs

1. **Correctness / cross-owner identity loss:** `using alias = struct lower …`
   and `using alias = struct lower;` bind only `resolved_scope`; a class
   specifier instead owns `scope`. Later `alias::word` consequently fails.
   `scopes.cpp:using_declaration` now selects the resolved or owned scope ID.
   Four added portable controls cover class definition, elaborated class,
   scoped-enum definition and alias-chain/base use. Both hosts accept the exact
   source; full AST parity with the reference passes. `probes/results.json`,
   `probes-fixed.log`, `classes-fixed.log` retain the original reducer/result.
2. **Architecture / expected rejection:** `parser.cpp:error` previously eagerly
   rendered and threw `std::runtime_error`. It now stores the first static
   expectation and source location; `require` returns a compact boolean, grammar
   callers terminate on sticky failure, and `parse` returns invalid NodeId 0.
   `peek/take/at` stop cursor access after failure. Only `driver.cpp:emit_ast`
   renders the diagnostic. No recovery scan, exception or abandoned speculative
   tree is introduced. 158 deterministic prefixes and all 188 fixtures under
   ASan/UBSan expose partial scopes/queues without signals, hangs or memory/UB
   reports. Initial experimental regression (112/188) was caught and repaired
   before commitment: nested `if` introduced dangling-else operator parsing;
   final braces restore all controls. Failed experiment logs are retained.
3. **Profile-owned avoidable work:** `tree.cpp:dump` performed one stream sentry
   per indentation depth per node. Before repair, classes20k rendering accounts
   for 23.82% inclusive cycles; `tree.cpp:dump` now writes one reusable,
   maximum-depth-bounded indentation span per node. Exact ASTs unchanged;
   11–24% fewer instructions and 10–19% fewer median cycles across large fixed
   inputs. Final profile rendering is 12.79% inclusive, no longer the same
   per-depth stream-insertion cost. No semantic work or coverage is skipped.

No required reference output, fixture, grammar or harness was revised. Existing
personal reducer proofs in `student.tests/pa5/reference-notes.md` remain intact
(C++11 parameter point-of-declaration/default scope, inherited types, qualified
members, cast/function-style syntax). New `local-default` and lambda-default
observations agree with those existing parameter-shadowing proofs; they are
not used to rewrite checked-in oracles. Reference SHA256 remains
`c0c0f4a9d1b97e611915c4758c2deb57c2f74e944cff7beac3e5ca7523e7b31b`;
bundle source revision remains `0cfcea43c95bfb8a121f7b1e104a016560ebb3db`
(bundle SHA256 `1b7b2863858e7c265417084e758838efbae6cfb788a5d0dec349d255c329a26c`);
the manifest is unchanged. Compiler agreement alone is not treated
as rule proof, and no reference-correction exception was exercised.

## Stage-due capability coverage (not only fixtures)

- `check.py`: integration + 13 ordinary families, six malformed grammar cases,
  driver rejection, TU reset; GCC/Clang qualification and reference AST equality.
- `scopes.py`: integration + 23 namespace/enum/using/qualified category families,
  six rejections. Cyclic/transitive nominations and common-ancestor placement
  preserve ordinary parser category decisions.
- `classes.py`: integration + 30 portable class/member/special/complete-class
  families, six rejections, including four added alias interactions. Existing
  rule-proven personal reference gaps keep explicit assertions, not silent
  parity exclusions. Class/ordinary/scope sanitizer runs pass.
- `probes/`: 14 additional exact-byte host-qualified interaction reducers;
  all now accepted by student. Thirteen reference AST matches; local-default
  disagreement is the documented parameter-shadow issue. Separate nested-class
  and lambda-default reducers retain complete-class interactions.
- `coverage.py`: all 11 inherited/new probes qualified by both hosts. Namespace,
  enum, class and qualified-special pass; template, dependent, non-type,
  template-template, nested-angle, specialization and dependent-conversion fail.
  The reference also rejects dependent conversion; it is not a passing student
  control. `capabilities-final.json` and `coverage-final.log` preserve results.
- `graph-final.log`: 566 nodes, 109 queries, lookahead 3; retained ownership,
  location/literal/name structure and deterministic views after parser death.
  `scope-graph-final.log`: 300 nodes/40 scopes. `class-graph-final.log`: 481 nodes,
  432 tokens, 128 queries, lookahead 3. Flat-index oracle: 171529 distinct keys.
- `sanitized-fixtures.json`: all 188 fixtures; `rejection-san-final.log`: 158
  prefixes. Clean grammar rejection means 0/1 only, not signal exit accepted as
  failure. Full graph sanitizer plus personal controls also pass.

## Architecture audit — explicit owners and stage boundary

Trace: `driver.cpp:emit_ast` constructs TU-local identifiers/preprocessor;
`preprocess/engine/preprocessor.cpp:Preprocessor::next` supplies immutable-source
identities through `preprocess/post/cursor.cpp:PostCursor::next`;
`parser.cpp:declaration/declarator` and `scopes.cpp:using_declaration` construct
structured class/alias facts; `classes.cpp:class_specifier` binds compact scope
IDs and indexed bases; `classes.cpp:finish_bodies` consumes completed-class
regions once; `tree.cpp:dump` is an explicit output view. Alias->class->member
identity remains in the graph, not regenerated from output. Preprocessor source
and identifier owner lifetimes encompass graph consumers in the TU loop.
PA5 has no demanded-template instantiation, LowIR, MIR, mangling or ELF path:
template syntax is still incomplete. A source-to-ELF/demanded-template trace is
**unavailable, not certified**; these owners must be added at their owning PAs.

| Spec invariant | Upholding `file:function` / boundary |
|---|---|
| One parse per region; no parser in semantics/lowering | `parser.cpp:peek`, `ambiguity.cpp:ambiguous_statement`, `scopes.cpp:prepare_name` factor only bounded common prefixes. `classes.cpp:defer_body/defer_expression` collect balanced compact tokens solely for required complete-class regions; `finish_bodies` parses each once, detaches nested queues, releases region storage. No semantics/lowering exists to call the parser. |
| Compact canonical keys; no rendered lookup keys | `preprocess/lex/identifiers.cpp:IdentifierTable::intern`; `index.h:SyntaxIndex::find/insert`; `scopes.cpp:bind/lookup/using_declaration`; `names.cpp:qualified_raw` retain IdentifierId/ScopeId. `tree.cpp:compact` is view-only. Full type/specialization canonicalization is future semantic work. |
| Bounded query work; no TU-wide scans | `scopes.cpp:lookup` indexes the requested name and visits lexical/base/nominated edges only, with query-generation visit stamps. `common_namespace` traverses namespace ancestors only. `index.h:SyntaxIndex::grow` geometric rehash is insertion-owned, not per query; parameter-prefix overlay shares the function index. `parser.cpp:declared_name` walks declarator-local edges only. |
| Cheap filters before deduction/conversion | `scopes.cpp:lookup` filters parameter order, qualifier category and namespace-only before accepting bindings; `parser.cpp:type_start` uses recorded compact category. Deduction/conversion/overload candidates are not implemented or claimed at PA5. |
| Demand-driven instantiation/lowering/emission | `classes.cpp:finish_bodies` processes exactly language-required complete-class regions once. No unrelated semantic emission work is invented. Templates/ABI/emission remain explicitly future owners, not discharged by fixture success. |
| Precise worklists, complete keys, direct typed phase data | `classes.cpp:finish_bodies` detaches the completion queue; no global retry. `scopes.cpp:hint` caches only immutable spelling-derived fallback by TU identifier; no environment-dependent lookup cache. `parser.cpp:error/require/parse` compact expected failure repair. `tree.cpp:node/literal/append` TU arenas and `driver.cpp:emit_ast` TU destruction release them. No text phase roundtrip or copied semantic graph. |
| Allocation/release boundaries | `index.h:SyntaxIndex` flat entries/slots, no per-binding allocation; `tree.cpp:node/append/literal` geometric arena storage; `classes.cpp:finish_bodies` releases deferred tokens at completion. Rendering stacks/indentation are output-only. Source buffers/identifier table and graph are per-TU driver owners, not process-global mutable caches. |
| Counters/timers/slowest profile triggers | `driver.cpp:emit_ast` token/node/edge/query/scope/lookahead and parse/dump counters; `tree.cpp:dump` profile-attributed repair; pinned PMU/raw/profile evidence below. Remaining samples show streaming lexical/parser/output work, not an identified superlinear owner. |

The handout's capitalization fallback (`scopes.cpp:hint`) is a PA5 syntax
contract, not a hardcoded fixture/library-type recognizer. `bind/lookup` override
it with real declaration scope facts. No mandated limit or architecture rule
was reclassified; only absent future-stage machinery is marked not yet applicable.

## PMU protocol, raw evidence and host comparisons

CPU0, Intel Xeon E5-2696 v4; exact CPU/events/compiler versions, flags, affinity,
input/executable SHA256 and section sizes in each `manifest.json`. Final frozen
binary: `2c682b004636e1ca07896e95168bc865211f8b9f87b30c6247a43609518a3ef6`;
455544 file bytes, 390110 text bytes (compiler, not generated program).
Student `--emit-ast --telemetry -o /dev/null`; GCC/Clang
`-x c++ -std=c++11 -fsyntax-only` on **the same bytes**; reference emits its AST.
Host frontends perform semantic work PA5 does not: faster student is not equal
whole-compiler capability. Required AST parity checked before timings.

Warm qualification and pinned warmup precede three interleaved runs per variant;
`perf stat -x ';' -e '{instructions:u,cycles:u}' -- taskset -c 0 ...` inherits
subprocesses. Enabled duration/raw 100.00% running saved in `*.perf`; no scaled,
unavailable or missing counts are reported as zero. `*.time` holds wall/user/
system/RSS; `*.stderr` phase/work telemetry. `raw.json` retains all observations;
`summary.json` includes median/range **for IPC too**, not just ratio of medians.
No cold-cache or runtime speedup claim. Compare timing/RSS separately from work.

Evidence directories:
- `fixed/`, `scopes/`, `classes/`, `parameters/`: inherited fixed input reruns
  before compact-failure/render changes, including 2k/20k/60k observations.
- `final/`: frozen compact-rejection build, pre-render repair PMU and profiles.
- `after-render/`: reviewed code binary, all fixed benchmarks, exact slowest
  candidates, GCC/Clang/reference, branch/cache triples and final profiles.
- `amplified/`: 20k namespace-isolated copies of the host-valid conditional test;
  checked AST parity, repeated counters and separate profile. This dominates
  startup but is **not** mislabeled as an exact fixture measurement.
- `suite-times.json` / `slowest.txt`: three pinned wall observations of every one
  of 188 tests; strongest observed wall candidate is the template/inherited-cast
  test (failure at line 3), strongest successful wall candidate braced-return;
  measured parse-time candidate conditional-associativity. Millisecond wall
  rankings are unstable and startup-dominated; no suite algorithm bottleneck is
  inferred from this ranking. Exact candidates are all measured against hosts.

The exact failing template test's `perf record` has no samples (documented in
`slowest-wall-record.stderr` and report); this is missing sampling evidence,
not successful attribution. Its supported amplified conditional counterpart and
fixed classes/parameters have useful symbolic stacks: post-fix 227/55/82 samples,
zero lost. Profiles use `cycles:u -F 99 --call-graph dwarf,8192`, separately from
comparison runs. Classes: peek 46.67% inclusive/11.90% self; dump 12.79% inclusive;
lookup 3.15% self. Parameters: peek 36.01%, dump 14.46%, lookup 6.05% self.
Sparse unknown kernel-address samples (classes 1.05%, parameters 5.10%) remain
unresolved attribution, not optimization conclusions or user-counter zeros.
No large growing host-relative factor was found in supported work; parameter
student/GCC instructions fall from 1.976x to 1.500x with the output-owner repair.
Additional semantics remain mandatory rather than skipped for measurement.

Separate class branch/cache passes run 100%: branch counts 2183.00–2183.02M,
branch misses 14.94–15.14M; cache references 31.66–36.85M, misses 13.33–13.40M.
CPU-specific cache events are diagnostic, not portable budgets. Query/node work
and 2k->20k retired instructions stay ~10x rather than TU-wide/parameter-count
quadratic. Final ordinary/class work 731.08->7279.13M and 1289.09->12843.95M.

The following representative **final** table shows medians [raw min,max]; M is
millions. Complete four-variant tables and user/system/IPC spreads are in raw
JSON. No numeric prior-plan target is relaxed.

| Workload / compiler | Instructions M [range] | Cycles M [range] | IPC median | Wall s [range] | RSS MiB |
|---|---:|---:|---:|---:|---:|
| ordinary2000 student | 731.08 [731.08,731.10] | 341.97 [336.33,343.86] | 2.14 | 0.15 [0.15,0.16] | 40.2 |
| ordinary2000 gcc | 751.43 [751.35,751.46] | 511.32 [507.52,511.83] | 1.47 | 0.20 [0.20,0.20] | 36.8 |
| ordinary20000 student | 7279.13 [7279.13,7279.25] | 3287.58 [3264.90,3360.20] | 2.21 | 1.29 [1.26,1.49] | 299.4 |
| ordinary20000 gcc | 7354.67 [7353.54,7354.68] | 5155.02 [5086.18,5282.32] | 1.43 | 1.70 [1.70,2.01] | 270.4 |
| extended2000 student | 1642.44 [1642.44,1642.45] | 780.49 [772.94,789.90] | 2.10 | 0.33 [0.33,0.33] | 71.7 |
| extended2000 gcc | 2907.61 [2907.55,2907.65] | 2992.27 [2981.62,3003.06] | 0.97 | 1.09 [1.08,1.10] | 114.4 |
| scopes2000 student | 1188.17 [1188.15,1188.19] | 570.08 [566.03,575.42] | 2.08 | 0.24 [0.23,0.24] | 47.3 |
| scopes2000 gcc | 942.11 [942.10,942.18] | 803.43 [802.41,808.81] | 1.17 | 0.34 [0.34,0.35] | 91.8 |
| scopes20000 student | 11936.83 [11936.53,11936.93] | 5776.31 [5770.61,5863.26] | 2.07 | 2.63 [2.63,2.74] | 557.7 |
| scopes20000 gcc | 10194.17 [10185.56,10197.30] | 8993.25 [8876.42,9128.19] | 1.13 | 3.81 [3.54,3.85] | 768.5 |
| classes2000 student | 1289.09 [1289.09,1289.12] | 629.52 [618.07,630.27] | 2.05 | 0.29 [0.28,0.29] | 78.6 |
| classes2000 gcc | 3910.95 [3910.91,3911.97] | 4322.32 [4319.84,4373.62] | 0.90 | 1.74 [1.73,1.77] | 227.2 |
| classes20000 student | 12843.95 [12843.85,12844.05] | 6136.77 [6060.40,6145.47] | 2.09 | 2.75 [2.45,2.76] | 561.8 |
| classes20000 gcc | 55171.41 [55123.82,55182.69] | 88140.30 [86750.38,90515.25] | 0.63 | 32.01 [30.03,32.26] | 2067.5 |
| parameters60000 student | 2510.62 [2510.62,2510.62] | 1170.75 [1167.71,1181.70] | 2.14 | 0.56 [0.50,0.59] | 169.4 |
| parameters60000 gcc | 1673.26 [1670.77,1675.68] | 813.53 [793.84,816.89] | 2.06 | 0.32 [0.30,0.36] | 119.5 |
| slowest-wall student | 2.57 [2.57,2.57] | 2.66 [2.63,2.67] | 0.96 | 0.00 [0.00,0.00] | 4.3 |
| slowest-wall gcc | 20.15 [20.15,20.15] | 14.75 [14.74,14.85] | 1.37 | 0.00 [0.00,0.00] | 14.7 |
| slowest-passing-wall student | 2.65 [2.65,2.65] | 3.16 [3.15,3.90] | 0.84 | 0.00 [0.00,0.00] | 4.3 |
| slowest-passing-wall gcc | 19.84 [19.84,19.84] | 14.34 [14.31,14.35] | 1.38 | 0.00 [0.00,0.01] | 14.1 |
| slowest-parse student | 2.71 [2.71,2.71] | 4.01 [3.34,4.22] | 0.68 | 0.00 [0.00,0.00] | 4.3 |
| slowest-parse gcc | 19.88 [19.88,19.88] | 14.46 [14.15,14.50] | 1.37 | 0.01 [0.00,0.01] | 13.4 |
| amplified-slowest student | 4621.97 [4621.97,4621.97] | 3181.88 [2087.34,3445.86] | 1.45 | 1.28 [0.91,1.40] | 204.7 |
| amplified-slowest gcc | 4016.90 [4016.18,4017.24] | 2805.35 [2772.21,2836.11] | 1.43 | 1.15 [1.10,1.16] | 259.4 |

Other final same-input comparisons (full spreads in JSON): ordinary20k Clang /
reference 7180.56M/7734.83M instructions, 9682.17M/3214.46M cycles, IPC 0.74/2.41,
wall medians 3.43/1.26s, RSS 135.9/160.6MiB. Classes20k Clang/reference
17221.40M/14930.92M instructions, 31019.01M/6233.90M cycles, IPC 0.56/2.40,
wall 10.58/2.28s, RSS 356.1/271.3MiB. Parameters60k Clang/reference
2091.76M/3653.78M instructions, 1517.66M/1591.93M cycles, IPC 1.38/2.30,
wall 0.49/0.61s, RSS 130.8/112.3MiB. Class student RSS is ~2.07x reference
but ~0.27x GCC; graph identity/arenas are retained, not waived or secretly freed.

Phase/work example, classes20k: 2460000 tokens, 2760001 nodes, 2760000 edges,
1060000 category queries, 460001 scopes, lookahead 3; final representative
parse/dump 2.415/0.293s vs pre-render 2.552/0.540s. Parameters60k: 480022 tokens,
660029 nodes, 240008 queries, 120006 scopes, lookahead 2; final parse/dump
0.498/0.051s. The rejection checks impose a small supported-path cost (~2–3%
instructions before the rendering repair), disclosed in `fixed/` vs `final/`.
Rendering A/B: ordinary/scopes/classes/parameters instructions -16.83/-11.02/
-15.77/-24.05%; corresponding cycle medians -14.54/-9.78/-12.49/-19.41%.
Extended mix -20.22% instructions/-15.24% cycles. RSS effectively unchanged.
Wider pre-fix small-input/ordinary wall and amplified cycles spread prevent
small latency/IPC benefit claims; instruction spreads do not obscure the
substantial removal of rendering work. Amplified workload cycles/latency remain
inconclusive across the observed spread; no threshold has been invented.

No generated executable exists at PA5: generated runtime, executable instructions,
object/text size and native optimization legality/profitability/invalidation/
encoding traces are not measurable yet. Compiler section size is explicitly not
a surrogate for them. The mandatory PA33/PA34 per-workload <=1.25x GCC O2/O3
instruction gate and inherited stage-due controls remain in force. Harness
hang guards were used for checking malformed inputs, **not** as cost budgets.

## Exit checks, ledger and next work

- `make test-pa5`: exit 2, **115/188**, 73 failures exactly matching entry;
  `stage-start.log`, `stage-exit.log`, `progress.json` show zero added failures
  and unchanged 188 coverage. Additional personal passes do not compensate for
  any failed course case.
- Exact prior-through command from the prompt: exit 0, **205/205** (`prior-exit.log`).
- `perl scripts/cppgm_file_audit.pl --stage pa5 --paths dev/src`: exit 0,
  **49 files** (`file-exit.log`).
- Clean build, `git diff --check`, portable controls, retained graph/index,
  sanitizer and repeated same-input PMU qualification verified before code commit.
- Code fixes and expanded controls committed as `615f033c`; plan/audit only are
  committed afterwards. No further code edits after the reviewed tip.

| Audit | Reviewed range (exclusive..inclusive) | Finding / disposition | Validation / remaining boundary |
|---|---|---|---|
| 12, 2026-10-05 | `08275a628da86ffd921633806a3f9ca72fcaaaeb..615f033c0a6c4fd106edc7b922173196397a48a3` | Alias owned-scope loss, exception-based rejection and profile-owned rendering cost fixed; architecture owners cited; no oracle/target relaxation | PA1–4 205/205; PA5 115/188, same 73 failures; 49-file audit; graph/portable/sanitizer controls and repeated host PMU preserved. Template/dependent grammar and full-stage integration remain. |

Remaining work is grouped broadly in `plan.md`: (1) retained template/dependent
syntax/environments and all argument/parameter/operator/specialization/angle
interactions; (2) full-stage integration, hosted/deep controls and fixed host
cost qualification. Do not fragment this into fixture-sized handoffs. Three
accepted handoffs already reworked namespace/default/class ownership boundaries;
that fragmentation was avoidable. No whole-stage completion or advancement is
requested; Ralph reruns external gates and owns advancement.
