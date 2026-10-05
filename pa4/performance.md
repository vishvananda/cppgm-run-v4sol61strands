# PA4 performance evidence

## Independent final audit: accepted measurements

The accepted implementation is frozen in `$RALPH_ARTIFACT_DIR/pa4-perf/audit-certified/`.
The prior baseline is `audit-base` (stage HEAD `b923cabe`). Every source/header,
binary, host driver/cc1plus and input hash, flag, command, exact token parity,
three raw repetitions, PMU running percentage, IPC, wall/user/system time and
peak RSS is retained in `manifest.json`, `parity.json`, `raw.json`,
`summary.json`, and per-run `.perf/.time/.stderr` files. Production cursor and
rendered preproc are measured separately; no compiler delegates to the host.
GCC 15.2.0 and Clang 22.1.2 preprocess identical C++11 source with `-E -P`;
our implementation is built `-std=c++11 -O3 -g`. All variants have one unmeasured
warmup, then three interleaved warm-cache runs pinned to CPU 0 on Intel Xeon
E5-2696 v4 @ 2.20GHz, allowed affinity 0–87. User-mode
`{instructions:u,cycles:u}` are simultaneous and **100% running**. Timing
resolution is .01s; tiny cases are scale controls, not latency-benefit claims.
All failed/interrupted/noisy experiments are retained, not filtered away.

### Fixed compiler families: same-input host comparison

Instructions and cycles in millions, RSS in MiB. Values are medians; brackets
are the three-run min–max. Columns show **student / GCC**, including both IPCs.
The output must be exactly the specified number of `done` tokens before timing.
Generated executable runtime, text and object size are **N/A at PA4** (no native
program is produced), not a zero or a claim of backend parity. The PA33/PA34
per-workload ≤1.25x GCC executable instruction gate remains mandatory.

| fixed family | instructions student / GCC M | cycles/IPC student; GCC | wall student; GCC s [range] | RSS student / GCC MiB |
|---|---:|---:|---:|---:|
| text | 763.4 / 522.8 | 306.5/2.49; 186.8/2.80 | 0.09 [0.09–0.13]; 0.06 [0.05–0.06] | 6.3 / 12.1 |
| lookup | 1384.1 / 522.3 | 580.6/2.38; 208.9/2.50 | 0.19 [0.19–0.19]; 0.08 [0.07–0.08] | 44.0 / 31.3 |
| arguments | 1840.4 / 1696.7 | 739.9/2.48; 635.3/2.67 | 0.23 [0.22–0.27]; 0.25 [0.24–0.26] | 5.7 / 106.2 |
| pastes | 2738.7 / 1778.1 | 1058.2/2.59; 682.8/2.60 | 0.32 [0.31–0.34]; 0.23 [0.22–0.23] | 7.9 / 53.3 |
| deep | 583.7 / 256.7 | 328.6/1.78; 170.2/1.51 | 0.14 [0.13–0.14]; 0.13 [0.13–0.14] | 66.7 / 155.7 |
| conditional | 4501.0 / 1343.5 | 1838.4/2.45; 457.7/2.94 | 0.54 [0.51–0.56]; 0.14 [0.14–0.15] | 15.3 / 23.7 |
| includes | 775.3 / 388.8 | 315.6/2.46; 164.8/2.36 | 0.09 [0.09–0.09]; 0.05 [0.05–0.05] | 6.3 / 18.9 |
| aliases | 1007.2 / 1802.5 | 387.8/2.60; 724.6/2.48 | 0.11 [0.11–0.18]; 0.27 [0.27–0.27] | 5.0 / 111.1 |
| nested | 428.7 / 22730.1 | 188.3/2.28; 9545.2/2.38 | 0.06 [0.06–0.06]; 4.01 [4.00–4.03] | 5.7 / 1553.2 |
| probes1000 | 31.5 / 24.1 | 16.6/1.90; 15.7/1.54 | 0.01 [0.01–0.01]; 0.01 [0.01–0.01] | 6.9 / 12.0 |
| probes4000 | 118.8 / 49.7 | 62.0/1.91; 25.7/1.93 | 0.03 [0.03–0.03]; 0.01 [0.01–0.01] | 18.0 / 14.1 |
| probes16000 | 467.7 / 152.1 | 249.0/1.88; 65.9/2.31 | 0.12 [0.11–0.12]; 0.03 [0.03–0.03] | 61.8 / 21.7 |

