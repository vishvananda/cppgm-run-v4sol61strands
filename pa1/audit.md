# PA1 independent whole-stage audit — October 4, 2026

## Scope and reconstructed design / Spec Alignment

Reviewed `spec.md`, the complete PA1 handout, testing/reference policy, all stage
commits (326539d base through 5f1b0c16), source, controls and prior plan. Conclusions
below were reconstructed from code, then tested; prior checkpoint prose was not
used as proof. PA1 implements phases 1–3 and token rendering, not macro expansion,
parsing, semantic instantiation, IR optimization, native generation or ELF emission.
The declaration/template trace is therefore intentionally stopped at typed tokens:
`template<class T> struct box { static const int value = 7; }; int π =
box<::n::size_t>::value;` passes the explicit GCC/Clang qualification and flows from
`dev/pptoken.cpp:main` -> immutable `SourceBuffer` -> `Lexer::next` -> `emit_token`
-> debug view. It does not claim a demanded specialization or an ELF object exists.
Later-stage invariants remain mandatory when those phases are implemented.

`Lexer::decode` validates UTF-8 scalars. `phase1` handles trigraphs and complete
UCNs; `translated` splices and synthesizes the terminal LF. Eight bounded character
slots support maximal munch. `next` owns directive context, comments, headers,
pp-numbers, identifiers, operators and literals. `raw_literal` resumes physical
bytes after the opening quote, validates its <=16-code-point delimiter and scans
linearly before one bulk body copy. Ordinary literals preserve UCN provenance so
encoded punctuation/control characters are data, not lexical delimiters/escapes.
Locations and ranges remain physical byte identities, including spliced input.

The source is TU-owned immutable storage; the caller owns an identifier table with
dense IDs, cached spelling hashes, geometric open-addressed slots/entries and
64-KiB immutable spelling slabs (oversized names get an appropriately sized slab).
The lexer owns only bounded lookahead and largest-token spelling scratch. The
renderer owns a 64-KiB buffer, released at scope exit, and cannot be copied.
EOF and destruction flush the output, including valid records before rejection.
No host compiler/reference is invoked by implementation code. Hosts only qualify
cases or provide benchmark yardsticks; generated text is an explicit view.

## Architecture audit: every PA1 surface

Paths below are relative to `dev/src/` unless explicitly prefixed.

| Spec invariant | Upholding file:function / due-scope conclusion |
|---|---|
| One parse per source region; no late lexer/parser | `preprocess/lex/lexer.cpp:Lexer::next`, `Lexer::translated`, `Lexer::raw_literal`; one streaming lexical traversal, constant bounded lookahead, no parser or later re-lexing phase. Raw physical cursor restoration is bounded lookahead recovery, not TU replay. |
| Compact canonical keys; no rendered lookup/cache keys | `preprocess/lex/identifiers.cpp:IdentifierTable::intern`, `IdentifierTable::spelling`; equality checks original normalized identifier spelling only at interning, then typed tokens carry `IdentifierId`. `preprocess/lex/emit.h:emit_token` is strictly an output adapter. |
| Bounded per-query work, no TU scan in token path | `lexer.cpp:Lexer::peek`, `Lexer::matches`, `Lexer::next`; <=8 lookahead, <=4-char operator checks, fixed grammar inventories. `unicode.cpp:in_ranges` searches a fixed Annex E inventory. `identifiers.cpp:IdentifierTable::intern/grow` uses expected constant probing at <=70% occupancy, cached hashes and amortized geometric rehash, not declaration scans. |
| Cheap filters before expensive candidate work | `lexer.cpp:Lexer::next` dispatches literal/operator families by initial character before bounded matching; `unicode.cpp:identifier_initial/identifier_nondigit` filters ASCII first; `identifiers.cpp:IdentifierTable::intern` checks hash and length before spelling comparison. Deduction/substitution/conversion are not PA1 capabilities. |
| Demand-driven instantiation/mangling/lowering/emission | `lexer.cpp:Lexer::next`, `preprocess/lex/emit.h:emit_token`, `dev/pptoken.cpp:main`; pull only the requested token, render only in explicit tool. No instantiation/mangler/backend exists in PA1, nor fabricated output from those phases. |
| Precise worklists, complete keys, direct typed facts and releases | `lexer.cpp:Lexer::translated/next/finish` retain complete character provenance, directive context, source range and ID. No fixed-point analysis/cache exists to invalidate. `identifiers.cpp:IdentifierTable::intern/grow` keys the full spelling. `lexer.h:SourceBuffer/IdentifierTable/Lexer`, `dev/pptoken.cpp:main` delimit source/TU/scratch lifetimes; `tokens/DebugPPTokenStream.cpp:append/flush/write_token` delimit bounded rendering. No owning per-token node, shared_ptr graph, recursive destruction, textual phase transport or multiple TU token vectors. |
| Slow workloads use counters, phase/work timers and attribution | `dev/pptoken.cpp:main`, `student.tests/pa1/lex_bench.cpp:main`, `student.tests/pa1/measure.py`; raw PMU/time/RSS, bytes/tokens/decoded/spelling/probe/slab counters and sampled stacks listed below. Rendering allocation/formatting owner fixed, not hidden by harness limits. |

