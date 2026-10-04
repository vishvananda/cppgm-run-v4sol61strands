# PA1 implementation handoff

Stage base commit: 326539d7382056c8816d77ad7c4e85acb7de6653
Last reviewed commit: 326539d7382056c8816d77ad7c4e85acb7de6653

## Design / ownership / validation
- Baseline 0/54; implemented all PA1 behavior, now 54/54 unchanged fixtures.
- `preprocess/lex/lexer.cpp:translated`: immutable UTF-8 buffer -> bounded
  character cursor; scalar validation, UCNs/trigraphs, splice/final LF. Tokens
  retain physical byte offsets/line/column, not a normalized whole-TU copy.
- `Lexer::next`: typed pull tokens with kind/range/identifier ID. Borrowed spelling
  scratch lives until next(); legacy adapter is rendering only. No owning token
  vectors, syntax trees, textual cross-phase transport or whole-TU per-token scan.
  Whitespace/comments, pp-numbers, operators and directive headers share context.
- `Lexer::raw_literal`/`ordinary_literal`: raw physical bytes restore phase 1/2;
  bounded <=16 delimiter code points, linear body scan, one batched spelling copy.
  Validate ordinary escapes, character content, UD suffixes and partial input.
- `unicode.cpp`: bounded Annex E binary range checks; UTF-8 encoding.
- `identifiers.cpp:IdentifierTable::intern`: dense open-addressed ID slots,
  cached hashes, geometrically growing entries, bulk immutable spelling slabs.
  Stable byte views across rehash; copying interner forbidden. TU-owned release.
- Work model O(bytes + tokens + spelling bytes), expected constant intern lookup,
  amortized linear rehash, memory O(source + distinct names + largest token).
  Telemetry: bytes/decoded/tokens/spellings/raw candidates/probes/slabs/times/RSS.
- Explicit personal controls: 39 exact outputs, 24 rejections; GCC/Clang portable
  qualification, typed cursor/locations/identity/Unicode/1k-100k scale checks,
  GCC ASan+UBSan and Clang cursor pass. Required make/test-through and file audit
  (22 files) pass; prior stages vacuous 0/0. See student.tests/pa1/README.md.

## Performance evidence
Frozen PMU/timing artifacts under `$RALPH_ARTIFACT_DIR/pa1-perf{,-final}`; CPU 0,
Xeon E5-2696 v4, O3 -g, warmup then 5 interleaved runs, instructions/cycles
100% running. Same-input GCC 15.2 / Clang 22.1 / reference, byte parity checked.
Final parity-corrected flat-interner cursor on declarations/raw/hosted: instructions
2.46/.932/2.34B; cycles .950/.335/.902B; IPC 2.588/2.784/2.598;
wall .27/.11/.25s; RSS 15.8/19.3/14.9MB. GCC instructions 1.71/.558/1.61B;
wall .17/.09/.16s; RSS 25.7/34.8/25.6MB. Full tool/host/reference medians and
ranges, checksums, hashes, all observations and qualification documented there.
Typed cursor GCC instruction ratios 1.43/1.67/1.46; rendered tool 2.88/2.31/3.04
(costly token records versus host source output), reference ratio ~1.09/.86/1.08.
Profiled raw/declarations/hosted triggers: no lost samples, useful call stacks.
Raw batch copy -25% instructions; ring experiment rejected (+5-6% instructions);
flat slabs further -2% instructions / ~8-9% cycles; final escape-parity
correctness fix adds ~4/1.5/4% instructions versus flat (retained observations). Unstable small-input timing
preserved as inconclusive; final hosted enlarged to dominate startup. Scale work
counters grow linearly. No generated executable runtime/size at PA1; separately
compiler text 32,551/41,316 bytes cursor/tool. No numeric requirements waived.

## Remaining groups / handoff ledger
- Unfinished PA1 implementation: none known. Later PA2-34 tools remain scaffold;
  preprocessing expansion, parser/semantics, LowIR/backend are not PA1 scope.
- Independent review still required: whole-stage phase ordering/Unicode and
  malformed-input coverage, physical source mapping, flat hash collision costs,
  borrowed spelling lifetimes, hosted-scale architecture and cost review.
- Observation-only reference discrepancy: escaped `"\\UFFFFFFFF"` (two physical
  backslashes) is accepted as a simple backslash escape plus ordinary characters
  here but reference rejects it. PA1 handout escape/c-char grammar and C++11
  [lex.ccon] simple-escape-sequence support this; no fixture/reference/bundle edits.
  Larger measured workloads match reference exactly. Review is not waived.
- Current group complete; no partial-progress stopping boundary. Ralph owns
  independent audit/advancement. Both review markers preserved unchanged.
- Inherited dirty spec/backend-quality controls committed unchanged separately.
  Keep PA33/34 per-workload <=1.25x GCC instruction gate mandatory; PA24/26
  representation must preserve typed uses/defs, call/EH constraints, revisable
  locations. Early lexical measurements do not certify future generated code.