`text`: 300k ordinary tokens; `lookup`: 120k definitions + 100k tokens;
`arguments`: 70k eight-use invocations; `pastes`: 200k concatenations;
`deep`: 30k function-alias chain + four substitutions; `conditional`: 85k
mixed `defined`, macro value and lazy-error expressions; `includes`: 60k once
include attempts + 60k output tokens; `aliases`: repeated 100-deep object chain;
`nested`: 1k identity nesting repeated 3k times; probes: qualified
`__has_cpp_attribute` operands at 1k/4k/16k scale. Course/Clang scale limitations
are retained below; common timing never substitutes a smaller student workload.
Clang/reference costs and separate dump costs are in the same summary, not lost.

Primary repeat spread (student only; all host spreads are in raw/summary JSON):

| family | instructions M min–max | cycles M min–max | IPC min–max |
|---|---:|---:|---:|
| text | 763.432–764.932 | 305.8–306.5 | 2.49–2.50 |
| lookup | 1384.094–1384.094 | 577.2–580.7 | 2.38–2.40 |
| arguments | 1837.590–1840.391 | 733.1–757.5 | 2.43–2.51 |
| pastes | 2737.741–2738.741 | 1056.6–1065.8 | 2.57–2.59 |
| deep | 583.723–583.723 | 323.1–331.0 | 1.76–1.81 |
| conditional | 4501.001–4501.426 | 1829.0–1859.7 | 2.42–2.46 |
| includes | 775.278–775.278 | 315.2–317.7 | 2.44–2.46 |
| aliases | 1007.249–1007.249 | 387.4–392.6 | 2.57–2.60 |
| nested | 428.713–428.713 | 187.0–191.1 | 2.24–2.29 |
| probes1000 | 31.545–31.545 | 16.5–16.7 | 1.88–1.91 |
| probes4000 | 118.785–118.785 | 60.7–63.2 | 1.88–1.96 |
| probes16000 | 467.743–467.743 | 236.6–250.0 | 1.87–1.98 |

### A/B profitability and rejected alternatives

Against baseline on the same bytes, 85k conditions change 4732.8M → 4501.0M
instructions (**−4.9%**), cycles 1905.0M [1897.1–1992.2] → 1838.4M
[1829.0–1859.7] (**−3.5%**), IPC 2.48 → 2.45. The two ranges do not overlap;
`audit-conservative` and scaled `conservative-complete` corroborate cycle/work
benefit. Wall .55 [.53–.55] → .54 [.51–.56] overlaps: **latency improvement is
inconclusive**. This fixes unneeded directive indexing/copying and repeated
conditional scratch allocation, not semantic shortcuts. Includes fall 803.7M →
775.3M instructions and 327.4M → 315.6M cycles; aliases increase 990.1M →
1007.2M instructions for correct provenance but improve cycles 405.6M → 387.8M.
Arguments retire ~1.5% fewer instructions but cycle/wall ranges overlap. Paste
rises ~0.6% instructions and ~0.7% cycles; deep/nested timing differences are
inconclusive. No uniform-speedup claim is made. Provenance bookkeeping is
correctness-required, not an optional profitable transform.

