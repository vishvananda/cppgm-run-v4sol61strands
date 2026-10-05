# PA4 performance evidence

## Protocol and capability

Artifacts: `$RALPH_ARTIFACT_DIR/pa4-perf/` labels `initial`, `singleton`,
`owned`, `final`, `indexed`, `validated`, `handoff`, `source-owned`,
`compact-probes`, `final-handoff` retain all observations, frozen O3/g binaries, input hashes, flags, commands and telemetry.
`indexed` stopped at a Clang crash; completed earlier workload measurements
remain raw evidence, not a successful whole run. `source-owned` records the
source-lifetime/alias fixes; `compact-probes` pairs
linear probe compaction with its frozen pre-fix binaries. `final-handoff`
remeasures all twelve families after extracting the compaction helper, paired
with `compact-probes`. The final manifest hashes both sources and headers.
GCC and Clang preprocess
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
ranges in `final-handoff/summary.json`; complete dump costs are there separately.

| workload | student/GCC instructions M | student cycles M / IPC | student wall range s | student RSS MiB | GCC wall s / RSS MiB |
|---|---:|---:|---:|---:|---:|
| 300k ordinary tokens | 764.9 / 522.8 | 309.7 / 2.47 | .09–.14 | 6.3 | .06 / 12.1 |
| 120k definitions + 100k tokens | 1384.7 / 522.3 | 576.6 / 2.40 | .20–.21 | 44.2 | .08 / 31.3 |
| 70k repeated 8-use arguments | 1868.3 / 1696.7 | 737.1 / 2.53 | .21–.23 | 5.7 | .24 / 106.2 |
| 200k pastes | 2730.1 / 1778.1 | 1061.0 / 2.57 | .36–.38 | 7.8 | .27 / 53.3 |
| 30k alias chain + 4 substitutions | 583.6 / 256.8 | 323.5 / 1.80 | .15–.15 | 66.6 | .15 / 155.7 |
| 85k conditionals | 4739.9 / 1343.6 | 1894.0 / 2.50 | .61–.64 | 15.5 | .16 / 23.7 |
| 60k once includes + 60k tokens | 803.7 / 388.0 | 332.3 / 2.42 | .10–.11 | 6.3 | .06 / 18.9 |
| 300k three-link aliases | 990.1 / 1802.9 | 404.0 / 2.45 | .12–.19 | 5.0 | .28 / 111.0 |
| 60 × 1k nested arguments | 434.8 / 22729.9 | 181.7 / 2.39 | .06–.06 | 5.0 | 4.16 / 1552.4 |
| 1k attribute probes in one expression | 32.0 / 24.1 | 17.0 / 1.89 | .01–.01 | 6.9 | .01 / 12.0 |
| 4k attribute probes | 120.7 / 49.7 | 59.6 / 2.02 | .03–.03 | 18.7 | .01 / 14.0 |
| 16k attribute probes | 475.5 / 152.1 | 244.6 / 1.94 | .13–.14 | 63.1 | .03 / 21.6 |

All comparable families are within a small fixed factor of GCC. The largest
remaining compiler-cost gap is conditionals: 3.53x instructions, 3.94x median
wall; versus Clang .24s / 89.3 MiB and reference .80s / 11.3 MiB. Lookup is
2.65x instructions / 2.50x wall. The final 16k-probe operand is 3.13x GCC
instructions, 4.33x wall and 2.92x RSS; its linear scaling is verified below. These are disclosed costs, not parity claims.
Profiles of these disproportionate families and paste are retained:
`source-owned/{conditional,lookup,pastes}.perf.data` and `.profile.txt`,
plus final `final-handoff/{conditional,lookup,probes16000}.perf.data`.
The short probe profile has only 19 samples; the separate ten-invocation
profile `probes16000-repeated.perf.data` has 137 samples, no lost samples and
symbolized cursor/lexer/operand stacks. It is coarse attribution, not evidence
for fine percentage differences.
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
  (8.25 seconds / 4.35 GiB), reference accepts 10k, host Clang SIGSEGVs at 3k
  and 10k. Portable timing uses 1k depth accepted by every tool. Student-only
  100k success is supplementary evidence, not a claim of equal supported work.

