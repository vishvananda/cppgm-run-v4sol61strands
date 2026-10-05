# PA4 independent full-stage audit — October 5, 2026

## Scope and Spec Alignment

Reviewed `spec.md`, `pa4/README.md`, `macros.md`, `directives.md`, PA1–3
interfaces, every preprocessing source owner, and stage history independently.
The stage starts at `d257e04fb48b42db12267c2e438ac18dc7dc5e89`; commits
`f3e69900`, `541d46eb`, `ed969186`, `4fe744f1`, `00850578`, and `b923cabe`
were reconstructed rather than accepted as checkpoint conclusions. Historical
plan review markers remain historical, not a claim to have reviewed this audit.

PA4 owns phases 1–6 and phase-7 token conversion, not declarations, templates,
semantics, LowIR, MIR or ELF. The declaration/template-to-ELF trace is therefore
stage-inapplicable; it is **not** certified here. The exercised equivalent trace
is included source → macro argument/replacement → directive expression or
structured post-token cursor. The parser must consume this cursor directly.
Hosted system-header support, executable benchmarks, lowering, optimization
levels and the **PA33/PA34 per-workload ≤1.25x GCC executable instruction gate**
remain mandatory when due. No spec requirement or prior measurement is waived.

### Whole-stage reconstruction

`preproc.cpp:main` creates an identifier table and preprocessor per primary
source. `Preprocessor::Impl::include_file` owns immutable source records and
active file lexers; `raw` pulls one token, applies physical/presumed provenance,
and intercepts source directives. `execute` handles active/inactive conditional
stacks, definitions, includes, line control and pragmas. `MacroEngine::next`
rescans through explicit prescan frames and shared indexed argument spans,
returning one token to `Preprocessor::next`. `PostCursor::next` classifies tokens
and converts retained literal elements. A controlling expression instead uses
`condition` → `OperandSource` → the same `PostCursor` → PA3's typed
`ControllingExpression`; no observation is rendered and reparsed.

Macro definitions are dense ID-indexed slots; undef releases and reuses a slot.
Parameters are compiled to positional IDs at definition time. An invocation
collects raw arguments once, indexes delimiter/comma links once, prescans only
ordinary substituted parameters once, and retains their expanded values only
for that invocation. Stringize and paste require raw arguments. Persistent
32-bit radix paint roots and flat `(root, macro-ID)` extension keys keep
membership independent of nesting depth; explicit frames avoid C++ recursion.
Replacement text can combine with following source tokens to form invocations.
Course nesting, parameter substitution and permanently unavailable tokens
remain distinct. Predefined state, include identity and conditional state are
TU-local, including `_Pragma` invoked from an expansion.

## Architecture audit: exercised invariants and concrete owners

Paths below are relative to `dev/src/preprocess/` unless stated otherwise.