Expected hash lookup is not an adversarial worst-case guarantee. 100,000 distinct
sequential names, stable views across rehash, repeated IDs and a 100,000-byte name
pass direct controls; declaration/hosted telemetry records ~1.14/1.17M probes with
80,007/82,871 distinct names and millions of tokens. No growing TU-sized lookup
was observed. 1K/10K/100K mixed-token inputs give exactly proportional decoded,
token and raw-candidate work. Largest scratch grows geometrically, not per token.

## Findings and full ownership-path fixes

1. **UCN origin lost:** encoded quotes/backslashes/newlines could become syntax;
   basic/control UCNs outside literals were accepted. `Character::universal` now
   survives `decode -> phase1 -> translated -> peek/take -> ordinary_literal`.
   `take` applies context-sensitive restrictions on consumption, not speculative
   lookahead; literal UCNs do not form escapes or closing quotes. Encoded LF cannot
   serve as a physical splice. Octal/hex scans stop before UCN-origin data,
   including a UCN that decodes to a digit; a hex escape still requires a
   physical hex digit. Ordinary physical newline still rejects a literal.
   Physical range/column checks cover UCN quote, backslash, LF and embedded NUL.
2. **Raw delimiter exclusion too broad:** generic whitespace rejected CR although
   PA1 d-char excludes SP/HT/VT/FF/LF specifically. `raw_literal` now tests that
   exact course grammar; CR reducer agrees byte-for-byte with reference. Unicode
   delimiter semantics remain the handout's code-point grammar, not a host's
   narrower acceptance policy.
3. **Renderer hot allocation/formatting:** baseline profiles attributed ~51% of
   declaration tool cycles to emission. Converting long token-kind names to
   owning strings allocated per record; ostream sentries/number formatting added
   repeated work. `DebugPPTokenStream::write_token/append/flush` now use nonowning
   fixed kind names, decimal stack scratch and bounded bulk output. Initially
   inline bodies increased tool text to 57,723 bytes; retained that experiment,
   moved the shared renderer to its owning `.cpp`, and registered the source.
   Final text 38,443 vs baseline 41,316 bytes; no duplicated inline formatting.
   Binary spelling, 200K token, buffer-boundary, EOF, destruction without EOF
   and explicit exception-unwind flush controls pass.

### Standard/contract proof and reference discrepancies

No course fixtures, outputs, reference bundle or comparison rules were changed.
Reference bundle source revision is `0cfcea43c95bfb8a121f7b1e104a016560ebb3db`
(`reference-binaries/manifest.tsv`); its original executable hashes are frozen in
measurement manifests. New expectations are personal controls, not substituted
reference answers.

