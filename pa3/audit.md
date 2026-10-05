# PA3 independent full-stage audit

## Scope and reconstruction

Read `spec.md`, `pa3/README.md`, `AGENTS.md`, `TESTING_AND_REFERENCES.md`, the
PA2 audit, PA3 plan/performance ledger, stage history and the shared source.
Reviewed `3c0beafa`, `e3d2ea6c`, `0d170da6`, `84bdd387` against stage base
`99a5fe4c45fac2edcaef92d5984d07a03cd676ba`; independently reconstructed the
pipeline rather than accepting their checkpoints. This record describes the
final audit changes committed with it. `plan.md` is the compact current plan;
`performance.md` preserves prior observations and the final evidence appendix.

The executable owns immutable `SourceBuffer`, canonical `IdentifierTable`,
streaming `Lexer`, typed `PostCursor`, expression engine and explicit output
renderer (`dev/ppexpr.cpp:main`). The lexer translates phases 1/2 with bounded
lookahead and owns phase-3 literals. `PostCursor` converts literals once, retains
one PP lookahead and physical provenance, and preserves logical newlines in
controlling mode. PA3 facts are already constants: number/character promoted
values, Boolean literals, zero for ordinary identifiers, and the mock-defined
callback result. Explicit precedence stacks reduce these facts while parsing;
there is no separate expression evaluator graph or input replay.

Representative trace: `defined(odd) ? (1u + 2) : (1 / 0)` enters one source
buffer; `Lexer::next` returns canonical `odd` ID and terminal ranges;
`PostCursor::next` supplies typed integer scalars; `parse` queries `defined`
once by ID, records both arms, and checks all grammar/literals. `binary`
rejects division by zero without executing it. `reduce` selects the true arm's
validity, but computes conditional unsignedness from both arms; `result` reads
one surviving fact; `dev/ppexpr.cpp:render` emits `3u`. Reversing the condition
emits `error`. Malformed syntax in either arm still emits `error`. Recovery
continues on the next logical line, but malformed UTF8/literal phase failures
are fatal. Mock `defined` checks parity of the first UTF8 byte, not host macros.

A declaration such as `template<class T> T f(T);` is not a PA3 grammatical
expression and is rejected by `ControllingExpression::parse`. No demanded
template or source-to-ELF trace exists at this stage. The inherited PA2 host
qualification includes declarations/templates to exercise token conversion,
not student declaration semantics or template instantiation. The downstream
architecture obligations remain stage-due, with no substitute host backend.

## Architecture audit — owning evidence for every invariant

All paths below are under `dev/`; future-only surfaces are explicitly separated
from paths exercised by this PA.

