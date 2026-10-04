# PA1 implementation plan

Stage base commit: 326539d7382056c8816d77ad7c4e85acb7de6653
Last reviewed commit: 326539d7382056c8816d77ad7c4e85acb7de6653

## Design / ownership
- Baseline: 0/54 required tests, stub exits NOT_IMPLEMENTED; no earlier stages.
- Source translation owner: shared preprocess lexer. Immutable UTF-8 source ->
  bounded translated-character lookahead (physical offsets/locations), then tokens.
  Decode UTF-8/UCNs/trigraphs, splice, synthesize final LF; raw literal regions
  use original bytes, not a whole-source transformed copy. O(source bytes).
- Token recognition owner: pull cursor, compact kind/identifier identity and
  source range; reused borrowed spelling scratch only for tool/legacy callbacks.
  Intern identifiers once; no successive owning token vectors or text transport.
  Whitespace/comments, Annex E names, numbers, punctuators: bounded local work.
- Literal/context owner: same cursor. Ordinary/raw/UD literals and directive-only
  headers; linear scans with at most 16 delimiter characters per raw candidate.
- Validation: all 54 course fixtures unchanged, explicit student tests for phase
  interactions, rejection, locations, Unicode boundaries and scale; GCC/Clang
  qualification of portable sources; file audit and through-PA1 report.

## Remaining groups
1. Implement source translation and typed token cursor, wrapper bridge.
2. Complete lexical/context edge cases and broader personal controls.
3. Repeated pinned PMU/timing/RSS measurements versus same-input GCC/Clang and
   reference; freeze inputs/binaries, profile disproportionate supported work.

## Performance / later design
No measured claims yet. PA1 measures phases 1-3, not semantic compilation.
Compiler latency/RSS separate from executable runtime/size (not produced here).
Keep PA33/34 mandatory per-workload <=1.25x GCC instruction gate; PA24/26 must
retain typed uses/defs, call/EH constraints and revisable locations. Supplied
backend-quality controls are inherited unchanged and become due at owning PAs.

## Handoff ledger
- Implementation unfinished: all PA1 behavior groups above.
- Independent audit questions: Unicode/phase interaction coverage, source mapping,
  streaming lifetime/interning, complexity and host-relative cost beyond fixtures.
- No reference changes, waived requirements or reduced coverage.
- Initial dirty spec/backend controls are preserved unchanged in a separate
  baseline commit; review markers above remain fixed throughout implementation.
