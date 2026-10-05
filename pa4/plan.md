# PA4 compact plan and final audit ledger

Stage base commit: d257e04fb48b42db12267c2e438ac18dc7dc5e89
Last reviewed commit: d257e04fb48b42db12267c2e438ac18dc7dc5e89

Independent audit input HEAD: `b923cabe` (historical markers above preserved).
Final audit: this cohesive commit; see `audit.md` for invariant/function evidence
and `performance.md` for all retained measurements, including rejected trials.

## Final Spec Alignment / design
- Immutable TU-owned source records → pull `Preprocessor : PPSource` → shared
  `PostCursor`; directives feed typed PA3 expressions directly. No textual
  production transport, whole-TU output vector or semantic/IR phase replay.
- Interned IDs/dense reusable macro slots and positional parameters; flat
  paint-extension and device/inode once tables. Once-indexed bulk argument
  sequences, shared spans and explicit prescan frames; demanded arguments
  expanded once, emitted work proportional to replacement output.
- TU-local macro/conditional/include/predefined state; completed lexers released,
  source bytes retained for structured ranges through EOF. Generated tokens
  anchor to invocation identities; physical arguments remain byte-splittable.
- Stage-due controls preserved: all PA1–3 semantics/scale/location controls,
  PA4 macro/directive/integration/error families and independent oracle limits.
  Declaration/template/LowIR/MIR/ELF/executable optimization is not exercised
  yet. PA33/34 per-workload ≤1.25x GCC executable instructions remains mandatory;
  no later capability is certified early or waived by this handoff.

## Final findings and cohesive changes
- Fixed generated literal-operator suffix/range provenance across synthetic,
  ordinary replacement and one-token alias paths; cross-file string sequences
  no longer mix physical offsets. Structured controls observe what dumps miss.
- Stream already-owned directive operands instead of gratuitous indexing/copying;
  reuse/reset conditional cursor/expression capacities, not cached semantics.
- Inline invocation state; one bulk substitution scratch buffer. Replace owning
  per-header pragma-once hash nodes with a flat device/inode set.
- Reject ring-buffer and ASCII lexer experiments: reduced instructions without
  a cycle benefit is insufficient. All raw measurements are preserved.
- Document reduced presumed-__FILE__ include-reference discrepancy with local
  contract proof and exact bundle revision. No course references/coverage or
  comparison/normalization rules changed.

## Final performance evidence
Accepted `pa4-perf/audit-certified`: identical C++11 bytes, token parity,
frozen baseline/final/GCC/Clang/reference, one matching-affinity warmup,
three CPU-0 interleaved repeats of instructions/cycles/IPC/time/RSS. All PMU
running percentages 100%. Profiling, phase/work counters and separate branch/
cache passes cover every disproportionate family and slowest suite fixture.
85k conditions: 4732.8M → 4501.0M instructions (~−4.9%); principal paired cycles
1905.0M → 1838.4M (~−3.5%, nonoverlapping ranges), wall ranges overlap.
Host same bytes: 1343.5M instructions / .14s / 23.7MiB vs student .54s / 15.3MiB.
The residual ~3.35x instruction/~4x latency gap is profiled and scales linearly;
redundant indexing/allocation fixed, not excused by timeouts.
16k probes ~467.7M / .12s / 61.8MiB vs GCC 152.1M / .03s / 21.7MiB;
prior quadratic compaction proof retained. Replacement/conditional/header
scaled controls show consumed/produced-work growth, not global rescans.
Slowest fixture: 155.1M / .03s / 20.1MiB vs GCC 219.5M / .07s / 78.7MiB.
Compiler text +~3%; native-program runtime/text/object size N/A at PA4.
No numeric target reclassified, no spec budget waived. Historical evidence
including 100k depth, Clang scale crash and rejected/noisy runs remains intact.

## Validation and closed handoffs
- Required file audit: pass, 36 files; `make test-pa4`: 105/105, exit 0;
  `make test-report-through-pa4`: 205/205, exit 0 (reports serial).
- PA4 personal controls, 30 independent interaction families/batches, GCC/Clang
  portable qualification, Clang-built and ASan+UBSan implementations, structured
  origin/lifetime checks; inherited PA1–3 and PA2–3 independent controls pass.
- Inherited open audit questions all closed: broader course paint, raw/pasted
  provenance (defect fixed), many-header once/path ownership (flat fix/scales),
  conditional capability/cost (independent controls/scales/profiles/owned fixes).
- No known unfinished PA4 contract, correctness, self-containment, architecture,
  timeout or file-audit defect. Later hosted/template/executable work is due at
  later stages; bounded lexer/conversion tuning opportunities are disclosed.
- Intended changes committed; clean working-tree verification is the final
  local gate. Ralph owns external acceptance and advancement, not this record.

- Delivery repeat: `pa4-perf/audit-delivery` and `pa4-audit/delivery-complete`
  reproduce accepted binary hashes, instruction work and scale curves; all
  counters run at 100%. Text-cycle outlier and inconclusive wall ranges are
  preserved in `performance.md`; no noisy latency claim or budget waiver.
- Post-consolidation delivery checks repeat file audit 36 files, PA4 105/105,
  cumulative 205/205, independent 30 families, personal scale/host controls and
  structured origins, all exit 0 (`pa4-audit/delivery-*.log`).
