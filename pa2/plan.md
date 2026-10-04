# PA2 consolidated plan and final audit handoff

Stage base: `4ffe018923d794748dddd398aede178ade3aeb60`.
Last independently reviewed implementation/control commit:
`83ca4ff2818fdfb29bec2e2ea1d261dc098d619a` (includes `a84e1c18`, `8d813db5`,
`23013af0`). Final documentation commit is identified in the handoff, not
self-referenced here. Whole-stage review: [audit](audit.md); raw-evidence index
and final median/range ledger: [performance](performance.md).

## Final design / Spec Alignment

- Immutable TU source and identifier spelling slabs feed a streaming lexer.
  Identifier/suffix keys are dense IDs. Bounded lookahead and reusable spelling
  scratch replace full-TU token storage; PA1 literal collection stays disabled.
- Lexer decodes ordinary/raw literal elements once, preserving numeric-escape
  versus Unicode provenance and physical suffix locations. Typed PostCursor
  owns fixed simple-token metadata, numeric grammar/LP64 candidate selection,
  character conversion, maximal phase-6 concatenation and recovery.
- Source -> decoded PP tokens -> typed scalars/IDs/borrowed payloads -> explicit
  renderer. No textual production transport or rendered lookup/cache keys.
  The required floating stream scan is confined to the debug renderer; typed
  float conversion uses allocation-free libc scans. Raw UD prefix text is data,
  not identity. Array/raw-UD bytes are borrowed until the next cursor operation.
- Numeric work is O(spelling length), concatenation O(sequence bytes/elements).
  Current-sequence scratch retains capacity; TU source/identifier storage is
  released at TU exit. Audit cites each exercised invariant's `file:function`.
- PA2 owns phases 1–3, restricted no-op phase 4, literal conversion, maximal
  concatenation and token classification. Parsing/semantics/LowIR/native/ELF
  surfaces are not represented as complete. PA33/PA34's per-workload <=1.25x
  GCC executable instruction gate is unchanged and mandatory when due.

## Findings and changes

- Completed all classification, integer/float, UD literal, character/string
  encoding, conflicting prefix/suffix recovery and literal-operator families.
- Independent review covered numeric boundaries, all encoding interactions,
  raw phase reversal, literal provenance, borrowed lifetimes and suffix ranges,
  with GCC/Clang-qualified execution and exact reference comparisons.
- Profiles exposed impossible comment and `<::` probes in shared Lexer::next;
  first-character filters eliminate that work across PA1/PA2 without changing
  phase handling or maximal munch. Legality, profitability, invalidation and
  bounded work are recorded in the audit; no optimizer pipeline exists yet.
- Corrected sequence/converted-byte telemetry for split `operator""suffix`;
  typed physical-splice test now checks it. Behavior remains unchanged.
- Added independent seeded encoding/UD/floating/conflict/provenance controls
  and reproducible pinned slowest-fixture diagnostics. No fixture/reference,
  coverage, comparison rule or mandatory target was weakened or reclassified.

## Performance evidence

- Frozen same-input host/reference A/B: CPU 0, Xeon E5-2696 v4 @ 2.20GHz,
  one warmup + three interleaved observations, simultaneous user instructions/
  cycles and per-run IPC; 100% counter running. Raw counts, timing/RSS, hashes,
  spread, separate branch/cache passes and sampled attribution are retained.
- Five fixed declaration/raw/hosted/literal/concat workloads: final cursor
  instructions 2872.79/1233.72/2719.22/1762.42/1176.81M; wall medians
  0.37/0.17/0.33/0.23/0.14s. Cursor instruction reductions versus pre-audit
  baseline 7.54/2.51/8.96/5.44/2.87%; renderer 5.09/1.97/6.11/3.73/2.73%.
  Concat cycle changes overlap/noise: inconclusive, not a runtime win claim.
- Full/half declaration/concat instructions 2.003x/1.996x. Slowest fixture
  renderer 167.77M versus matching PA2 reference 146.10M instructions; larger
  concat confirms linear work beyond startup timing. Profiles attribute the
  remaining costs to bounded decoding/conversion and explicit debug output.
- GCC syntax-only does more work; GCC preprocessing less. Cursor instruction
  ratios 0.23–1.79x/1.67–2.21x respectively, with lower RSS. Hosted GNU/BF16
  token differences and Clang semantic rejection are preserved, not normalized.
  Host comparisons bracket PA2 and do not claim full native-compiler parity.
- Generated executable runtime/text/object metrics N/A at PA2. Actual compiler
  text/file sizes are separately retained. Future fixed backend controls remain
  unchanged; harness timeouts are hang guards, never architecture/work budgets.
- Earlier implementation measurements and experiments remain in
  `student.tests/pa2/README.md` (historical accepted snapshot), the raw artifacts
  and stage history; final ledger supersedes conclusions, not observations.

## Validation / ledger / remaining assignment work

- Required: file audit passes 31 files; through-PA2 report exit 0, 80/80
  (PA1 54, PA2 26). Independent final runs are retained under
  `artifacts/pa2-audit/`; the final handoff verifies committed-tree status.
- Inherited controls: PA1 52 exact/35 rejection; PA2 25 exact families, 1,617
  candidate cases, valid/invalid 20,000-piece sequences and typed ownership.
  New controls: 300 encoding/UD, 150 floating, 12 prefix conflicts, 5 raw/
  provenance, 3 unbounded UD literals. GCC/Clang and ASan/UBSan tools pass.
- Since the PA1 checkpoint, provenance collection, typed post cursor, numeric
  selection, UD retention, phase-6 lifetime, float adapter separation, physical
  suffix ranges, dispatch optimization and CPU metadata were independently
  audited; no stage-due unaudited handoff or implementation blocker remains.
- Remaining assignment work: PA3 onward, including semantic/lowering/backend
  surfaces and their stage-due controls. This is a completed model-owned PA2
  audit handoff, not Ralph acceptance or stage advancement. Ralph owns those
  gates and goal state; neither is modified by this audit.
