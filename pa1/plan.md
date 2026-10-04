# PA1 consolidated plan / final audit handoff

Stage base commit: 326539d7382056c8816d77ad7c4e85acb7de6653
Last reviewed commit: 326539d7382056c8816d77ad7c4e85acb7de6653
Implementation audited through: 5f1b0c16 plus this audit's fixes below
Final whole-stage review: `audit.md` (independent reconstruction, every architecture
invariant's `file:function`, findings/proofs, coverage, complete evidence ledger).

## Spec Alignment / design
- Immutable TU source -> bounded UTF-8/phase1/phase2 character cursor -> typed pull
  tokens with physical byte ranges/locations and interned name IDs -> explicit
  debug renderer. No textual cross-phase transport, TU token vector or late lexer.
- `Lexer::decode/phase1/translated/next`: Unicode/trigraph/UCN/splice/LF, comments,
  contextual headers, maximal munch, literals and suffixes. Literal UCN provenance
  is retained; basic/control values are accepted as literal data, rejected outside.
- `Lexer::raw_literal`: physical phase restoration, <=16-code-point delimiter,
  exact course delimiter exclusions, linear validated body scan and bulk copy.
- `IdentifierTable::intern/grow`: full-spelling identity, dense IDs, cached hash,
  <=70% open-addressing occupancy, geometric growth, stable TU-owned spelling slabs.
- `DebugPPTokenStream::write_token/append/flush`: bounded 64-KiB view buffer, no
  token-kind owning allocation; shared out-of-line formatting, EOF/unwind flush.
- O(bytes + tokens + spelling bytes), expected amortized interning; memory
  O(source + distinct-name storage + largest token + bounded render scratch).

## Audit findings / changes
- Fixed lost UCN origin and outside-literal basic/control acceptance across the
  full character->token ownership path; physical encoded LF cannot splice.
- Corrected overly broad raw-delimiter whitespace exclusion (CR is course-valid).
- PMU-guided renderer allocation/formatting removal; moved code out of line after
  measuring the initial inline text-growth regression. All experiments preserved.
- Added reducers, source/identity checks, binary/oversized rendering and flush
  controls, and frozen paired A/B measurement with host executable hashes.
- No course fixture/reference/bundle edits. Reference's literal-UCN and escaped
  backslash discrepancies have reduced sources and N3337/N3485 clause proof in
  `audit.md`, not compiler-agreement-only justification.

## Performance evidence
Artifacts: `$RALPH_ARTIFACT_DIR/pa1-audit/`, retaining original `pa1-perf{,-final}`.
CPU 0 / Xeon E5-2696 v4 / O3 -g; same frozen multi-MB inputs, one warmup, seven
interleaved baseline/final/GCC/Clang/reference runs; seven-run confirmation also
retained. Grouped instructions:u/cycles:u and separate branch/cache passes 100%
running. Raw counts, timing/RSS, telemetry, hashes, all spreads, profiles and
`accepted-summary.txt` retained (earlier summaries preserved). Hosts use C++11/trigraphs/-E/-P on identical input.

Final audit7 cursor declarations/raw/hosted instructions 2.505/.945/2.397B;
GCC 1.715/.558/1.606B; ratios 1.461/1.691/1.493. Wall .31/.14/.36s vs GCC
.19/.11/.23s; RSS 15,656/19,172/14,952 KiB vs 25,660/34,844/25,660 KiB. Rendered instructions
3.130/1.037/3.030B, -36.57/-19.62/-38.00% from baseline; cycles ~-32/-18/-38%,
wall .40/.16/.47s. Correctness cost: cursor instructions +1.87/+1.34/+2.27%.
IPC and every metric's median/range and host comparisons are in `audit.md` and
`accepted-summary.txt`; all earlier observations remain. Shared machine-wide
hosted cycle/wall spikes (~2x also in GCC/Clang/reference/baseline) are disclosed,
not discarded; counts and paired median renderer benefits remain repeatable.
Profiles resolve renderer allocation/sentry trigger; remaining bounded lexer work
is attributed, zero lost samples. Whole suite and repeated tiny fixtures reviewed
separately (startup/harness-dominated).

Text: final cursor/tool 33,420/38,443 bytes vs baseline 32,551/41,316. No PA1
emitted executable/object exists; generated runtime/size remains later-stage due.
No numeric budget waived or reclassified; harness timeouts are not budgets.

## Validation / ledger / remaining work
- Required file audit: pass, 23 files. Required through-PA1 report: exit 0, 54/54
  unchanged fixtures. Prior assignments: none.
- Explicit controls: 52 exact outputs, 35 rejections, combined GCC/Clang C++11
  qualification; typed cursor and renderer GCC/Clang + ASan/UBSan; 150 deterministic
  mixed reference comparisons; 1K–100K linear scale and 100K distinct names.
- Full GCC/reference workload parity; Clang lexical parity except its documented
  phase4 removal of three GCC pragmas (original timing inputs not altered).
- Every handoff since stage base independently reviewed: streaming lexer,
  phase fixes, raw bulk copy, flat/slab IDs, speculative escape parity and final
  evidence. No unaudited PA1 handoff or open PA1 implementation/audit defect.
- PA2–34 remain assignment work, not PA1 audit blockers. Preserve backend-quality
  controls and PA33/34 <=1.25x GCC instruction gate, and future typed MIR call/EH/
  allocation constraints. Early lexical measurements do not certify a backend.
- Intended fixes and audit record are committed; clean final git status is checked
  after committing. Ralph owns external acceptance and stage advancement.
