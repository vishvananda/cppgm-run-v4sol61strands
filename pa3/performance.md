# PA3 measured evidence — October 4, 2026

Implementation: `0d170da6` (measurement/control-only and documentation changes
follow). Raw artifact root:
`/home/vishvananda/work/private/v4sol61strands/artifacts/`.

## Protocol and comparison boundary

`student.tests/pa3/measure.py` freezes input/binary SHA256, GCC/Clang versions,
flags, commands, CPU model, checks exact output before timing and verifies the
non-rendering cursor checksum against every independently generated expected
value. CPU 0, Xeon E5-2696 v4; one warmup, three interleaved task-scoped
`perf stat {instructions:u,cycles:u}` repeats; O3/g C++11 student binaries.
Every counter was >=95% running; retain raw scaling/time-running fields.
GCC/Clang use C++11 `-E -P`; their #if envelopes contain **identical expression
text**, validate every value and emit matching output. Host directive work and
student per-line rendering are explicit differences, not full C++ compiler
parity. PA3 excludes directives/macros/hosted declarations. PA2 inherited
hosted sources are measured separately below; don't mislabel them PA3 support.

`pa3-perf/{initial,final,signoff}-{manifest,observations,summary,parity}.json`
and all individual `*.perf`, `*.time`, `*.telemetry` are retained, with frozen
binaries. Signoff compares initial and final binaries within each repeat.
Initial/final runs overlapped inherited measurements and other validation;
signoff ran without those jobs. Wall timing across those sessions changed
substantially with scheduling/frequency; no cross-session latency improvement
is asserted. Instructions agree; the table uses signoff only. Counts are user
mode; wall/user/system and RSS are all retained separately in raw records.

Generated-program runtime, text/object size: **N/A**, PA3 emits expression
values, not native executables. These are compiler/frontend measurements, not
backend program measurements. Future PA33/34 per-workload 1.25x GCC instruction
limit remains mandatory; these early diagnostic ratios do not pass or waive it.

## Stage-supported results

Cells are median [min,max], instructions/cycles in millions, RSS MiB.
Arithmetic: 150k varied lines; lazy: 120k mixed lazy/type operations; chain:
300k terms, one line; nested: 15k expressions, 128 parentheses each;
identifiers: 120k lines with 120003 total distinct interned identifiers.