All trial labels under `pa4-perf/` are preserved: `audit-base`, `audit-trial`,
`audit-stream`, `audit-scratch`, `audit-ascii`, `audit-final`, `audit-accepted`,
`audit-signoff`, `audit-serial`, `audit-release`, `audit-conservative`, and
`audit-certified`, in addition to prior checkpoint labels below.
`audit-trial`'s ring-buffer experiment regressed text 764.9M → 785.6M and
conditional 4735.2M → 4816.3M instructions; it was rejected.
`audit-release` included an ASCII lexer fast path: text instructions 692.6M
instead of ~765M but cycles 337.6M instead of baseline ~320M; paste 1135.3M
instead of 1042M cycles in its paired run. It reduced IPC and repeatedly
increased cycles. That fast path was **reverted** before acceptance. Frozen
observations remain; they are not final-source performance claims.
`audit-conservative`'s lookup wall/cycles were noisy (.20–.33s, 604–1097M cycles,
unchanged instructions), while its paired baseline also varied .18–.34s.
`audit-certified` remeasures the unchanged binary serially: .19s and
577–581M cycles. Both runs remain raw evidence; no lookup speedup is claimed.

### Scale, slowest suite test and memory work model

Independent suite timing (`pa4-audit/suite-times-base.json`) identifies
`directives/600-repeated-argument-expansion.t` as slowest (~.038s), followed by
include work (~.004s). It is measured on exactly the same source with the host,
not compared to a startup-only empty file. Additional fixed scales are frozen
in `pa4-audit/scale-qualified/inputs.json`; all five student/baseline/host/ref
variants have exact parity. Measurements/profiles/branch/cache counters are
`pa4-audit/conservative-complete/`; its frozen cursor SHA256 is identical to
`audit-certified` (`e4318a0dc0546e7f6aab631895d162748defed364f3087d0b1da22b7ddc9f1d0`).
Headers have distinct contents to avoid GCC coalescing identical pragma-once
files. The initial identical-header parity failure (`release-supplement`) is
preserved; it is not a valid same-work comparison. `release-qualified` stopped
at an incorrectly specified profile path after completing scale counts; its
raw counts remain. `release-complete` profiles the rejected ASCII build.

| supplemental fixed family | instructions student / GCC M | cycles/IPC student; GCC | wall student; GCC s [range] | RSS student / GCC MiB |
|---|---:|---:|---:|---:|
| conditional21000 | 1114.7 / 343.8 | 458.1/2.43; 123.3/2.79 | 0.16 [0.14–0.16]; 0.05 [0.05–0.06] | 6.3 / 14.1 |
| conditional85000 | 4501.0 / 1343.5 | 1858.1/2.42; 459.2/2.93 | 0.60 [0.60–0.61]; 0.16 [0.15–0.17] | 15.6 / 23.7 |
| conditional340000 | 18006.2 / 5327.6 | 7627.1/2.36; 1798.0/2.96 | 2.30 [2.16–2.33]; 0.57 [0.56–0.59] | 50.7 / 63.5 |
| replacement1000 | 29.3 / 30.2 | 16.3/1.80; 16.5/1.83 | 0.01 [0.00–0.01]; 0.01 [0.01–0.01] | 5.0 / 11.3 |
| replacement4000 | 109.0 / 73.9 | 57.2/1.90; 30.9/2.39 | 0.03 [0.03–0.03]; 0.01 [0.01–0.01] | 5.7 / 12.8 |
| replacement16000 | 427.8 / 248.4 | 224.3/1.91; 87.2/2.85 | 0.07 [0.07–0.08]; 0.03 [0.03–0.03] | 12.4 / 18.2 |
| headers2000 | 110.4 / 155.3 | 54.8/2.02; 166.6/0.93 | 0.04 [0.04–0.04]; 0.08 [0.08–0.08] | 5.0 / 13.4 |
| headers8000 | 437.0 / 1853.7 | 225.7/1.94; 2071.4/0.89 | 0.15 [0.15–0.17]; 0.76 [0.73–0.76] | 8.4 / 18.9 |
| slowest-fixture | 155.1 / 219.5 | 78.3/1.98; 104.6/2.10 | 0.03 [0.03–0.03]; 0.07 [0.07–0.07] | 20.1 / 78.7 |