- Final source audit found quadratic `vector::erase` in repeated attribute
  probes. `compact_attribute_probes` now moves/visits each operand token once.
  Paired `compact-probes` 1k/4k/16k instructions: 32.0/120.7/475.7M versus
  409.0/6158.7/97107.8M before; 16k wall .11–.18s versus 14.67–16.35s,
  median .11s versus 15.00s. RSS stays ~63 MiB, proportional to the retained
  directive operand. All hosts/reference produce the same output. Conditional
  instructions change <0.01%; wall ranges overlap, timing improvement is
  inconclusive. `final-handoff` helper extraction changes no family by >0.1%
  instructions versus `compact-probes`; no timing improvement claimed.

## Architecture ownership trace

| current-stage invariant | concrete owner / data flow | validation |
|---|---|---|
| immutable source and streaming delivery | `preprocessor.cpp:Preprocessor::Impl::include_file/read/raw`, `Preprocessor::next/source_buffer`; `PostCursor::advance/next` consumes PPSource directly | explicit `cursor.cpp` physical range, presumed line, lifetime after EOF; text/include measurements |
| compact ID lookup and TU lifetime | `identifiers.cpp:IdentifierTable::intern`, `macros.cpp:MacroEngine::defined/define/undefine`; `preproc.cpp:main` constructs one engine per primary | dense definitions, 20k define/undef cycles, primary reset |
| no recursive descendant replay | `macros.cpp:MacroEngine::sequence/arguments/next/substitute`; once-indexed shared spans feed explicit prescan tasks, expanded parameters cached | 100k nested control and nine original workloads |
| bounded recursion suppression | `macros.cpp:MacroEngine::painted/add_paint/grow_extensions`; persistent ID radix roots and flat extension keys | repeated aliases, deep chain, paint telemetry |
| direct typed controlling expressions | `preprocessor.cpp:Preprocessor::Impl::condition`, `OperandSource::next`; PA2 PostCursor feeds PA3 ControllingExpression, no dump/reparse | lazy invalid-expression controls and 85k conditionals |
| linear fixed attribute-probe grammar | `preprocessor.cpp:compact_attribute_probes` consumes to closing delimiter and compacts in place | qualified 1k/4k/16k portable probes and paired counts |

`MacroEngine::synthetic` lexes only preprocessing-generated tokens (paste,
stringize, builtins), and `_Pragma` lexes its destringized directive operand;
neither is post-parsing semantic or lowering replay. Required source-region
processing is not re-run through textual phase dumps. Declaration/template/ELF
trace and executable optimization are later-stage work, not PA4 validation.

## Current-stage quality controls

PA4 105/105; through-PA4 205/205; file audit passes. Explicit personal controls,
ASan+UBSan build, Clang-built implementation, source-location/range lifetime
control and inherited PA1–3 personal/PA2–3 audit controls pass. Final logs are `pa4-validation/handoff-{test-pa4,prior,through,file-audit,personal,
sanitizer,clang,pa1-personal,pa2-personal,pa2-audit,pa3-personal,pa3-audit}.log`;
location control exits 0.
Earlier parallel report invocations shared report state and produced inconsistent
totals; their artifacts are retained but excluded. Final report invocations were
serial. The initial compaction placement exceeded the file auditor's reported
function-span limit; extracting the grammar helper resolved it without changing
the auditor. No references, coverage or comparison rules were changed. Hosted-system-header support is
outside PA4's handout; large stage-supported inputs are measured, not mislabeled
as hosted compilation. Independent whole-stage review remains Ralph-owned.

Compiler binary sizes (not generated-program sizes): frozen O3/g cursor has
209,734 text bytes / 4,312,808 file bytes; dump tool 212,647 text bytes /
4,351,136 file bytes. Program runtime/text size remain N/A at this stage.
