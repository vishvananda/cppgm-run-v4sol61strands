# PA4 performance evidence

## Protocol and capability

Artifacts: `$RALPH_ARTIFACT_DIR/pa4-perf/` labels `initial`, `singleton`,
`owned`, `final`, `indexed`, `validated`, `handoff`, `source-owned` retain all
observations, frozen O3/g binaries, input hashes, flags, commands and telemetry.
`indexed` stopped at a Clang crash; completed earlier workload measurements
remain raw evidence, not a successful whole run. `source-owned` is the final
source version, paired with frozen `handoff` binaries. GCC and Clang preprocess
**identical** C++11 input bytes; student cursor, dump and reference outputs are
checked against independently specified token/count results before timing.
The cursor has no textual transport or rendering; dump-tool cost is separate.

CPU: Intel Xeon E5-2696 v4 @ 2.20GHz, taskset CPU 0 (allowed set 0–87).
Warmup outside measurements; warm-cache, three interleaved repeats per variant
per workload. Primary group `{instructions:u,cycles:u}`, 100% running for all
completed measurements; IPC derived from paired counts. Wall/user/system and
peak RSS are retained separately. Small timing gains with overlapping ranges
are inconclusive. Generated executable runtime and text size: **N/A**, PA4
produces tokens. The PA33/PA34 1.25x GCC executable instruction gate remains
mandatory at those stages; these compiler measurements do not certify it.

## Final compiler costs

Median retired instructions/cycles are millions. `student` is the structured
cursor, not the rendering tool. Each row also has Clang/reference/raw repeat
ranges in `source-owned/summary.json`; complete dump costs are there separately.

| workload | student/GCC instructions M | student cycles M / IPC | student wall range s | student RSS MiB | GCC wall s / RSS MiB |
|---|---:|---:|---:|---:|---:|
| 300k ordinary tokens | 764.9 / 522.8 | 304.7 / 2.51 | .08–.09 | 6.4 | .06 / 12.0 |
| 120k definitions + 100k tokens | 1384.2 / 522.5 | 576.1 / 2.40 | .18–.19 | 44.2 | .08 / 31.3 |
| 70k repeated 8-use arguments | 1868.3 / 1696.7 | 732.1 / 2.55 | .21–.30 | 5.7 | .24 / 106.2 |
| 200k pastes | 2722.9 / 1778.1 | 1066.9 / 2.55 | .34–.35 | 7.8 | .23 / 53.2 |
| 30k alias chain + 4 substitutions | 583.6 / 256.8 | 314.7 / 1.85 | .13–.19 | 66.8 | .14 / 155.7 |
| 85k conditionals | 4736.3 / 1343.6 | 1890.5 / 2.51 | .55–.62 | 15.5 | .15 / 23.7 |
| 60k once includes + 60k tokens | 802.6 / 387.7 | 326.2 / 2.46 | .10–.10 | 6.3 | .06 / 18.9 |
| 300k three-link aliases | 990.1 / 1802.9 | 408.5 / 2.42 | .12–.19 | 4.9 | .30 / 111.1 |
| 60 × 1k nested arguments | 434.8 / 22731.5 | 180.4 / 2.41 | .06–.08 | 5.0 | 3.99 / 1553.9 |

All comparable families are within a small fixed factor of GCC. The largest
remaining compiler-cost gap is conditionals: 3.53x instructions, 3.87x median
wall; versus Clang .22s / 89.4 MiB and reference .72s / 11.3 MiB. Lookup is
2.65x instructions / 2.25x wall. These are disclosed costs, not parity claims.
Profiles of these disproportionate families and paste are retained:
`source-owned/{conditional,lookup,pastes}.perf.data` and `.profile.txt`.
Conditional samples concentrate in lexer peek/next/phase1, macro cursor,
post-token conversion and small operand allocation, not TU-wide searches or
repeated descendant scans. Lookup is dominated by lexer peek/take and spelling
copies. Separate conditional branch/cache passes at 100% running recorded
825,864,702 branches / 1,906,721 misses and 450,471 cache references / 110,409
misses (machine-specific event meanings). Future tuning must preserve this
complete lexical/value validation, not memoize unchecked textual expressions.

## Defects found and fixed

- Initial paint sets accumulated fresh radix paths per invocation. Profiling
  identified insertion. The extension cache is a flat table keyed by immutable
  root + identifier ID; completed facts remain valid across define/undef.
  300k aliases fell from 516 MiB to 5 MiB; 98 nodes suffice regardless of repeats.
  Common direct aliases bypass argument scheduling and delimiter indexing.
  Paired `handoff` runs: 985M vs 2057M instructions on aliases; other unaffected
  families' instruction regressions are below 1%, timing claims inconclusive.
- Repeated pragma-once includes had excessive syscall latency. TU-local indexed
  path/identity caching avoids stat on confirmed once hits. Immutable file bytes
  are shared on normal repeat includes, while language processing still repeats.
  Completed lexer/expansion buffers are reclaimed; only source records persist.
- A nested argument audit found O(depth²) copies and an 8k-depth SIGSEGV with
  12 GiB RSS. Delimiters/comma links are now indexed once; argument spans share
  one immutable sequence; explicit prescan tasks replace recursive C++ calls.
  100k depth passes in .21s / 109 MiB, 400k indexed tokens, 100k argument spans,
  200001 expanded visits. Depth scaling is linear, not hidden by a cutoff.
- Same-source capability investigation is saved at
  `$RALPH_ARTIFACT_DIR/pa4-validation/nested-capability.json`: GCC accepts 10k
  (.8e1 seconds / 4.35 GiB), reference accepts 10k, host Clang SIGSEGVs at 3k
  and 10k. Portable timing uses 1k depth accepted by every tool. Student-only
  100k success is supplementary evidence, not a claim of equal supported work.

## Current-stage quality controls

PA4 105/105; through-PA4 205/205; file audit passes. Explicit personal controls,
ASan+UBSan build, Clang-built implementation, source-location/range lifetime
control and inherited PA1–3 personal/PA2–3 audit controls pass. No references,
coverage or comparison rules were changed. Hosted-system-header support is
outside PA4's handout; large stage-supported inputs are measured, not mislabeled
as hosted compilation. Independent whole-stage review remains Ralph-owned.
