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
  52 exact/35 rejection controls; PA2 personal 24 exact families, 1617 arithmetic
  candidate-table cases, 20k-piece valid/invalid maximal sequences, typed cursor
  ownership/provenance checks, and GCC/Clang-qualified portable C++11 inputs.
- Remaining: scale measurements/profile any host gap, final controls/regressions.

## Performance / later design
Measure frozen stage-supported same-source inputs against GCC/Clang/reference:
three pinned warm repetitions, instructions/cycles/IPC, wall/user/sys and RSS;
keep raw results and disclose missing counters. PA2 emits no executable, so
runtime/text/object comparisons are separate and currently not applicable.
PA33/34 per-workload 1.25x GCC instruction gate remains mandatory; future typed
lowering and per-function MIR must not be constrained by this debug renderer.

## Handoff ledger
Unfinished implementation: all PA2 conversion groups (turn start 0/26).
Independent-review questions: whole-stage capability/architecture audit and
performance interpretation remain distinct from implementation validation.
No references, fixtures, coverage or comparison rules changed.
