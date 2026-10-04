# PA2 independent whole-stage audit — October 4, 2026

## Final design / Spec Alignment

Stage base `4ffe0189`; independently reviewed implementation commits `a84e1c18`,
`8d813db5`, `23013af0`, and final code/control commit `83ca4ff2`. Read the complete
`spec.md`, PA2 handout, testing/reference policy, PA1 audit, stage diffs, source,
and plan. Checkpoint conclusions were not used as architecture proof. PA2 owns
phases 1–3, phase-4 no-op under the handout restrictions, literal conversion and
phase-6 maximal concatenation, and token classification. Macro expansion,
parser/semantics/templates, LowIR, optimizers, native generation and ELF are not
implemented at PA2; no successful native compiler or executable gate is claimed.

Reconstructed dataflow:
`dev/posttoken.cpp:main` -> immutable `SourceBuffer` + TU `IdentifierTable` ->
`Lexer::next` -> `PostCursor::next` -> typed `PostToken` -> explicit
`render_posttoken` view. `Lexer::decode/phase1/translated` validate UTF-8, apply
trigraphs/UCNs/splices and retain physical locations; eight bounded lookahead
slots support maximal munch. `ordinary_literal` decodes elements once with
UCN-vs-numeric provenance; `raw_literal` resumes original physical bytes and
undoes phase-1/2 lookahead inside quotes. Collection is opt-in, so PA1 does not
construct unused element vectors. PostCursor skips whitespace, retains one PP
lookahead, classifies a fixed metadata inventory, validates numeric grammar,
selects LP64 integer candidates, and encodes characters/strings. String encoding
is selected from the entire maximal sequence before encoding its elements;
conflicting encodings/suffixes produce one invalid and consume the entire sequence.

Numeric scalar bytes and enum types are inline. Identifier/suffix identities are
dense TU IDs, with immutable spelling slabs and geometric open-addressed slots.
Array/raw-UD bytes are current-cursor borrowed payloads, explicitly invalidated
by the next operation; source rendering is opt-in. String scratch is retained
only to resolve phase-6 deferred encoding, not a complete TU token vector.
`convert_number` uses allocation-free libc scans for typed floats. PA2-required
starter stream scans occur only in `render_posttoken` for compatibility, never
as production data transport. Debug hex/source strings are explicit outputs.

Concrete trace (retained at `artifacts/pa2-audit/declaration-template-trace.cpp`):
`template<class T> struct box { static constexpr char16_t value = u'π'; };
const char16_t s[] = "\x3c0" u"😀"; box<int> b;` flows through source offsets,
interned `box`/`T`/`value` IDs, fixed keyword enums, typed `char16_t` scalar π,
then phase-6 char16_t array `[0x3c0,0xd83d,0xde00,0]`. GCC/Clang qualify the
same C++11 declaration and the reference token view agrees. The trace stops at
typed tokens: no demanded specialization, lowering or ELF is invented.

## Architecture audit: every exercised surface

Paths below are relative to `dev/src/` unless explicitly prefixed.