The handout cites WG21 N3485; its C++11 predecessor N3337 has the same relevant
rules: §2.3 [lex.charset]/2 restricts basic/control UCNs **outside** literal
character sequences; §2.14.3 [lex.ccon] includes universal-character-name in c-char,
and §2.14.5 [lex.string] includes it in s-char. Therefore minimal source
`"\u0041"`, `"\u0022"`, `"\u005c"`, `"\u000A"` forms one string token with
respectively A, quote, backslash and LF as data, followed by the closing physical
quote. It cannot be rejected merely because the value is basic/control, or split
by the encoded delimiter. PA1's UTF-8 decoded token-data contract determines the
rendered spelling. Conversely `\u0061` outside a literal is ill-formed, not an
identifier spelled a. Reduced examples are retained in `check.py` and
`ucn-comparisons.json`; GCC/Clang semantic qualification agrees but is corroboration,
not proof. Reference rejects these otherwise valid literal UCN cases. Similarly
`"\\UFFFFFFFF"` is a simple backslash escape followed by ordinary characters under
[lex.ccon]'s simple-escape grammar, not an out-of-range UCN. Raw UCN-looking bytes
remain bytes under [lex.pptoken]/3 and [lex.string].

Primary documents (also retained as PDFs in audit artifacts):
- https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3337.pdf
- https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3485.pdf

## Capability coverage beyond fixture success

- Translation/Unicode: BOM/empty/terminal splice, all trigraph forms, partial UCN,
  invalid scalar/UTF-8, escaped UCN parity, spliced identifiers, Annex E initial
  restrictions and UTF-8 transitions; 52 exact outputs and 35 explicit rejections.
- Context interactions: comment+splice, include+digraph+comment+newline, noninitial
  hash and include-like names, maximal munch/`<::`, pp-number exponent signs,
  ordinary/raw/UD literal suffixes, false raw closing candidates and raw phase
  restoration. 150 deterministic mixed-family sources match reference exactly.
- Identity/lifetimes: physical source mapping, escaped/direct Unicode same ID,
  retained slab bytes across rehash, huge/repeated names and repeated EOF; direct
  GCC and Clang cursor controls plus ASan/UBSan. Renderer tests also use both
  compilers and sanitizers.
- Hosts: portable combined template/declaration source compiles with GCC/Clang
  C++11/trigraph flags. Fixed benchmark semantic-token sequences match GCC and
  Clang, except Clang consumes three unsupported GCC diagnostic pragmas in phase
  4; `clang-hosted-token.diff` records the exact 18 removed tokens. Removing only
  pragma lines on BOTH sides restores identical hosted lexical checksums. Original
  frozen timed inputs were never altered; full GCC/reference parity is preserved.

## Frozen performance evidence and budgets

All artifacts are under `$RALPH_ARTIFACT_DIR/pa1-audit/`; earlier `pa1-perf/` and
`pa1-perf-final/` observations remain intact. Baseline is 5f1b0c16; audit7 is the
final corrected implementation. `*-manifest.json` freezes executable/input hashes,
commands, GCC driver/cc1plus and Clang identity, flags and CPU. CPU 0, Intel Xeon
E5-2696 v4; C++11/O3/-g student binaries, hosts C++11/trigraphs/-E/-P; same physical
input, output to /dev/null, one warmup, seven interleaved runs per workload/variant.
Both use the same environment including metrics. Grouped instructions:u/cycles:u
had 100% running in every observation; raw enabled/running values are retained.
Separate branch/cache pairs also run at 100%. No scaled/multiplexed or unsupported
counter was accepted. `accepted-summary.txt` gives median/range for ALL metrics and
variants; raw `.perf/.time/.telemetry`, observations JSON and rejected experiments
are retained. Latency/RSS are not inferred from instruction count.
Fixed useful work: declarations 7,257,780 bytes / 3,440,001 cursor tokens;
raw 8,468,890 / 480,001; hosted 6,446,630 / 2,936,941 (including whitespace/EOF).
These counts are unchanged across A/B. Median cursor instructions per physical
byte are 345.1/111.5/371.9; GCC is 236.3/65.9/249.1. Individual read/lex or
read/lex-render phase times and intern/probe/slab/raw-candidate counts accompany
every student observation in `.telemetry`; no suite/harness work is charged to
these focused counts.