| spec invariant | upholding `file:function` and reviewed ownership boundary |
|---|---|
| One parse per source region; no late phase replay | `lex/lexer.cpp:Lexer::next/translated`, `engine/preprocessor.cpp:Preprocessor::Impl::raw/execute`, `expr/expression.cpp:ControllingExpression::parse/reduce`. No parser/semantic phase exists here. `engine/macros.cpp:MacroEngine::synthetic` lexes only newly generated paste/stringize/builtin spelling; `Preprocessor::Impl::next` lexes destringized `_Pragma`, as required preprocessing grammar, not a production dump. Non-once re-inclusion must process a header again; it is language work, not an optimizer retry. |
| Compact canonical lookup keys, not rendered text | `lex/identifiers.cpp:IdentifierTable::intern/grow` uses slab spellings and flat ID slots; `engine/macros.cpp:MacroEngine::defined/define/undefine` uses dense bindings, positional parameters and reusable definition slots; `MacroEngine::add_paint/grow_extensions` keys immutable roots and IDs. `engine/preprocessor.cpp:IdentitySet::count/insert/grow` keys device/inode; `Preprocessor::Impl::include_file` indexes interned path IDs. Diagnostic/dump rendering is never a cache key. |
| Bounded per-query work; no TU scan inside a token/lookup | `engine/macros.cpp:MacroEngine::painted/insert_paint` has a 32-bit bound; `sequence/arguments/next` index each retained sequence once, then jump across nested arguments; `engine/preprocessor.cpp:compact_attribute_probes` compacts each token once. `include_file` probes path/once identities, rather than scanning prior files. Radix merge/intersection traverses differing required paint subtrees, not unrelated TU state. |
| Cheap filters before expensive demanded work | `engine/macros.cpp:MacroEngine::next` checks token kind, blocked state, dense binding, function-call opening delimiter and paint before collecting/prescanning arguments; `define` precomputes prescan parameters. `Preprocessor::Impl::execute` skips inactive ordinary directives and already-chosen elif expressions; `include_file` checks known once paths before stat/read. Deduction/overload/conversion-candidate filters are later-stage controls. |
| Demand-driven construction and emission | `engine/macros.cpp:MacroEngine::expand_owned/next` streams an already-owned directive operand and indexes only demanded function invocations; `Preprocessor::Impl::condition` runs only demanded active conditions; `post/cursor.cpp:PostCursor::next/strings` does conversion on pull and renders source only for the observation mode; `post/render.cpp:render_posttoken` is an explicit tool view. No TU output vector, AST, native code or unused dump is constructed for the cursor. |
| Precise worklists, complete keys, direct typed facts and release boundaries | `engine/macros.cpp:MacroEngine::Rescan::State/next/substitute` owns contiguous frames, invocation scratch, shared flat sequences and once-expanded parameters. `Sequence` has no owning child sequences, so shared ownership is per required bulk sequence, not per token or recursively destroyed node. `expr/expression.cpp:ControllingExpression::parse/reduce/next` consumes/reduces each operator once into typed facts, not a textual AST. `condition`, `post/cursor.h:PostCursor::reset`, `expr/expression.h:ControllingExpression::restart` reset semantic state while retaining scratch capacity. `Preprocessor::Impl::raw` releases completed lexers; source records deliberately survive until TU destruction for range consumers. `dev/preproc.cpp:main` releases each TU before the next primary. |
| Meaningful locations across all conversion surfaces | `engine/macros.cpp:inherit_origin/synthetic/substitute/next` propagates invocation identity for generated/replacement tokens; raw parameter tokens retain argument identity. `post/cursor.cpp:PostCursor::next/strings` splits only byte-splittable physical tokens and refuses fabricated cross-file/backwards concatenation ranges. `Preprocessor::source_buffer/file_name` keeps physical and presumed names distinct. `student.tests/pa4/origin.cpp` checks actual structured ranges, suffixes, line changes and post-EOF buffer lifetime. |
| Hardware/phase/work telemetry on slowest and disproportionate work | `engine/preprocessor.cpp:record_expression/Preprocessor::Impl::condition`, `engine/macros.cpp:MacroEngine::next/sequence/add_paint`, `expr/expression.cpp:ControllingExpression::next`, `dev/preproc.cpp:main`, and `student.tests/pa4/bench.cpp:main`; frozen PMU comparisons, phase runs, branch/cache passes and symbolized profiles in `performance.md`. Telemetry is explicitly selected by the tool constructor, not an implementation environment shortcut. |

## Findings, changes, legality and budgets

1. **Structured provenance defect fixed across its ownership path.** Object and
   function replacement tokens, one-token alias fast paths, stringize and paste
   inherited some invocation fields but retained physical literal-end/suffix
   fields from definitions or temporary generated buffers. Literal-operator
   splitting could produce backwards ranges and wrong suffix locations while
   every textual fixture passed. `Token::generated` now distinguishes an
   invocation anchor from a byte-splittable physical range; `inherit_origin`
   centralizes the fields, and `PostCursor` honors that distinction. Adjacent
   strings across includes retain the first physical origin instead of mixing
   offsets from different files. No token spelling, payload or paint changes.