| Required invariant | Upholding file:function and due-scope assessment |
|---|---|
| One parse per source region; no later lexer/parser | `preprocess/lex/lexer.cpp:Lexer::next`, `Lexer::ordinary_literal`, `Lexer::raw_literal`; `preprocess/post/cursor.cpp:PostCursor::advance/next/strings/character` consumes already decoded elements. `preprocess/post/number.cpp:convert_number` validates PP-number grammar once and chooses its value/type. No literal spelling reparse in production. `preprocess/post/render.cpp:render_posttoken` explicitly owns the mandated float debug scan only. No language parser exists yet. |
| Compact canonical keys; no rendered keys | `preprocess/lex/identifiers.cpp:IdentifierTable::intern/grow/spelling` interns entry spellings into stable IDs; `PostCursor::strings` compares suffix IDs; `preprocess/post/types.cpp:classify_simple` uses immutable bounded language metadata, not TU semantic text keys. `convert_number` produces enum type and inline bytes. Raw UD prefix text is payload, never identity/cache key. |
| Bounded per-query work; no hot whole-TU scans | `Lexer::peek/matches/next` bounds lookahead and first-character grammar dispatch; `preprocess/lex/unicode.cpp:identifier_initial/identifier_nondigit` queries fixed Annex-E ranges; `IdentifierTable::intern` has amortized geometric growth and bounded-load hashing; `convert_number` is linear in spelling; `PostCursor::strings` visits only the maximal sequence; `cursor.cpp:encode/unit` is bounded per element. No declaration/scoping query exists yet. |
| Cheap filters before expensive work | `Lexer::next` filters literal/punctuator/comment candidates by first character; `convert_number` validates grammar/suffix before scanning scalar values; `PostCursor::character` checks count/scalar/range before conversion; `PostCursor::strings` checks encoding/suffix consistency before code-unit allocation/encoding. No deduction/substitution/overload phase yet. |
| Demand-driven construction/emission | `dev/posttoken.cpp:main` enables only PA2 literal collection; `dev/pptoken.cpp:main` leaves it disabled; `PostCursor::next` converts only requested next token/sequence; render-source retention is opt-in. Instantiation/mangling/IR/ELF emission remain later-stage obligations. |
| Precise worklists, full keys, direct typed transport, release boundaries | `PostCursor::advance/next` owns one lookahead and one pending literal-operator identifier; no retries/fixed points/cache invalidation machinery needed. `Lexer::ordinary_literal` passes typed `LiteralElement` provenance to `PostCursor::strings/character`; `convert_number` fills typed scalar storage; `IdentifierTable::grow` preserves IDs/slab pointers. `SourceBuffer`, `IdentifierTable`, `Lexer`, `PostCursor` and local output scratch are RAII-owned in `dev/posttoken.cpp:main`. Borrowed lifetime contracts are in `post/types.h:PostToken` and `post/cursor.h:PostCursor`; geometric scratch is released at TU exit. |
| Hardware/phase/work counters consulted on every trigger | `dev/posttoken.cpp:main`, `student.tests/pa2/bench.cpp:main`, `Lexer::finish` and `PostCursor::strings/next` expose input/convert times, tokens/elements/probes/bytes/maximum sequence and peak RSS. `student.tests/pa2/measure.py` and `audit_measure.py` retain fixed host comparisons; five workload profiles plus slowest-fixture profile are attributed below. Telemetry omission on the split literal-operator path is fixed in `PostCursor::next` and tested in `student.tests/pa2/cursor.cpp:main`. |

Self-containment: `dev/posttoken.cpp:main` and `dev/pptoken.cpp:main` call shared
C++ implementations only. `dev/frontend_source_sets.mk` registers those owners.
Reference/host processes occur only in tests, measurement scripts and wrappers'
explicit reference mode. `support/testing/test_runner.cpp:main` routes explicit
test input through `test_runner_real_main`, not an output oracle. Fixed global
metadata is read-only; no mutable process-global TU cache, hidden retry, text
roundtrip, fixture recognition or per-element heap-node ownership was found.

## Capability review, findings and changes

Independently exercised classification/invalid continuation; decimal/octal/hex
candidate order and limits; floating grammar, exponent boundaries and suffixes;
raw UD numeric prefixes beyond builtin range; ASCII/non-ASCII character rules;
all five string encodings, surrogate pairs and embedded numeric code units;
UCN delimiter/backslash provenance; raw undoing of transformations; compatible
and conflicting suffixes/encodings; whole maximal-sequence rejection/recovery;
comments/splices/physical ranges; and literal-operator suffix splitting.

Inherited `check.py` has 25 exact families, 1,617 independently computed integer
candidate-table cases, 20,000-piece valid/invalid sequences, rejections and typed
ownership checks. New `audit_check.py` adds 300 seeded scalar/numeric-code-unit
encoding/UD combinations with prefix order reversal, 150 exactly representable
float byte cases, 12 encoding conflicts with recovery, 5 raw/provenance cases,
and three 8,192-digit UD numeric spellings. All four comparison bundles match
`posttoken-ref` exactly. GCC and Clang execute the same portable string/float
payloads against independent Python encoding/packing oracles; course-defined
character/invalid rules use the handout, not compiler majority. Both host-built
and ASan/UBSan PA2 tools pass inherited and independent controls; Clang build is
warning-free. Typed cursor controls pass both host builds.

Findings resolved:
1. Profiles exposed unnecessary comment probes on every token and the `<::`
   probe on every punctuation candidate. `Lexer::next` now checks '/'/'<' first
   across the shared PA1/PA2 ownership path. Legality: the first decoded character
   must match the first grammar character; the unchanged phase cursor and maximal
   munch rules still decide all feasible alternatives. No cache keys or facts are
   changed; no invalidation/retry/fallback budget is introduced. Profitability is
   measured below. Complexity stays O(bytes/tokens) with constant grammar bounds.
2. Split `operator""suffix` returned a one-byte empty array without counting its
   sequence/converted bytes. `PostCursor::next` now records both; a physical-splice
   cursor test asserts 1 sequence, 1 converted byte and 0 elements. Token behavior
   is unchanged; telemetry cannot select semantics.

No reference outputs, supplied fixtures, required behavior or comparison rules
were modified. No numeric target was reclassified. Spec-mandated rules and the
PA33/PA34 gate remain intact. No other stage-due correctness/ownership/architecture
violation remained after these controls and manual full-path review.

## Performance evidence and resolution

