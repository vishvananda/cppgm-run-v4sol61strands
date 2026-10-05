# PA4 implementation plan

Stage base commit: d257e04fb48b42db12267c2e438ac18dc7dc5e89
Last reviewed commit: d257e04fb48b42db12267c2e438ac18dc7dc5e89

## Alignment / ownership
- Missing engine owns all 105 turn-start failures. Preserve PA1–3 lexer,
  interned IDs, structured post-token values and lazy controlling expressions.
- Add a shared PP pull-source interface; PostCursor consumes it directly.
  Immutable file buffers have TU lifetime; only macro definitions, directive
  operands, arguments and pending replacement tokens are retained.
- Macro owner: dense identifier-indexed definitions, normalized replacement
  lists, raw/expanded arguments, persistent unavailable paint, local rescan.
  Expected work: source bytes + visited/generated tokens + required argument
  substitutions; no whole-TU lookup or token-vector replay.
- Directive owner: file stack and conditional stack; indexed macro queries,
  file identity for once, presumed location on tokens, state reset per primary.
  Expected work: each physical token once plus necessary macro expansion.
- Validation: unchanged course cases, explicit student tests, prior report,
  file audit; portable cases qualified with GCC/Clang. Architecture and behavior
  controls due now; future PA33/34 instruction gate remains mandatory, not yet
  measurable on executables from this stage.

## Remaining groups
1. Pull-source adapter and complete macro replacement/diagnostics.
2. Conditionals, inclusion, locations/predefines, pragmas and integration.
3. Full tests, repeated host/reference performance controls and independent
   questions recorded separately below.

## Performance evidence
Pending: freeze stage-supported macro-heavy workloads; warm up and pin host,
student and reference; collect >=3 simultaneous instructions:u/cycles:u runs,
wall/user/system/RSS; compare output equivalence before interpreting costs.
Generated executable runtime/size: not applicable to token-output PA4.

## Handoff ledger
Implementation unfinished: all groups above.
Independent audit questions: whole-stage capability coverage, paint inheritance,
location provenance through substitution, scaling beyond fixture sizes.
Boundary: none yet; implement related groups while this ownership supports it.