| Workload | Variant | Instructions M | Cycles M | IPC | Wall s | RSS MiB |
|---|---|---:|---:|---:|---:|---:|
| arithmetic | ppexpr | 2276.157 [2276.146,2276.183] | 1010.263 [1005.443,1010.318] | 2.253 [2.253,2.264] | 0.280 [0.270,0.280] | 7.598 [7.586,7.598] |
| arithmetic | gcc | 2811.076 [2811.076,2811.076] | 1065.847 [1064.044,1065.970] | 2.637 [2.637,2.642] | 0.300 [0.300,0.300] | 26.449 [26.438,26.488] |
| arithmetic | clang | 2808.952 [2808.904,2812.663] | 1055.729 [1042.311,1082.470] | 2.664 [2.595,2.695] | 0.320 [0.310,0.320] | 96.152 [96.133,96.160] |
| arithmetic | reference | 2541.774 [2541.774,2541.774] | 1005.741 [1004.683,1006.970] | 2.527 [2.524,2.530] | 0.280 [0.280,0.300] | 7.676 [7.664,7.699] |
| lazy | ppexpr | 3757.448 [3757.440,3759.597] | 1639.290 [1632.872,1647.282] | 2.292 [2.281,2.302] | 0.470 [0.460,0.490] | 11.336 [11.301,11.340] |
| lazy | gcc | 2725.467 [2725.467,2725.467] | 921.127 [917.799,928.244] | 2.959 [2.936,2.970] | 0.280 [0.280,0.290] | 27.121 [27.109,27.125] |
| lazy | clang | 3156.553 [3156.498,3156.580] | 1220.797 [1190.649,1248.445] | 2.586 [2.528,2.651] | 0.370 [0.360,0.410] | 97.336 [97.207,97.371] |
| lazy | reference | 4136.998 [4136.998,4138.918] | 1635.377 [1626.196,1663.189] | 2.530 [2.489,2.544] | 0.480 [0.460,0.480] | 11.352 [11.328,11.359] |
| chain | ppexpr | 584.270 [584.270,584.270] | 283.512 [282.243,283.834] | 2.061 [2.058,2.070] | 0.130 [0.130,0.130] | 52.977 [52.949,52.977] |
| chain | gcc | 313.167 [313.167,313.167] | 113.586 [113.185,114.956] | 2.757 [2.724,2.767] | 0.050 [0.050,0.050] | 30.645 [30.570,30.672] |
| chain | clang | 335.524 [335.521,335.532] | 138.976 [138.640,139.364] | 2.414 [2.408,2.420] | 0.050 [0.050,0.050] | 76.031 [75.973,76.066] |
| chain | reference | 715.689 [715.689,715.689] | 275.139 [274.543,276.164] | 2.601 [2.592,2.607] | 0.130 [0.120,0.130] | 50.102 [50.051,50.398] |
| nested | ppexpr | 2289.113 [2289.113,2289.113] | 965.981 [951.785,1014.791] | 2.370 [2.256,2.405] | 0.270 [0.270,0.300] | 11.332 [11.328,11.340] |
| nested | gcc | 1602.847 [1602.847,1602.851] | 461.956 [461.907,462.225] | 3.470 [3.468,3.470] | 0.140 [0.130,0.140] | 16.168 [16.125,16.176] |
| nested | clang | 1039.060 [1039.059,1039.061] | 413.931 [407.123,424.036] | 2.510 [2.450,2.552] | 0.130 [0.130,0.140] | 83.559 [83.543,83.562] |
| nested | reference | 3461.212 [3461.212,3461.212] | 1148.555 [1144.749,1178.389] | 3.014 [2.937,3.024] | 0.320 [0.320,0.320] | 11.363 [11.188,11.367] |
| identifiers | ppexpr | 2085.350 [2085.337,2086.115] | 923.675 [921.260,928.873] | 2.258 [2.245,2.264] | 0.270 [0.260,0.300] | 14.656 [14.645,15.277] |
| identifiers | gcc | 1478.316 [1478.315,1478.316] | 548.603 [543.440,548.645] | 2.695 [2.694,2.720] | 0.170 [0.160,0.180] | 33.324 [33.312,33.395] |
| identifiers | clang | 2184.621 [2184.604,2185.642] | 883.844 [865.649,896.106] | 2.472 [2.438,2.525] | 0.290 [0.270,0.290] | 102.793 [102.785,102.805] |
| identifiers | reference | 1892.360 [1892.360,1893.560] | 701.657 [686.869,750.258] | 2.699 [2.522,2.755] | 0.240 [0.190,0.240] | 11.371 [11.316,11.383] |

## Owning hot paths, measured change and bounded residual costs

The initial nested profile attributed 26% to `classify_simple` (44 samples,
exploratory). `classify_simple` now directly dispatches one-character grammar
terminals instead of hashing them. Frozen signoff final/initial instruction
ratios: arithmetic 0.964, lazy 0.962, chain 0.961, nested 0.875, identifiers
0.986. Cycles improved, but nested baseline had substantial variation; its
larger cycle delta is not a precision claim. Identifier wall spread overlaps:
no latency win claimed there. No unaffected workload is silently discarded.

User-mode profiles: `pa3-perf/signoff-{lazy,chain,nested,identifiers}.data` and
matching `*-profile.txt`/`*-record.txt`, 5 executions sampled separately at
199 Hz with DWARF stacks; 122–483 samples, no lost samples. Some chain samples
have unresolved addresses; do not attribute these. Resolved useful stacks show
lexer `peek/next/take`, PostCursor conversion and typed parser/evaluator, not
TU scans/reparses/per-node allocation. Nested still spends most time decoding
parentheses. Its 1.43x GCC instructions / ~1.93x latency and chain's 1.87x
instructions / 2.60x latency are bounded costs of phase-1/2/3 processing and
explicit typed node construction versus host preprocessor direct evaluation,
not a scalability exemption. Chain peak 53 MiB versus GCC 31 MiB; 599999 nodes
and 300000 evaluator frames, proportional to its one unusually long line.
Representative 100k deep parentheses, unary chains and nested conditionals
complete without native stack recursion. Ordinary lines retain only max 5–16
nodes; all vectors are reused, nodes do not grow with line count.

