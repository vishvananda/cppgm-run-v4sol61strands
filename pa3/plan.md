# PA3 implementation / audit handoff

Stage base commit: `99a5fe4c45fac2edcaef92d5984d07a03cd676ba`.
Last reviewed commit: `99a5fe4c45fac2edcaef92d5984d07a03cd676ba`.
Entry markers preserved; implementation commits are not independent reviews.

## Design / spec alignment
- `PostCursor::advance/next` owns one typed lookahead, preserves logical newline
  in controlling mode, reuses PA2 literal conversion, keeps PP identifier IDs.
  `Lexer::next` retains course PA1 word-punctuator rules (`new/delete` included).
  No full-TU token vector, source reparse or debug-spelling semantic transport.
- `ControllingExpression::parse/reduce` parses once using explicit precedence
  stacks into a compact line-owned typed node arena. Parenthesis/conditional
  stack boundaries reject malformed grammar even in unevaluated branches.
- `ControllingExpression::literal/binary/evaluate` owns 64-bit promotion,
  static conditional result types, iterative lazy evaluation, evaluated
  division/mod/shift errors, signed div/rem and sign-preserving right shifts.
  Unsigned bit storage avoids host signed-overflow UB; INT64_MIN/-1 is rejected.
- `ControllingExpression::next` recovers expression errors at the logical line;
  lexical exceptions remain fatal even after a line-local error. Arena/stack
  capacity is reused then released at invocation end; no per-node allocations.
- Data flow: immutable TU source -> shared streaming lexer/converter -> typed
  nodes -> lazy value -> explicit renderer. O(bytes + tokens) work, O(largest
  expression + interned names) auxiliary storage; geometrically growing dense
  ID/slab intern table; no name lookup scales with TU size. Defined callback
  accepts canonical ID/context for PA4 macro lookup. Later parser/templates,
  lowering/backend concerns remain outside PA3, not simulated with shortcuts.

## Validation / remaining groups
Required PA3 20/20 (turn start 0/20); earlier PA1–2 80/80; through PA3 100/100;
file audit 33 files. No fixtures/references/comparison rules changed.
Personal controls explicitly rerun: PA1, PA2, PA2 independent audit, PA3;
PA3 100k depth/chains, 1200 GCC/Clang-qualified native/PP cases, Clang-built
implementation, ASan+UBSan all pass. Future backend qualification: both hosts,
8/8 deliberate failures detected; no native/backend acceptance claimed.

## Performance evidence
Details/ranges/raw paths: [performance](performance.md). CPU 0 warmup + 3
interleaved runs, frozen SHA256/flags, instructions/cycles/IPC and wall/user/
system/RSS. `pa3-perf/signoff-*` has same-session baseline/host comparisons;
`pa2-perf/pa3-signoff-*` preserves five inherited workloads including hosted.
PA3 latency 0.13–0.47s medians, RSS 7.6–53.0 MiB; <=1.87x GCC instructions,
<=2.60x GCC latency on these stage-supported diagnostic envelopes. Profiles
attribute long-line/deep-nesting gaps to bounded lexer/conversion/typed graph
work, not scans/reparses. Single-character dispatch reduces retired instructions
1.4–12.5%; identifier latency remains inconclusive (overlapping spread).
Inherited posttoken instructions 0.979–1.002x audited baseline, no growing cost;
Clang semantic GCC-header rejection remains a documented capability gap.
Generated-executable runtime/text size N/A: PA3 emits values. Mandatory PA33/34
per-workload 1.25x GCC instruction gate preserved; early ratios do not waive it.

## Handoff ledger
- Completed coherent group: all PA3 expression cursor, grammar/type, lazy integer
  evaluation and line-error ownership, plus measured shared-terminal hot path.
- Unfinished PA3 implementation: none known. Future PA4+ implementation remains.
- Independent-audit questions (not waived): whole-stage grammar/literal/type
  coverage, architecture/cost review beyond fixed fixtures, hosted capability
  boundary and any undiscovered semantic edges. Ralph owns this review.
- Handoff boundary: implemented and verified full PA3; do not advance into PA4
  before Ralph's whole-stage audit/acceptance. Review markers remain unchanged.
