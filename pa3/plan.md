# PA3 implementation ledger

Stage base commit: `99a5fe4c45fac2edcaef92d5984d07a03cd676ba`.
Last reviewed commit: `99a5fe4c45fac2edcaef92d5984d07a03cd676ba`.
These entry markers are preserved during implementation; not an independent PA3 approval.

## Design/spec alignment and behavior groups
- Cursor owner: immutable source -> shared lexer/literal conversion -> one typed
  lookahead, with logical-newline boundaries and PP identifier semantics.
  No full-TU token vectors, spelling transport, or source reparsing.
- Expression owner: iterative precedence parser -> compact line-owned typed node
  arena -> iterative lazy evaluator. IDs index contiguous storage; each token
  parsed once; each evaluated node visited once; O(bytes + tokens) work and
  O(largest logical expression + interned names) auxiliary storage. Retain arena
  capacity between lines; release all TU ownership at invocation end.
- Integer owner: PA2 integral classification/sign extension, 64-bit arithmetic,
  static branch conversion, lazy runtime errors, line-local grammar recovery.
  Validation: all 20 required fixtures, personal adversarial/deep tests,
  prior PA1/2 checks, file audit, repeated host-relative profiling.

## Remaining implementation
1. Implement cursor mode and integrated typed parser/evaluator; reusable mock
   defined callback allows PA4 real macro lookup without expression redesign.
2. Personal semantic/scaling controls, required suites and performance evidence.

## Performance and future controls
No measurements yet. Record frozen binaries/inputs, pinned warmup + >=3 runs,
raw instructions/cycles/IPC, latency/RSS separately from emitted program costs.
PA3 emits values, not executables: generated runtime/text costs are N/A.
PA33/34 per-workload <=1.25x GCC instruction gate remains mandatory; no current
representation choice or inconclusive PMU result waives that future gate.

## Handoff ledger
- Unfinished implementation: groups above.
- Independent-audit questions: whole-stage correctness, architecture and broad
  capability/cost review pending; fixture success alone is not certification.
- References/coverage: unchanged. Turn-start required failures: 20/20.