Conditions grow ~4x instructions for 4x work (1114.7/4501.0/18006.2M) and
memory follows immutable source size, not accumulated expression graphs. Long
replacement cost (29.3/109.0/427.8M) is linear in emitted replacement work;
2k/8k headers grow 110.4/437.0M instructions. The slowest fixture emits 64 tokens
from an 8k chain: 8,002 invocations, one argument span, 24,070 indexed tokens,
two task frames. Prescan occurs once, not 64 chain replays. Student .03s/20.1MiB
versus GCC .07s/78.7MiB; no unmeasured slowest-test algorithm remains.
The inherited 100k nesting, arbitrary-depth, repeated-paint and qualified probe
controls remain mandatory. No timeouts are work budgets; no numeric target is
reclassified. Residual conditional ~3.35x instructions/~4x wall and 16k probes
~3.08x instructions/~4x wall vs GCC are bounded but still important tuning
opportunities; fixes and attribution are required even though fixtures pass.

### Profiles, phase counters and attribution

Before-fix profiles: `pa4-audit/base-{conditional,lookup,deep,probes16000}.report`
and corresponding `.perf.data`. After-fix: every fixed family has
`pa4-audit/conservative-complete/<family>.{perf.data,report}`; focused
`.self.report` files remove children double-counting. Task-scoped user-mode
`cycles:u`, 999 Hz, DWARF call stacks, separate from stat comparisons; the slowest
fixture is repeated ten times inside the sampled task. Zero lost samples;
607 conditional / 191 lookup / 192 deep / 395 paste / 126 probes / 237 headers /
333 slowest-fixture samples, with useful symbols and caller paths. Sampling is
higher-frequency than the spec's 99-Hz example to make short workloads useful;
no profile overhead is included in comparisons. Sparse/unknown addresses are
not interpreted as precise source attribution.

| trigger | largest sampled self costs, approximate | ownership resolution |
|---|---|---|
| 85k conditions, largest host multiple | Lexer peek 18.4%, macro next 9.4%, raw 8.8%; parser/conversion, operand storage and reductions distributed | `MacroEngine::expand_owned` no unconditional sequence index; `Preprocessor::Impl::condition`, `PostCursor::reset`, `ControllingExpression::restart` reuse/reset scratch; typed lazy evaluation unchanged |
| many definitions | Lexer peek 33%, raw 9%, Lexer next 8.5% | dense macro binding/positional parameter and flat identifier tables verified; no per-lookup TU scan; rejected lexer shortcuts retained rather than forcing regression |
| deep chain/slowest fixture | deep Lexer peek 14%, paint insertion 9%; fixture Lexer peek 16%, macro next 9%, paint insertion 8% | fixed-depth radix paint, cached extensions, once-indexed spans and explicit tasks uphold work bound; no descendant replay |
| paste | Lexer peek 18%, raw 12%, macro next 11% | raw paste → one-token generated lexing → rescan; invocation/part allocations bulk-owned; small required provenance overhead disclosed |
| 16k probes | Lexer peek 15%, condition 9%, operand growth 8% | linear `compact_attribute_probes`; source/operand retention proportional to actual bytes; checkpoint quadratic algorithm removed and preserved paired proof below |
| 8k distinct headers | Lexer peek 31%, identifier intern 6%, take 6% | flat device/inode once set, indexed paths, completed lexer release; no owning hash node per header |

`pa4-audit/final-phase.stderr`: 6,205,015 source bytes, 3,315,008 source tokens,
85k demanded conditions, 935k expression facts/reductions combined, 425k actual
reductions, max live expression facts/operators **4/4**, 34 paint nodes,
84,999 paint extension hits, **zero indexed directive tokens**. Explicit enabled
expression parse/evaluate phase times ~.074/.002s in the separate run. Timers
are disabled in PMU comparisons to avoid perturbing work; zeros there mean
telemetry disabled, not zero expression work.

