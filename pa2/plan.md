# PA2 implementation plan

Stage base commit: 4ffe018923d794748dddd398aede178ade3aeb60
Last reviewed commit: 4ffe018923d794748dddd398aede178ade3aeb60

## Design / ownership
- PA1 `Lexer` owns immutable source decoding, source ranges and identifier IDs.
- New typed posttoken cursor owns fixed-table simple classification, one-pass
  numeric grammar/range/type selection, and literal escape/encoding/phase-6 rules.
  Data flow: borrowed lexer spelling -> typed values/IDs -> explicit PA2 renderer;
  no text serialization between phases or complete TU token vectors.
- Numeric work O(spelling length); literal work O(sequence bytes/code units),
  storage bounded by the current maximal string sequence plus immutable source
  and identifier table. Preserve numeric-escape vs Unicode provenance until the
  concatenation encoding is known. Rendered source retention is debug-only.
- Validation: unchanged 26 course fixtures, PA1 through report, file audit;
  personal boundary/property checks and GCC/Clang-qualified portable literals.

## Completed behavior groups / remaining work
- Implemented simple classification and numeric grammar, LP64 candidate tables,
  builtin overflow and unbounded raw UD spellings; PA2-required floating scanning.
- Literal values are decoded by `Lexer::ordinary_literal/raw_literal`, including
  numeric-vs-Unicode provenance (no later spelling reparse). Opt-in collection
  leaves PA1 lightweight. `PostCursor::strings/character` owns encoding/range,
  maximal concatenation and recovery. Sequence scratch/code units reuse cursor
  capacity; returned array bytes are borrowed until the next cursor operation.
- Reserved/unreserved literal-operator suffix identifiers split only after
  `operator` plus the plain empty string. Source/locations and IDs stay typed.
- Validation so far: PA2 26/26, PA1 54/54; file audit 31 files; PA1 personal
  52 exact/35 rejection controls; PA2 personal 25 exact families, 1617 arithmetic
  candidate-table cases, 20k-piece valid/invalid maximal sequences, typed cursor
  ownership/provenance checks, and GCC/Clang-qualified portable C++11 inputs.
- Typed float scanning is allocation-free; PA2-required stream scans live only
  in `render_posttoken`. Raw UD prefix bytes are borrowed payloads, not keys;
  suffix identities are interned. Physical suffix locations survive splices.
- Current implementation groups complete; independent audit remains separate.

## Performance evidence / later design
- Explicit controls: `student.tests/pa2/{check.py,cursor.cpp,README.md}`. Full
  personal suite passes GCC-built, Clang-built and ASan/UBSan tools; portable
  cases execute with both hosts. Clang-built PA1 passes 54 fixtures too.
- Artifact root `/home/vishvananda/work/private/v4sol61strands/artifacts/`:
  `pa2-perf/accepted-{manifest,observations,summary,parity}.json`, all raw perf,
  time/telemetry and cycle profiles. Detailed medians/ranges and host comparisons
  in `student.tests/pa2/README.md`. CPU 0, Xeon E5-2696 v4 @ 2.20GHz, warmup + 3
  interleaved runs, grouped instructions/cycles:u, 100% counter running.
- Five 4–8MB workloads: declarations/raw/hosted/literals/concat typed cursor
  median latency 0.41/0.16/0.36/0.23/0.14s, RSS 15.6/19.1/14.9/11.6/11.6MB;
  instructions 3109/1266/2987/1864/1212M. Same-source GCC syntax-only latency
  2.78/0.32/3.76/0.89/0.11s; instructions 9598/1672/11850/3794/656M.
  Host does more semantic work, preprocessing does less; neither is exact PA2.
  GCC-preprocessing instruction ratios 1.81–2.27x; Clang preprocessing worst
  3.49x (concat, no code-unit conversion). Debug rendering measured separately.
- Concat profile found repeated prefix inventory probes: first-character
  dispatch reduces cursor instructions 22.1%, cycles 14.8%; renderer 21.3%/
  16.5%. Stream-scan isolation reduces declaration/literal cursor instructions
  4.3%/7.1%. Disclosed small regressions: raw cursor instructions +0.50%, cycles
  +1.14%; declaration renderer +0.87%/+0.66%. Overlapping small timing changes
  are inconclusive. Full/half declaration/concat work ratios 2.004/1.996.
- Profile attribution: `accepted-{declarations,concat,hosted}-profile.txt`,
  519/168/436 samples, zero lost, owners Lexer decoding/take and fixed grammar
  classification; no whole-TU queries/per-node allocations. Extra branch/cache
  passes retained. Inherited PA1 repeated controls rerun in `pa1-audit/pa2-*`:
  instructions rise <=2.94%, raw cycles +3.61%, outputs remain exactly equal.
- Four portable outputs exactly match reference; hosted has 24 enumerated GNU
  alias/BF16 differences, not a corrected/normalized oracle. Clang syntax rejects
  GCC system-header extensions; its hosted semantic timing is unavailable,
  while same-source preprocessing succeeds. Gap diagnostics retained; full
  hosted semantic support is later-stage work, not PA2 token conversion work.
- PA2 emits no executable: generated runtime/text/object size N/A, independently
  of compiler latency/RSS. PA33/34 per-workload 1.25x GCC executable instruction
  gate remains mandatory. Preserve direct typed lowering/per-function MIR before
  PA24/26; this debug tool must never dictate production representations.

## Handoff ledger
- Finished: all PA2 behavior groups; failure count 26 -> 0 without course fixture,
  reference, coverage or comparison-rule changes. PA1 54/54, PA2 26/26,
  through PA2 80/80, file audit 31/31; validation logs `pa2-final-*.log`.
- Unfinished implementation: none known within PA2; future preprocessing,
  semantics, lowering and native backend remain later assignments.
- Independent review (not waived): whole-stage correctness/capability and
  architecture audit, especially numeric grammar, literal provenance, borrowed
  lifetimes and interpretation of diagnostic host-relative costs. Review markers
  above remain the entry HEAD and have not been advanced by implementation.
- Handoff boundary: completed PA2 full-stage implementation to Ralph for its
  independent audit; this is not assignment certification or stage advancement.