2. **Unneeded directive indexing/copies removed.** `expand_owned` streams its
   already-owned input, moves tokens on first pull and reserves output once;
   demanded macro calls alone build delimiter indexes. This is legal because
   lookahead/unread and raw argument collection still use the existing rescan
   machinery; raw-before-prescan, lazy expressions and builtin order are tested.
3. **Allocation ownership tightened.** A macro invocation is inline in its
   explicit frame rather than a heap node; substitution reuses one bulk `part`
   buffer. Conditional cursor/expression scratch is TU-local and reset between
   expressions. There is no semantic memoization: changing macros, line values,
   builtins and defined queries are re-evaluated, with no cached stale values.
   Source/definition/argument buffers retain only required deferred behavior.
4. **Pragma-once identity storage made flat.** The former owning hash node per
   header violates the dominant-map ownership requirement. Device/inode equality
   is unchanged; geometric open addressing replaces nodes. Path fast checks,
   hardlink aliases, many distinct headers, ordinary re-includes and TU reset
   were rechecked. Flat storage is released with the TU.
5. **Rejected experiments retained.** A lexer ring-buffer trial increased
   instructions. An ASCII fast path reduced instructions but raised cycles/low-
   IPC work on several families (notably text/paste); it was removed before
   acceptance. Counts alone are not profitability. Correctness-required
   provenance bookkeeping remains despite its small paste/alias instruction
   increase; no timing improvement is claimed for it.

Work budgets are the spec's linear/near-linear consumed/produced-work model,
not harness timeouts. Paint operations have fixed ID-depth bounds; indexing and
probe compaction are linear; substitution is proportional to actual emitted
replacement tokens; repeated ordinary inclusion is proportional to required
source processing. Buffers grow geometrically, active frames scale with actual
nesting, and prescan executes once per demanded argument. No arbitrary expansion
cap discards required output. Existing 100k-depth controls and 1k/4k/16k probes
remain; new 21k/85k/340k conditions, 1k/4k/16k replacement lengths and 2k/8k
headers verify scaling. No earlier numeric target has been reclassified.

## Capability review beyond fixtures and oracle boundaries

`student.tests/pa4/audit_check.py` has **30** independent interaction families /
random batches. It compares GCC/Clang/reference on their common supported domain
and uses explicit course results at the documented boundary: nested rescan and
paint, redefine, empty/variadic/stringize/paste, lazy mixed-signed conditional
expressions, inactive invalid directives, predefined prescan and counter order,
line control, pragma origin, same-inode aliases, ordinary headers, include chains,
multiple primary reset, raw/pasted literal tokens, adjacent cross-file strings,
rejections, 300 headers and deterministic combined macro/conditional arithmetic.
`check.py` retains its portable qualification, location and extreme-scale controls.
`origin.cpp` adds direct structured assertions; dumps cannot substitute for them.

### Reduced reference discrepancy; no course reference correction

Reducer (also an explicit independent control): primary `input.cc` contains
`#line 80 "virtual/input.cc"` then `#include "next.hh"`; only
`virtual/next.hh` exists and contains `19`. Required observation is integer 19.
`pa4/directives.md`, “Source File Inclusion and __FILE__”, requires trying the
presumed-file directory first, then cwd; “Line Control” explicitly says changing
`__FILE__` impacts future include behavior. Thus the reduced course-contract
proof is independent of compiler agreement. The bundled reference instead exits
with `included file not found: next.hh at virtual/input.cc:81:2`.
Hosts ordinarily search by physical include origin, so are **not** oracles here.
A broader `_Pragma`/line/alias interaction also records this discrepancy.