Artifact root **`/home/vishvananda/work/private/v4sol61strands/artifacts/`**.
`pa2-perf/{audit,audit-filter,audit-final,audit-confirm,audit-signoff}-*` retain
all manifests, executable/input hashes, host versions, commands, raw `.perf`,
`.time`, `.telemetry`, repeated observations, output checksums and summaries.
Historical `{initial,dispatch,final,accepted}` data remain untouched. Signoff
binaries correspond to `83ca4ff2`. Xeon E5-2696 v4 @2.20GHz, CPU 0 affinity;
C++11/O3/g implementation builds; matching C++11/trigraph inputs for hosts,
GCC/Clang `-fsyntax-only` and `-E -P`, no claimed exact phase-equivalence.
One warmup, three interleaved measurements/variant; primary simultaneous
`{instructions:u,cycles:u}` group inherited through compiler subprocesses.
Kernel/off-CPU work is excluded from PMU counts but raw wall/user/system/RSS is
preserved. `pa2-audit/counter-availability.json` verifies 1,720 event observations
in the audit/final comparisons and diagnostics: all 100% running, no missing
counts; full time-running fields are retained. No scaled-count benefit claim.

Five fixed inputs: 80k declaration namespaces; 40k adversarial raw strings;
frozen GCC-expanded hosted TU with phase-4 directives removed; 40k literal-rich
namespaces; 1,000 arrays of 501 concatenated strings. Typed payload checksums
are stable across A/B. Four supported benchmark debug views match reference
exactly. Hosted differences are retained in `audit-signoff-parity.json`: GCC
extensions (`__int128`, `__typeof`, `__typeof__`, `0.0bf16`) are not C++11 PA2
keywords/literals. No output is normalized; Clang rejects GCC-only hosted
semantic extensions, so that semantic comparison is explicitly inconclusive.
Clang preprocessing still appears in the measurement ledger.

`performance.md` contains the full signoff medians/ranges with GCC/Clang/reference
on every same input. Cursor instruction/GCC semantic ratios are approximately
0.30/0.74/0.23/0.46/1.79; against GCC preprocessing 1.68/2.21/1.70/1.86/1.98.
Semantic hosts do more than PA2; preprocessing does less (no phase-5/6 typed
encoding). Compared with matching PA2 reference work the renderer ratios are
1.08/0.76/1.07/1.08/1.02, a bounded constant cost. RSS is 11–19 MiB versus GCC
semantic 66–731 MiB and GCC preprocessing 18–34 MiB. This is not a claim of
final full-compiler parity, nor a waiver based on harness timeouts.

A/B cursor instructions improve declarations/raw/hosted/literals/concat by
7.54%/2.51%/8.96%/5.44%/2.87%; renderer by
5.09%/1.97%/6.11%/3.73%/2.73%. Signoff cycles improve respectively
5.95%/1.68%/11.42%/2.35%, but concat is +0.67%; renderer cycles improve
7.09%/2.66%/7.56%/3.11%, concat +0.46%. Earlier serial confirm showed concat
-1.44%/-0.52%; ranges overlap, so concat cycle benefit is **inconclusive**, not
claimed. Regressions and all observations are preserved. The `audit-final`
declaration cursor cycle median 1.735B [1.274B,1.773B] was an outlier while PA1
measurements ran concurrently; retained rather than discarded. Serial confirm
1.269B [1.269B,1.285B] and signoff 1.292B restore comparable stable evidence.
Literal renderer confirm outlier 1.417B and overlapping wall-time regressions
are retained; no blanket latency improvement claim. Half/full instruction ratios
2.003/1.996 for declarations/concat, with GCC on those same half inputs in
`pa2-audit/scale-{manifest,observations}.json`, confirm linear work. Max sequence
501, 501k elements and 1.006M converted bytes; no accumulating token stream.

Profile before/after: `pa2-audit/{declarations,hosted,concat,raw,literals}-`
`{profile-repeated,final-profile,signoff-profile}.txt` and raw `.data`, commands
in `*-profile-manifest.json`. Sampling uses `perf record -e cycles:u -F 99
--call-graph dwarf,8192`, twelve separate runs to obtain useful stacks, outside
measured comparisons. Before samples 527/515/189/207/277; signoff
451/421/183/218/284, all zero lost. Before owners included bounded grammar
`Lexer::matches`; shared filter fixes that cost. Signoff declaration/hosted are
mostly `Lexer::peek/next/take`, classification and PostCursor dispatch; concat
`ordinary_literal` 30.63%, `take` 18.02%, `peek` 11.03%, `phase1` 7.05%,
`PostCursor::strings` 6.87%; raw `raw_literal` 21.85%/decode 10.69%. Useful
symbol/call stacks reach main and typed cursor, not allocator-dominated per-node
work. Additional concat branch/cache groups: 213.087M branches, 31,372 misses;
244,711 cache references, 3,405 misses, 100% running. CPU-specific diagnostics,
not portable numeric limits. Remaining ordinary decoding is required capability
work and is linear; no disproportionate owning algorithm remains unresolved.