Final audit7 values: instructions/cycles in billions; IPC; wall median [range] in
seconds; peak RSS median in KiB. Complete spread including IPC/RSS/user/system is
in `accepted-summary.txt`.

| Input / variant | Instructions | Cycles | IPC | Wall [range] | RSS |
|---|---:|---:|---:|---:|---:|
| declarations / cursor | 2.505 | 1.001 | 2.501 | 0.31 [0.29,0.45] | 15,656 |
| declarations / rendered | 3.130 | 1.315 | 2.380 | 0.40 [0.38,0.46] | 15,720 |
| declarations / GCC | 1.715 | 0.611 | 2.806 | 0.19 [0.18,0.21] | 25,660 |
| raw / cursor | 0.945 | 0.357 | 2.648 | 0.14 [0.14,0.16] | 19,172 |
| raw / rendered | 1.037 | 0.402 | 2.582 | 0.16 [0.16,0.18] | 19,240 |
| raw / GCC | 0.558 | 0.254 | 2.196 | 0.11 [0.11,0.13] | 34,844 |
| hosted / cursor | 2.397 | 0.985 | 2.434 | 0.36 [0.35,0.65] | 14,952 |
| hosted / rendered | 3.030 | 1.271 | 2.384 | 0.47 [0.46,0.92] | 15,092 |
| hosted / GCC | 1.606 | 0.603 | 2.661 | 0.23 [0.22,0.47] | 25,660 |

Cursor GCC instruction ratios 1.461/1.691/1.493; rendered 1.825/1.857/1.887.
Rendered B/A instructions -36.57/-19.62/-38.00%; cycles ~-32/-18/-38%; wall
-.20/-.03/-.28s. Rendering does more output work than host -E; compare cursor
separately, not as permission for unlimited rendering cost. Final tool beats the
same-format reference in all three workloads. UCN correctness adds cursor
instructions +1.87/+1.34/+2.27% and cycles ~+6/+5/+4%; disclosed, not waived.
RSS is stable and below hosts; no growing multiple or unresolved architecture
trigger remains. No optimization-level or generated-runtime claim is made.

Initial final/confirm cycles had large outliers despite 100% counter running;
retained all observations, ran seven confirmation repetitions, then measured
compact out-of-line rendering. Final audit7 includes the last escape-scan UCN fix
and again uses seven paired runs. Hosted repetitions exhibited synchronized near
2x cycle/wall spikes across student, baseline, GCC, Clang and reference; fixed
instructions and paired medians preserve the renderer benefit, while these shared
machine effects forbid a precise standalone cursor runtime-regression claim.
Accepted-summary retains full ranges (hosted rendered student cycles
1.245–2.516B, GCC .589–1.255B), no outliers are discarded. Compact/audited sets independently show
similar instruction/cycle reductions. This is diagnostic PA1 acceptance with
bounded work, not a claimed PA33 instruction-gate pass.

Sampling is separate: `baseline-{declarations,raw,hosted}.data/.report` and
`audit7-{declarations,raw,hosted}-{cursor,pptoken}.data/.report`, cycles:u/99Hz,
DWARF 8192, matching affinity, useful symbols/stacks and zero lost samples. Final
profiles repeat the frozen command ten times to obtain useful sample counts, not
as timing observations. Remaining hot owners are bounded lexer peek/take/phase1,
raw scanning, bulk spelling input/copy and renderer write_token, rather than
allocating per record or scanning the TU. Hosted rendered self costs are peek
23.75%, take 12.57%, next 11.50%, write_token 11.12%, matches 8.87%, phase1 4.90%,
intern 4.70%; `.self.report` gives non-overlapping self attribution (DWARF children
may overlap). All six final sample sets contain 116–415 samples and zero lost.
Branch/cache raw passes compare baseline
and compact declarations (three runs each) and are diagnostic CPU-specific events.