Bundle source revision: `0cfcea43c95bfb8a121f7b1e104a016560ebb3db`;
reference preproc binary SHA256:
`d4f0d481a747930219faecb600de0398669adfa5ce86c6019ff64cb239726c2f`.
No course reference, bundle, normalization or comparison rule was modified.
This is a documented supplemental oracle boundary, not a fixture relaxation or
use of the permission to correct reference outputs. Likewise course paint is
proved from `pa4/macros.md` nesting/parameter/unavailable rules, not copied from
GCC when they differ. Inputs and all raw observations/expected values/companion
files are retained in the semantic ledger under `$RALPH_ARTIFACT_DIR/pa4-audit/`.

## Performance, validation and handoff ledger

`performance.md` consolidates all frozen trials and final same-input host
comparisons. Accepted final label: `pa4-perf/audit-certified`; scaled controls,
all fixed-family samples, the slowest fixture and separate branch/cache passes:
`pa4-audit/conservative-complete`. An identical-binary delivery rerun is retained
as `pa4-perf/audit-delivery` and `pa4-audit/delivery-complete`, including its
text-cycle outlier and overlapping wall ranges (see `performance.md`).
All measured PMU events ran at 100%; repeats,
ranges and noisy observations are preserved. Conditional work falls about 4.9%
in instructions with repeatable cycle benefit on the principal 85k case;
wall ranges overlap, so no latency speedup is certified. Remaining ~3.35x GCC
conditional instructions/~4x wall and ~3.08x 16k-probe instructions are disclosed,
profiled and scale linearly; the owning extra indexing/allocation algorithms
were fixed, not excused by fixture timeouts. Generated executable runtime/text
and object size are N/A at PA4. Compiler binary text is measured separately.

Final validation on the accepted source:
- `perl scripts/cppgm_file_audit.pl --stage pa4 --paths dev/src`: **pass**, 36 files.
- `make test-pa4`: **105/105**, exit 0.
- `make test-report-through-pa4`: **205/205**, exit 0; required reports ran serially.
- PA4 personal, 30 independent interactions, GCC/Clang portable qualification,
  Clang-built implementation, ASan+UBSan and structured origin controls pass.
- Inherited PA1: 52 exact cases/35 rejections and structured cursor; PA2:
  25 families/1,617 integer cases; PA3: 100k scale/1,200 qualified cases pass.
  Independent PA2: 300 encoding/UD families, 150 floats, 12 prefix conflicts,
  five raw/provenance cases, three unbounded UD literals; PA3: 6,432 typed
  oracle/host/reference rows and 39 boundaries/interactions pass.
- Logs: `pa4-audit/final-file-audit.log`, `final-pa4.log`,
  `final-through-pa4.log`, `final-independent.log`, `conservative-*.log`.
- Post-consolidation delivery validation repeats all required checks: file audit
  36 files, PA4 105/105, cumulative 205/205, independent 30 families, personal
  behavior/scale/host qualification and structured-origin controls, all exit 0.
  Logs: `pa4-audit/delivery-file-audit.log`, `delivery-pa4.log`,
  `delivery-through-pa4.log`, `delivery-independent.log`,
  `delivery-controls.log`, `delivery-origin.log`. Final semantic ledger:
  `semantic-observations-a57835bbe175-1.json`. Python audit controls compile.

All four inherited unaudited handoffs are closed: broader paint interactions,
raw/pasted provenance (defect fixed), many-header once/path behavior (flat owner
fixed and scaled), and conditional capability/cost (independent interactions,
scaling, telemetry and profiles; redundant indexing/scratch fixed). No known
unfinished PA4 contract group, correctness, architecture, self-containment or
timeout defect remains. Remaining lexer/conversion tuning is a disclosed bounded
constant-factor opportunity, not a silently waived acceptance target. Later
assignment capabilities remain due at their stages. This commit is the model-
owned audit handoff; Ralph owns external acceptance and advancement.