### Slowest fixtures and inherited controls

Every PA1/PA2 fixture was timed three times against its reference;
`pa2-audit/fixture-observations.json` retains all observations. PA1 fixtures are
startup-scale (~1.5–3ms); expanded fixed workloads avoid claiming improvements
from their noise. PA2 `700-hard-string-concat.t` (429,984 bytes) is the dominant
fixture: initial wall 22.4–23.1ms versus reference 17.3–17.5ms. Explicit repeated
pinned measurements and profiles in `pa2-audit/slowest*` compare cursor,
renderer, reference, GCC and Clang preprocessing on **the same fixture**. The
fixture deliberately contains ill-formed token cases; Clang -E rejects suffixes
and its timings are labeled non-equivalent diagnostics, never a passing host
oracle. `slowest-signoff` renderer retires 167.77M instructions vs reference
~146M, an improvement from initial ~174M; cursor 130.43M vs GCC -E 91.99M.
Raw ranges/times/RSS/IPC appear in the ledger. Thirty-execution sampled profile
has useful lexical/render stacks, zero lost; output rendering remains explicit,
not a production-phase representation. Longer concat benchmark supplies reliable
counter spread where /usr/bin/time's 10ms resolution is too coarse for a fixture.

Inherited PA1 performance controls rerun against its frozen audited binary:
`pa1-audit/pa2-audit-final-*` retains all observations/outputs/manifests;
`pa2-audit/inherited-pa1-final-measure.log` reports median/range. Cursor
instructions deltas declarations/raw/hosted -8.70%/-0.37%/-10.53%, cycles
-7.53%/-0.50%/-10.36%; pptoken instructions -6.94%/-0.41%/-8.33%, cycles
-9.18%/-0.13%/-8.88%. Full reference output equality remains passing. No
inherited-control handoff was dropped during the shared lexer changes.

### Runtime and size / later mandatory gates

PA2 produces token views, not native objects/executables. Generated-program
runtime, text/object size, loop/call/memory/FP/self-hosting executable benchmarks
are **N/A at this stage**, not zero and not inferred from token counts/output
bytes. `pa2-audit/compiler-size.json` separately records actual compiler text:
cursor 74,305 -> 74,113 bytes; posttoken 73,499 -> 73,307 bytes (minor telemetry
code/layout variation), with file sizes/hashes retained. No generated-code claim.
The fixed backend workloads in `student.tests/backend-quality/check_instructions.py`
remain mandatory at PA33 and PA34: every O2/O3 workload <=1.25x GCC retired
user-mode instructions, paired protocol, missing/unstable evidence non-passing.
PA24/26 call/EH location facts, PA32 bounded inlining/dataflow and serialized
facts, PA33 allocation/scheduling/final cleanup and PA34 whole-self runtime must
be independently audited then. Early frontend comparisons do not satisfy or
reclassify those gates. There are no PA2 optimizer transforms to claim legality/
profitability/invalidation budgets for beyond the lexical filter reviewed above.

## Validation / ledger / handoff

- Required `perl scripts/cppgm_file_audit.pl --stage pa2 --paths dev/src`: pass,
  31 files; `pa2-audit/final-file-audit.log` (also earlier signoff pass).
- Required `make test-report-through-pa2`: exit 0, **80/80** (PA1 54, PA2 26);
  `pa2-audit/final-through.log`. Incoming Ralph primary log was independently
  read but not reused as proof of this run.
- PA1 and PA2 explicit inherited controls, expanded independent controls and typed
  provenance/physical-range tests pass; `final-{pa1,pa2,independent}-controls.log` (earlier signoff runs retained).
- Clang-built tool passes both control suites, warnings absent; GCC ASan+UBSan
  passes both; `clang-signoff-*`, `sanitizer-signoff-*`; typed cursor both hosts.
- Python syntax checks and `git diff --check` pass. Changes are cohesive and
  committed; audit documentation final commit checked separately at handoff.

Unaudited handoffs since PA1: PA2 provenance collection, typed post cursor,
numeric type selection, UD raw payload retention, phase-6 lifetime, float debug
adapter separation, physical literal-suffix location, dispatch optimization and
CPU metadata correction are now independently reviewed and tested above. No
PA2 audit item remains unaudited. Remaining assignment work is PA3 onward,
including the deferred compiler/executable surfaces and their inherited controls;
it is not a PA2 blocker. Ralph retains authority to accept/advance the goal.