| Spec invariant | Owning `file:function` and independent conclusion |
|---|---|
| One parse per source region; no lexer/parser after parsing (§1) | `ppexpr.cpp:main` creates one immutable buffer and one pipeline. `src/preprocess/lex/lexer.cpp:Lexer::translated/next` advances one physical cursor; `src/preprocess/post/cursor.cpp:PostCursor::advance/next` consumes only current/lookahead. `src/preprocess/expr/expression.cpp:ControllingExpression::parse/reduce/result` parses/reduces once; `result` reads one fact and does not relex, reparse or traverse a tree. Invalid-line draining in `next` is forward recovery, not replay. |
| Compact canonical keys; no rendered text lookup/cache keys (§2) | `src/preprocess/lex/identifiers.cpp:IdentifierTable::intern/spelling` assigns dense IDs with hash-indexed canonical entry storage and bulk spelling slabs. `Lexer::finish/next` carries IDs/ranges; `PostCursor::next` retains IDs. `ControllingExpression::ControllingExpression/parse` compares cached `true`/`defined` IDs and calls `DefinedQuery` by ID. `ppexpr.cpp:mock_defined` reads a spelling byte only to implement the required mock. Literal character bytes/number spelling are input conversion, not rendered semantic keys. |
| Bounded per-query work (§3, §4, §9) | `IdentifierTable::intern` uses hash probing and geometric table growth, no whole-TU lookup scan; `spelling` is direct ID access. `ControllingExpression::literal/binary/reduce` have constant work per scalar/operator; `reduce_before/parse` push/pop each operator once, amortized linear in this line. `next` drains at most the current line. `PostCursor::strings` grows only the required adjacent literal sequence. No retry registry, all-declaration scan or growing instruction query exists. |
| Cheap filters before conversion/deduction (§3) | `Lexer::next` filters safe one-character punctuation before Unicode/literal prefix paths; literal dispatch follows first character and bounded maximal munch. `PostCursor::next` selects conversion by token kind; `src/preprocess/post/number.cpp:convert_number` classifies numeric form before parsing it. `ControllingExpression::literal/parse` accepts only allowed integral/character/simple/identifier facts; no deduction/substitution/overload candidate engine is yet due. |
| Demand-driven instantiation/mangling/lowering/emission (§4, §6) | `ppexpr.cpp:main` enables literal facts but disables post-token source rendering (`PostCursor(..., false, true)`); only requested value/error lines are rendered. `PostCursor::strings` builds units only for a valid required literal sequence. `ControllingExpression::reduce` retains only live typed facts. No unused syntax/tree, LowIR, mangling, MIR or ELF is constructed. Instantiation/backend remain N/A until their stages. |
| Precise worklists, complete keys, typed flow and allocation/release (§5, §6, §8) | `ControllingExpression::push_operator/leaf/reduce` use compact indices and contiguous facts/operators, no individually allocated nodes. `next` clears stacks at line boundaries and reuses capacity; invocation destruction releases storage. `PostCursor::advance/character/strings/next` transfers typed scalars/elements plus ranges without a diagnostic text roundtrip, reuses sequence scratch and releases invocation owners. `IdentifierTable::grow/intern` geometrically grows canonical flat storage/slabs. There is no semantic memo cache to have an incomplete invalidation key, no speculative retained abandoned tree and no backend worklist at PA3. |
| Hardware/phase/work counters consulted on slowest workload (§9) | `ppexpr.cpp:main` exposes read/frontend/parse/result/RSS and lexer/intern/reduction/live-stack counters. `ControllingExpression::next/reduce` records parse/result timing and reduction facts. `student.tests/pa3/measure.py` and `audit_measure.py` freeze parity/input/command/host/binary, pin affinity, retain three interleaved PMU runs and independent profiles. Full triple, error floods, chains, lazy/nested and inherited hosted/raw/concat triggers were sampled and their ownership resolved; evidence/limitations below and in `performance.md`. |

The original retained arena/second evaluation walk and expected-error throws
were fixed through their entire expression ownership path, not patched in the
renderer. Typed facts preserve `SourceRange`, signedness and deferred validity;
operators preserve source start and compact base index. Source-bearing scalar
facts coexist with required immutable input, not duplicated source vectors.
No implementation delegates semantics to GCC/Clang/reference; those tools run
only in explicit personal correctness/measurement harnesses.

## Independent capability review beyond fixture success

`student.tests/pa3/audit_check.py` implements an independent typed Python oracle,
not expected values captured from the implementation. 6,432 trees cover all
binary families, unary ops, precedence, associativity, signed/unsigned relations,
conditional result types and nested lazy/domain interactions. Evaluated C++
undefined cases are excluded from portable host evidence. Every portable row is
checked by GCC and Clang `#if`, reference and the student implementation;
`check.py` separately qualifies 1,200 expressions as native C++ static assertions
and PP expressions. Host diagnostic warnings remain in artifacts. 39 explicit
boundary/interaction cases cover course-only promotion and lexical ownership.

Coverage reviewed as families, not a fixture count:
- Numeric PA2 typing/suffix boundaries, decimal-vs-hex/octal candidate selection,
  signed/unsigned 64-bit usual conversions, character widths and `char16_t`/
  `char32_t` course unsignedness. Unicode escape and raw spelling provenance
  continue through the shared lexer/cursor. `literal` sign-extends narrow signed
  representations explicitly; course promotion differences are not compared as
  if native host types were identical.
- Every admitted unary/binary operator; nested parentheses and right-associative
  conditional boundaries; mixed unary/shift/comparison/bitwise/logical nesting;
  no assignment/comma/float/string/UD literal grammar accidentally admitted.
- `&&`, `||`, `?:` skip only domain-error validity in the unchosen branch. Both
  branches' syntax, lexical validity and conditional signedness are checked.
  Error conditions include divisor/modulus zero, signed min/-1 and shift counts
  outside [0,63]. Negative signed div/rem and sign-preserving right shift are
  explicit; unsigned storage prevents host signed-overflow UB.
- `defined x` / `defined(x)`, canonical Unicode names, UTF8-byte mock parity,
  ordinary identifier/true/false behavior, inherited alternative word operators
  including `new/delete`, `and/or/not` and multi-token maximal munch.