Separate three-repeat groups `{branches:u,branch-misses:u}` and
`{cache-references:u,cache-misses:u}` on conditional/lookup/paste/probes/includes
for student/baseline/GCC are all **100% running**. Exact raw events and min/median/
max summaries are `conservative-complete/branch-cache-summary.json`.
Conditional branches fall ~826.7M → 771.4M; branch misses ~2.04M → 2.20M versus
GCC .28M. LLC-reference/miss observations vary with cache state; e.g. conditional
student median 444k/93k versus baseline 431k/94k. No small cache-benefit claim
is based on that noise. Xeon generic cache events describe LLC activity, not
all L1 accesses; these are same-machine attribution checks, not portable rates.

Compiler binary text grows 209,734 → 215,991 bytes (~3%) for the ownership and
provenance fixes; total debug binary 4,312,808 → 4,378,960 bytes.
`pa4-audit/final-compiler-size.txt` retains `size` output (the ordinary rendered
preproc tool has 240,809 text bytes). These are compiler sizes, **not generated
program text/object size**. Native loops/calls/memory/FP/self-host executable
controls do not exist at this stage and must not be falsely certified. Hosted
header/template frontends are later-stage capabilities; PA4 includes exercise
course headers. Later runtime/text/object and PA33/34 instruction gates remain.

## Preserved checkpoint evidence (historical, not final-source tables)

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

## Delivery reproducibility run (same accepted binaries)

After consolidation, the complete frozen protocol was repeated as
`$RALPH_ARTIFACT_DIR/pa4-perf/audit-delivery` against `audit-base`, followed by
`pa4-audit/delivery-complete` (all scales, profiles and branch/cache passes).
Its cursor SHA256 is identical to `audit-certified`:
`e4318a0dc0546e7f6aab631895d162748defed364f3087d0b1da22b7ddc9f1d0`.
This is additional evidence, not replacement of the accepted raw observations.
Both repeat sets' events ran at 100%; all comparisons retained parity and the
same CPU-0 affinity, flags, warmup and interleaving. The primary run contains
504 numeric event observations; supplemental runs contain 450. Manifests and
raw files remain in those directories.

Instruction medians reproduce the accepted work counts. Conditional cycles
repeat at 1828.6M (1808.4–1933.3M), versus paired baseline 1964.4M
(1906.1–2127.9M); IPC median 2.46. Wall .58s (.51–.60), versus baseline .55s
(.53–.65), does **not** establish latency improvement. Includes cycles are
313.9M (313.0–314.0), versus baseline 339.4M (324.9–351.8); aliases 383.1M
(380.5–389.9), versus 409.4M (406.1–410.8). These confirm reduced work/cycle
medians, not universal speedups. Nested cycles remain a small regression with
overlapping ranges: 189.0M (188.4–192.5), versus 186.5M (182.7–196.0).

Text shows a conspicuous cycle outlier: median 313.6M, range 303.9–589.8M,
while instruction counts remain 763432010 (763431951–763432029). Wall spans
.09–.18s. This reproduces no source/work-count change; separate text profiles
still attribute work to the unchanged lexer/lookahead path. It is retained as
machine timing spread, not deleted or used to claim a benefit. The earlier
`tuning`/ASCII trial remains rejected on its multi-family cycle regressions.

Supplemental 21k/85k/340k condition instruction medians reproduce
1114.6/4501.0/18006.2M; wall ranges .13–.15/.50–.54/2.09–2.23s.
Distinct once-header 2k/8k medians reproduce 110.4/437.0M; wall .04s and
.14–.17s. The slowest fixture remains 155.1M/.03s/~20.1MiB versus GCC
219.5M/.07s/~78.7MiB. Separate final `cycles:u` DWARF profiles and branch/cache
passes are retained alongside raw counts. No new algorithmic or semantic
finding arose; work budgets and later-stage gates are unchanged.