All 54 fixture inputs were timed five times; max input is 4,134 bytes
`900-real-world.t`. Process startup dominates, so rankings of tiny single-process
fixtures are inconclusive, not optimization budgets. `slowest-*` retains five
same-input student/GCC/Clang/reference counts/times/RSS. `suite.data/.report` shows
build-triggered GCC work; the separate already-built `steady-suite.data/.report`
attributes harness Perl parsing, not an algorithmically slow lexical fixture.
The fixed multi-MB declarations/raw/hosted workloads replace startup-dominated
fixture timing for the resolved algorithmic review.

Compiler text (not generated code): cursor 33,420 vs 32,551 baseline; tool 38,443
vs 41,316 baseline. `audit7-size.txt` gives final frozen text; `audit7-object-size.txt` gives final
implementation objects. `final-compiler-size.txt` retains earlier objects/tool, and `compiler-size.txt` retains host driver/frontend/reference sizes.
PA1 emits no executable/object; generated-program runtime/text/object comparison
is not executable at this stage and remains due with the backend. The fixed
backend-quality samples, checks and PA33/34 per-workload <=1.25x GCC user-mode
instruction gate are unchanged and mandatory. No previous invented numeric target
was reclassified, no measurement deleted, no timeout treated as a budget.

Legality: transformed spelling/provenance/phase ordering retained, explicit byte
parity and standard-derived exceptions above. Profitability: renderer profile plus
repeatable A/B counters/cycles/latency, memory and text tradeoffs. Invalidation:
source immutable, no lexical cache; `next` clears borrowed scratch, raw entry
invalidates only bounded character lookahead, ID slabs survive rehash. Budgets:
O(bytes + tokens + spelling bytes), expected amortized identifier operations,
O(source + distinct-name storage + largest-token scratch), bounded 8-character
lookahead, <=16 raw delimiter code points and 64-KiB renderer. No fixed-point,
global retry, optimization search or unbounded growth pass exists in PA1.

## Validation / reviewed handoff ledger

Required final `make test-report-through-pa1`: exit 0, 54/54 unchanged tests.
`perl scripts/cppgm_file_audit.pl --stage pa1 --paths dev/src`: exit 0, 23 files.
`make test-pa1` also passes 54/54 (`accepted-test-pa1.log`).
Personal controls 52/35 and host qualification pass; GCC/Clang cursor and renderer,
ASan/UBSan, linear scale, 150 mixed-reference comparisons and diff checks pass.
Logs: `accepted-required.log`, `accepted-file-audit.log`, `accepted-controls.log`,
`accepted-cursor-{g++,clang++}.log`, `accepted-cursor-sanitized.log`,
`accepted-render-{g++,clang++}.log`, `accepted-render-sanitized.log`,
`differential.log`. Earlier assignments: none before PA1.

Audited every previously unaudited handoff since the baseline marker: shared lexer
(34ff2599), phase/EOF/Unicode fixes (4be9f437), raw batch (aee6f19b), flat/slab
interner (4a1c4ffb), speculative escape parity (86a3d0d1), and final measurement
handoff (5f1b0c16). The first two required UCN/CR ownership-path fixes and the
rendering owner required profiling cleanup; the remaining designs hold under
controls and reviewed costs. No open PA1 capability/architecture/correctness issue
or unaudited PA1 handoff remains. PA2–34 assignment implementation remains; no
future stage is certified by this lexical audit. Ralph owns external acceptance
and advancement; its state/goal files were not modified.

Evidence-ledger note: two initial shell wrappers printed passing required-check
output but failed afterwards (`sh: Bad substitution` for Bash PIPESTATUS). Both
checks were rerun with an explicit Bash wrapper and their actual exit 0 was
recorded. Recoverable scratch copy/probe/host-qualification failures and rejected
measurement experiments were not treated as validation; original observations
remain. `accepted-source-hashes.json` ties final source/control files to this
measurement handoff; `accepted-artifact-ledger.tsv` indexes retained evidence.