- Empty/whitespace/comment logical lines, physical splices/trigraphs, newline
  recovery and EOF without newline; phase errors remain fatal even when an
  earlier expression error has already caused draining of that logical line.
- 100k nested unary/parenthesis/conditional and flat-chain growth; 900k chain
  measured separately. No native recursion and no accumulating facts after
  reductions; limits are genuine index/allocation capacity, not timeout budgets.

Inherited PA1 checks: 52 exact cases, 35 rejections and GCC/Clang qualification.
PA2 checks: ownership/provenance, 25 exact families, 1,617 integer rows, maximal
sequences; independent PA2 adds 300 encoding/UD families, 150 floating cases,
12 prefix conflicts, five raw/provenance controls and three unbounded UD literals.
These exercise shared phase translation, literals, identifier identity and
punctuation affected by the PA3 changes. Clang-built and ASan+UBSan versions of
the final PA3 source pass both PA3 control scripts. The sanitizer checks are
correctness evidence, not part of timing/PMU measurements.

## Optimization legality, profitability, invalidation and budgets

- **Status returns:** only expected expression grammar/domain failure changes
  representation. `parse/reduce/literal/result` propagate status; `next` owns
  line recovery. Lexical/resource exceptions still escape to the top-level
  fatal handler. Invalid-workload profile showed exception unwinding ownership;
  final profile has lexer/parser work instead. 2237.869M -> 402.910M instructions
  and disjoint 0.22–0.23 -> 0.04–0.06s wall ranges justify the change.
- **Typed constant reduction:** every PA3 terminal is constant after literal/
  identifier conversion. `reduce` keeps both-arm type information and lazy
  error validity; `binary` checks domains before executing division or shifts.
  Computing harmless constants in an unchosen branch cannot expose a side
  effect, trap or diagnostic. No cache is shared across lines; `next` clears
  the fact/precedence state, and callback queries are performed for each parsed
  occurrence. PA4 will supply its macro environment through ID/context, not
  reuse cached PA3 defined results. Index overflow is explicitly guarded in
  `push_operator/leaf`; native frames do not grow with source nesting.
  Chain 900k RSS drops 120412 -> 5780 KiB; instruction growth 300k->900k is
  ~2.99x. Logical nodes are telemetry, not retained allocations. Final triples
  need at most three live facts/four operators.
- **Punctuation early filter:** only terminals that cannot start a longer token
  are admitted; `< > : % & | + - / .` and raw/literal prefixes still follow
  multi-character handling. Phase translation runs through `take`, so physical
  provenance and phase-error ownership are unchanged. Inherited maximal-munch
  controls and full fixtures pass. Nested immediate-precursor instructions
  decrease 6.69%, cycles 13.54%; no semantic-result cache/invalidation is added.
- **No inflated claims:** final nested instructions improve versus entry but
  cycles/latency regress (IPC 1.874 vs 2.393); GCC median wall is 0.13s vs 0.32s.
  Profile identifies bounded phase translation/lexer/cursor/parser stacks, not
  replay or global work. Other overlapping latency ranges remain inconclusive.
  Full-triple final wall 0.68–1.10s overlaps entry 0.87–0.97s; reduced
  instructions/cycles do not prove latency improvement in that session. All
  alternate implementations, failed attempts and intermediate sessions remain
  preserved. No prior-plan numeric target was reclassified and no harness
  timeout was used as a budget.

## Frozen hardware-counter evidence and comparisons

Artifact root: `/home/vishvananda/work/private/v4sol61strands/artifacts`.
`pa3-perf/audit-handoff-{manifest,observations,summary,parity}.json` and raw
`audit-handoff-*` freeze final fixed workloads. Final focused data:
`pa3-audit/focused-audit-handoff/` (manifest/observations/summary, frozen inputs,
expected outputs, raw time/PMU files, `*.data`, reports and record logs).
Inherited final comparisons: `pa2-perf/pa3-audit-accepted-*` and
`pa1-audit/pa3-audit-accepted-*`. All older `signoff`, `audit-before`, intermediate
and `audit-accepted` sessions remain, not overwritten by final measurements.

