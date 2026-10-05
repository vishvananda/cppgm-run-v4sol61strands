# PA4 implementation plan

Stage base commit: d257e04fb48b42db12267c2e438ac18dc7dc5e89
Last reviewed commit: d257e04fb48b42db12267c2e438ac18dc7dc5e89

## Design / spec alignment
- Reusable PPSource pull interface feeds PostCursor and PA3 expression machinery
  directly; dumps are observations, never production transport. Identifiers
  interned on entry; definitions/parameters indexed by IDs, no TU scans.
- Macro owner: raw/shared argument spans, once-indexed delimiter/comma links,
  explicit prescan task stack, cached expanded arguments, stringize/paste and
  course paint. Work tracks bytes, visited/generated tokens and substitutions.
  Persistent 32-bit radix sets and flat (root, macro-ID) extension cache avoid
  chain-quadratic membership and repeated-call storage growth. Definitions reuse
  slots across undef; caches are immutable TU facts, no global invalidation.
- Directive owner: active file/conditional stacks, fixed directive grammar,
  inode once identities and indexed TU path cache, source/presumed locations,
  all predefined state local to primary source. Immutable source buffers outlive
  tokens; completed lexers are released. Deferred definitions/operands/arguments
  alone retain token sequences. Includes without once repeat processing.

## Remaining groups / validation
Implementation: no known unfinished PA4 behavior group. Macros, conditionals,
includes, locations, predefines, pragmas and integration implemented together.
Course PA4 **105/105**, earlier PAs **100/100**, through-PA4 **205/205**; file audit
passes (36 files). Explicit PA4 semantic/random/scale and location controls,
ASan+UBSan, Clang-built implementation, inherited PA1–3 personal controls and
PA2–3 independent-oracle audit controls pass. No references/coverage changed.

## Performance evidence
See `performance.md` and `student.tests/pa4/README.md`. Nine same-source families,
warm pinned three-repeat instructions/cycles/IPC plus time/RSS, GCC/Clang/ref and
frozen A/B binaries; raw artifact labels retained. Final `source-owned` cursor
instructions/GCC: 0.55x aliases, 1.10x arguments, 1.53x paste, 2.65x definitions,
3.53x conditional; conditional .55–.62s, 15.5 MiB vs GCC .15s, 23.7 MiB.
Disproportionate families profiled; remaining gap is lexer/conversion/operand
cost, not whole-TU search or descendant replay. Timing gains with overlapping
ranges remain inconclusive, not waived. Generated runtime/size N/A at PA4.
100k nesting passes: 400k indexed tokens / 100k spans, .21s, 109 MiB. The prior
8k-depth/12 GiB crash and repeated-call paint growth are fixed. Host Clang's
3k-depth crash is recorded separately; common timing uses qualified 1k depth.
PA33/34 per-workload 1.25x GCC executable instruction gate remains mandatory.

## Handoff ledger
Implementation unfinished: none known in PA4's documented contract.
Independent audit questions (not waived): broader course paint interactions,
raw/pasted-token source provenance, many-header once/path cache behavior,
conditional-cost capability coverage beyond fixtures. Hosted-system headers
are later-stage work per handout, not an unimplemented PA4 acceptance claim.
Boundary: full validated implementation handoff, not assignment certification;
Ralph owns independent audit and advancement. Review markers above preserved.