Slowest required fixture: `300-triple.t`, 492075 lines. Three pinned warm runs
in `pa3-perf/slowest-triple-{manifest,observations}.json`: student 7.569B
instructions, 3.297B cycles, IPC 2.296, wall 1.06 [1.04,1.08]s, 18.71 MiB RSS;
reference 5.980B instructions, 2.391B cycles, IPC 2.501, wall
0.78 [0.74,0.78]s, 18.76 MiB RSS. Profile `slowest-triple.data`/profile text:
215 samples, none lost, frontend lexer/conversion dominates. Host qualification
of course char16/char32 promotion is not equivalent: course promotes them to
unsigned 64 bits; host PP promotions differ. Portable same-source hosts are
compared above instead of calling this gap a failed local edit.

## Retained PA2 controls / large hosted source

`student.tests/pa2/measure.py ... --label pa3-signoff --against audit-signoff`
retains all five fixed inputs, old and new cursor/posttoken, GCC and Clang,
semantic and preprocessing comparisons, exact output parity and every
observation. Raw: `pa2-perf/pa3-{inherited,final,signoff}-*.{json,perf,time,telemetry}`.
Rows below expose the hosted comparison boundary: hosts' syntax-only phase
consumes more semantic work, hosts' -E consumes less literal conversion.

| Workload | Variant | Instructions M | Cycles M | IPC | Wall s | RSS MiB |
|---|---|---:|---:|---:|---:|---:|
| hosted | posttoken | 4028.034 [4028.034,4028.034] | 1740.208 [1728.417,1752.512] | 2.315 [2.298,2.330] | 0.510 [0.490,0.520] | 14.570 [14.570,14.664] |
| hosted | posttoken-baseline | 4114.812 [4114.812,4114.812] | 1781.021 [1772.365,1912.537] | 2.310 [2.151,2.322] | 0.540 [0.540,0.570] | 14.566 [14.520,14.574] |
| hosted | gcc | 11852.689 [11849.692,11854.244] | 12041.767 [11929.603,12109.923] | 0.984 [0.979,0.994] | 3.960 [3.870,4.040] | 731.332 [731.309,731.340] |
| hosted | gcc-lex | 1603.603 [1603.602,1603.603] | 573.127 [572.959,574.341] | 2.798 [2.792,2.799] | 0.180 [0.170,0.180] | 24.371 [24.367,24.387] |
| hosted | clang-lex | 1454.508 [1454.507,1454.510] | 580.231 [578.825,582.101] | 2.507 [2.499,2.513] | 0.180 [0.180,0.200] | 87.359 [87.254,87.441] |

Clang semantic compilation of the same GCC-expanded system headers remains
**inconclusive**, not passing: GCC extensions exceed portable C++11 (prior PA2
capability diagnosis preserved). GCC semantics and both hosts' preprocessing
succeed. No references changed: documented extension-only differences remain
in parity records. On all five PA2 workloads final/baseline posttoken retired
instructions remain 0.979–1.002x; concat's +0.18% and overlapping wall spread
are disclosed, not called a speedup. No growing cost or RSS regression.

## Validation and architecture review boundary

Required PA3 20/20, cumulative PA1–3 100/100, earlier PA1–2 80/80; file audit
33 files. Personal PA1, PA2, PA2 independent-audit controls rerun explicitly;
PA3 semantic/deep/scaling/random controls pass with GCC and Clang-qualified
native assertions and PP expressions. Clang-built PA3 passes, ASan+UBSan PA3
passes; logs/binaries in `pa3-validation/`. Backend future portable controls
qualified with both hosts; all 8 deliberate failure mutations detected,
`pa3-backend-qualification.json`; not a premature native-backend gate pass.

Independent whole-stage review remains Ralph-owned. Known unfinished PA3
implementation: none. Audit questions: broad capability/cost sufficiency,
compact per-line graph and canonical identity ownership, inherited hosted
coverage, and any undiscovered semantic edge cases. No review question is
waived by these measurements or fixtures.