Protocol: Xeon E5-2696 v4, CPU 0, C++11 O3/g student binaries, one warmup then
three interleaved repeats at matching affinity, task-scoped user instructions/
cycles and derived IPC, wall/user/system/peak RSS. Groups reported 100% running;
separate branch/cache passes avoid multiplexing. Record commands use
`cycles:u -F 99 --call-graph dwarf,8192`; zero lost samples and useful O3/g
symbols/stacks. Short after-error sampling is only seven samples, so it supports
broad ownership, not precise percentage claims. Profiling is separate from A/B.
Input/executable hashes, compiler/cc1plus versions/hashes and commands live in
manifests; individual raw observations and min/median/max live in JSON and text.

`pa3-audit/fixture-times.json` independently measured all 100 fixtures; full
`300-triple.t` (~12 MB) dominates suite execution. Investigated the full fixture
and a same-expression portable subset with GCC/Clang, plus error floods,
300k/900k chains, lazy/nested and inherited hosted/raw/concat profiles. Full
triple has course-specific mock/promotion semantics, so reference parity and a
portable same-text host subset are kept distinct. Malformed host diagnostic
recovery exits nonzero and is not an equivalent-output compiler claim. GCC/Clang
#if envelopes retain the exact expressions and expected result lines but do
additional directive work. PA1/2 host lexical and semantic comparisons are
separate; Clang semantically rejects the GCC-expanded hosted headers, retained
as a qualification limitation. These are not claims of PA3 hosted C++ semantics.

See the final appendix in `performance.md` for every median/spread and paired
counter/cache results. Final fixed PA3 I/GCC medians: arithmetic 0.760x, lazy
1.321x, chain 1.710x, nested 1.279x, identifiers 1.373x. Largest fixed median
wall multiple is nested 2.46x; all disproportionate paths have profiles and
bounded work. The flat-chain storage and expected-error algorithms were fixed
before acceptance, rather than excused by fixture success.

Generated-program runtime/emitted text/object size is **N/A**: PA1–3 emit token
or value diagnostics, not ELF. Future fixed loop/call/memory/FP/template/
self-host controls, due O2/O3 budgets and growth controls remain inherited.
Backend host qualification and 8/8 deliberate-failure detection were rerun:
`pa3-audit/backend-{host-qualification,sensitivity}.{json,log}`. Native-quality
PA3 status is `NOT_APPLICABLE` (`backend-pa3-status.json`); instruction checker also exits 0 with `NOT_APPLICABLE`
(`backend-instruction-pa3-status.log`); it deliberately does not create its
output directory before PA33. A subsequent directory listing failed because
that directory does not exist, not because the checker failed. The gate
starts at PA33, not PA3. Mandatory PA33/PA34 **each-workload <=1.25x GCC user
instructions** plus behavior/MIR/debug/whole-self checks remain unchanged.

## Validation and acceptance ledger

Final required rerun logs (after final source and documentation edits):
- `pa3-audit/audit-exit-file-audit.log`: `perl scripts/cppgm_file_audit.pl --stage pa3 --paths dev/src` — PASS, 33 files.
- `pa3-audit/audit-exit-through-pa3.log`: `make test-report-through-pa3` — PASS, 100/100 (PA1–2 80; PA3 20).
- `pa3-audit/audit-exit-personal.log`: all five PA1/PA2/PA3 personal and independent controls — PASS.
- `pa3-audit/{clang,sanitizer}-{controls,audit}-final.log`: final alternate-host and ASan+UBSan PA3 controls — PASS.
- `pa3-audit/audit-exit-source-verification.json`: rebuilding final source at
  identical O3/g flags exactly matches frozen final performance binary SHA256
  `53337f2125fa10d4cbb765549c0b8d09b3b82b39799cdc2b8df16cc1dec322e0`.
- `git diff --check`: PASS; intended implementation/control/documentation changes
  committed cohesively; final `git status --short` empty at handoff.

No fixture/reference/bundle revision was necessary: no reference outputs were
changed, so the reference-correction exception was not exercised. Course-defined
promotions and mock behavior are preserved even where native host semantics
would differ. No coverage or comparison rule weakened.

Resolved inherited handoffs since `84bdd387`: independent whole-stage syntax/
type/lazy interactions, all exercised architecture invariants, inherited control
coverage, final host/PMU spread and dominating workload attribution. Open PA3
correctness/self-containment/architecture/file-audit/timeout defects: none known.
Remaining assignment implementation is PA4+ and its stage-due controls. This
model-owned signoff does not alter Ralph state or advance its goal; Ralph owns
external verification and acceptance.
