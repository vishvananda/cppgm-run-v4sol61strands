# PA3 measured evidence — October 4, 2026

Historical implementation for the original measurements: `0d170da6`. The final
independent-audit appendix supersedes that design and signoff; earlier raw
observations remain preserved. Raw artifact root:
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

## Final independent audit — October 5, 2026

This appendix supersedes the implementation handoff conclusions, not its raw
measurements. All experiments and binaries remain under the same artifact root.
Final source is the audit commit with this appendix; the `audit-handoff` O3/g
binaries include the final typed-fact algorithm, punctuation filter and metrics.

Protocol: Xeon E5-2696 v4, CPU 0, one unmeasured warmup, three interleaved
task-scoped repeats of `{instructions:u,cycles:u}` at identical affinity, input
and environment. Raw perf files include time-running and running percentage.
All primary and separate branch/cache groups ran 100%; no scaled or unavailable
event is treated as zero. Flags, input/executable/host/cc1plus hashes, versions,
commands, parity and every wall/user/system/RSS observation are in manifests.
Use same-session pairs only. No compiler job ran alongside final measurements.

### Fixed PA3 compiler workloads

Raw: `pa3-perf/audit-handoff-{manifest,observations,summary,parity}.json` and
`audit-handoff-*.{perf,time,telemetry}`. Input definitions remain in `measure.py`.
Host #if envelopes contain identical expression text, verify every value and emit
the same result lines; directive work is a disclosed difference. `cursor` checks
an independently computed value checksum without rendering. Compiler and
generated-executable costs are not conflated. Cells are median [min,max];
instructions/cycles are millions, peak RSS is MiB. User/system are preserved raw.

| Workload | Variant | Instructions M | Cycles M | IPC | Wall s | RSS MiB |
|---|---|---:|---:|---:|---:|---:|
| arithmetic | ppexpr | 2136.801 [2136.801,2136.801] | 929.624 [924.359,933.133] | 2.299 [2.290,2.312] | 0.260 [0.260,0.270] | 7.641 [7.617,7.648] |
| arithmetic | ppexpr-baseline | 2276.146 [2276.146,2276.157] | 1010.130 [1003.134,1039.103] | 2.253 [2.191,2.269] | 0.280 [0.270,0.280] | 7.586 [7.570,7.645] |
| arithmetic | cursor | 2008.349 [2008.349,2008.349] | 851.487 [850.032,868.959] | 2.359 [2.311,2.363] | 0.230 [0.230,0.270] | 7.641 [7.637,7.645] |
| arithmetic | gcc | 2811.076 [2811.076,2811.076] | 1068.595 [1064.863,1083.630] | 2.631 [2.594,2.640] | 0.310 [0.310,0.330] | 26.480 [26.414,26.488] |
| arithmetic | clang | 2811.819 [2808.949,2812.282] | 1064.633 [1045.788,1073.352] | 2.641 [2.620,2.686] | 0.320 [0.310,0.340] | 96.152 [96.055,96.172] |
| arithmetic | reference | 2541.774 [2541.774,2541.774] | 1006.725 [1002.654,1008.951] | 2.525 [2.519,2.535] | 0.280 [0.270,0.310] | 7.668 [7.664,7.695] |
| lazy | ppexpr | 3601.782 [3601.782,3602.497] | 1607.852 [1605.520,2228.074] | 2.240 [1.617,2.243] | 0.460 [0.440,0.640] | 11.320 [11.309,11.328] |
| lazy | ppexpr-baseline | 3758.137 [3757.417,3758.151] | 1641.895 [1639.170,1653.048] | 2.289 [2.273,2.293] | 0.460 [0.450,0.460] | 11.332 [11.277,11.340] |
| lazy | cursor | 3505.979 [3505.979,3505.979] | 1549.157 [1542.549,1554.669] | 2.263 [2.255,2.273] | 0.430 [0.420,0.490] | 11.316 [11.309,11.355] |
| lazy | gcc | 2725.467 [2725.467,2725.468] | 921.538 [917.101,1810.777] | 2.958 [1.505,2.972] | 0.280 [0.260,0.520] | 27.156 [27.137,27.172] |
| lazy | clang | 3156.535 [3156.494,3156.548] | 1235.162 [1206.326,1236.816] | 2.556 [2.552,2.617] | 0.390 [0.350,0.390] | 97.344 [97.262,97.363] |
| lazy | reference | 4136.998 [4136.998,4138.438] | 1626.892 [1623.498,1638.131] | 2.543 [2.525,2.549] | 0.460 [0.450,0.480] | 11.367 [11.273,11.367] |
| chain | ppexpr | 535.380 [535.380,535.380] | 213.205 [207.436,217.541] | 2.511 [2.461,2.581] | 0.060 [0.050,0.060] | 4.941 [4.922,5.004] |
| chain | ppexpr-baseline | 584.270 [584.270,584.270] | 300.595 [286.809,303.748] | 1.944 [1.924,2.037] | 0.110 [0.100,0.120] | 52.922 [52.836,52.977] |
| chain | cursor | 535.346 [535.346,535.346] | 212.533 [210.276,221.147] | 2.519 [2.421,2.546] | 0.060 [0.050,0.060] | 4.273 [4.270,4.273] |
| chain | gcc | 313.167 [313.167,313.168] | 112.673 [112.574,115.889] | 2.779 [2.702,2.782] | 0.040 [0.040,0.040] | 30.559 [30.516,30.652] |
| chain | clang | 335.525 [335.521,335.526] | 140.292 [140.027,140.364] | 2.392 [2.390,2.396] | 0.040 [0.040,0.040] | 76.035 [75.992,76.051] |
| chain | reference | 715.689 [715.689,715.689] | 275.468 [274.206,276.664] | 2.598 [2.587,2.610] | 0.110 [0.110,0.110] | 50.305 [50.273,50.391] |
| nested | ppexpr | 2049.524 [2049.524,2049.525] | 1093.385 [1045.621,1105.862] | 1.874 [1.853,1.960] | 0.320 [0.290,0.320] | 11.281 [11.180,11.312] |
| nested | ppexpr-baseline | 2289.113 [2289.113,2289.114] | 956.532 [945.677,964.464] | 2.393 [2.373,2.421] | 0.270 [0.260,0.290] | 11.336 [11.332,11.340] |
| nested | cursor | 2032.161 [2032.161,2032.161] | 1043.556 [1000.266,1060.344] | 1.947 [1.917,2.032] | 0.320 [0.300,0.330] | 11.324 [11.320,11.328] |
| nested | gcc | 1602.847 [1602.847,1602.848] | 460.670 [460.424,462.231] | 3.479 [3.468,3.481] | 0.130 [0.130,0.140] | 16.172 [16.043,16.176] |
| nested | clang | 1039.062 [1039.062,1039.065] | 409.433 [408.851,411.199] | 2.538 [2.527,2.541] | 0.130 [0.120,0.130] | 83.586 [83.574,83.648] |
| nested | reference | 3461.212 [3461.212,3461.212] | 1148.660 [1145.447,1149.945] | 3.013 [3.010,3.022] | 0.320 [0.320,0.330] | 11.348 [11.289,11.367] |
| identifiers | ppexpr | 2029.428 [2029.428,2029.435] | 904.729 [887.998,916.423] | 2.243 [2.215,2.285] | 0.260 [0.250,0.260] | 15.273 [14.688,15.273] |
| identifiers | ppexpr-baseline | 2085.350 [2085.342,2085.360] | 923.291 [921.279,923.996] | 2.259 [2.257,2.264] | 0.260 [0.260,0.290] | 14.676 [14.602,15.215] |
| identifiers | cursor | 1933.638 [1933.638,1933.653] | 850.401 [840.675,1248.966] | 2.274 [1.548,2.300] | 0.270 [0.230,0.350] | 15.266 [15.234,15.270] |
| identifiers | gcc | 1478.316 [1478.315,1478.317] | 548.314 [544.565,551.168] | 2.696 [2.682,2.715] | 0.160 [0.160,0.180] | 33.344 [33.320,33.402] |
| identifiers | clang | 2184.720 [2184.657,2184.915] | 889.666 [885.146,910.380] | 2.456 [2.400,2.468] | 0.280 [0.270,0.290] | 102.789 [102.762,102.816] |
| identifiers | reference | 1891.640 [1891.640,1892.360] | 685.952 [684.265,699.288] | 2.758 [2.706,2.764] | 0.200 [0.190,0.200] | 11.258 [11.188,11.375] |

Final / entry instruction ratios for arithmetic/lazy/chain/nested/identifiers:
0.9388, 0.9584, 0.9163, 0.8953, 0.9732.
Final / GCC ratios: 0.7601, 1.3215, 1.7096, 1.2787, 1.3728.
Chain latency/RSS improve strongly; arithmetic cycles also improve. Lazy and
identifier wall ranges overlap: do not claim statistically established latency
benefit. Nested **regresses** relative to entry in cycles/latency despite fewer
instructions (1.874 vs 2.393 IPC). Its 0.29–0.32s range vs GCC 0.13–0.14s is
a bounded 2.46x median, not GCC parity. The `audit-final`, `audit-fold`,
`audit-compact`, `audit-dispatch`, `audit-early-punct`, `audit-signoff`, and
`audit-accepted` sessions preserve intermediate and confirmation results.
Early punctuation filtering improved nested instructions 6.69% and cycles 13.54%
against the immediately preceding build; final range variation does not establish
a net entry-relative nested latency win. The source-faithful constant facts and
status returns are retained for correctness/architecture plus chain/error benefit.

### Slowest suite fixture, error recovery and growth

`pa3-audit/fixture-times.json` independently times all 100 fixtures. The 12,039,435
byte `300-triple.t` dominates (~0.74s initial scan); the next largest cost is
PA2 concat (~0.024s), while startup dominates remaining milliseconds. Focused
measurements below use `pa3-audit/focused-audit-handoff/`:
`focused-{manifest,observations,summary}.json`, all raw `*.perf/*.time/*.stderr`,
and frozen inputs/expected outputs. Full triple has course-specific mock-defined
and character promotions, so no host-equivalent full-fixture result is claimed;
portable triple excludes those cases and shifts and preserves source expression
text. Malformed host #if diagnostic recovery is not equivalent output and exits
nonzero; it is informational, not a claimed performance win over equivalent work.

| Workload | Variant | Instructions M | Cycles M | IPC | Wall s | RSS MiB |
|---|---|---:|---:|---:|---:|---:|
| portable-triple | before | 1172.009 [1171.814,1172.051] | 519.157 [515.107,521.192] | 2.257 [2.249,2.275] | 0.160 [0.140,0.170] | 7.594 [7.586,7.641] |
| portable-triple | after | 1111.352 [1111.352,1111.547] | 491.405 [480.370,497.972] | 2.262 [2.232,2.314] | 0.150 [0.140,0.160] | 7.645 [7.645,7.648] |
| portable-triple | gcc | 1406.717 [1406.716,1406.718] | 517.975 [517.716,1099.212] | 2.716 [1.280,2.717] | 0.160 [0.160,0.330] | 18.238 [18.195,18.277] |
| portable-triple | clang | 2207.982 [2207.982,2207.997] | 1016.499 [1013.309,1433.224] | 2.172 [1.541,2.179] | 0.730 [0.720,1.030] | 90.562 [90.504,90.652] |
| portable-triple | reference | 1266.400 [1266.396,1266.403] | 587.704 [564.993,610.820] | 2.155 [2.073,2.241] | 0.190 [0.170,0.210] | 8.445 [8.418,8.492] |
| invalid | before | 2237.869 [2237.569,2237.869] | 832.881 [822.403,844.075] | 2.687 [2.651,2.721] | 0.230 [0.220,0.230] | 4.227 [4.219,4.281] |
| invalid | after | 402.910 [402.910,402.910] | 161.964 [161.612,166.058] | 2.488 [2.426,2.493] | 0.050 [0.040,0.060] | 4.270 [4.219,4.289] |
| invalid | gcc | 5058.486 [5058.480,5058.486] | 2799.590 [2767.137,2857.617] | 1.807 [1.770,1.828] | 2.030 [2.030,2.100] | 14.777 [14.762,14.797] |
| invalid | clang | 1000.160 [867.029,1366.951] | 399.492 [355.926,519.945] | 2.504 [2.436,2.629] | 0.120 [0.110,0.160] | 86.051 [86.047,86.086] |
| invalid | reference | 504.248 [504.240,504.248] | 235.363 [234.526,236.703] | 2.142 [2.130,2.150] | 0.080 [0.080,0.080] | 8.449 [8.414,8.539] |
| chain-1 | before | 584.244 [584.244,584.244] | 323.958 [285.065,352.804] | 1.803 [1.656,2.050] | 0.130 [0.130,0.130] | 52.973 [52.828,52.977] |
| chain-1 | after | 535.352 [535.352,535.352] | 215.501 [212.615,223.759] | 2.484 [2.393,2.518] | 0.060 [0.060,0.070] | 4.277 [4.270,4.277] |
| chain-1 | gcc | 313.170 [313.170,313.170] | 115.503 [113.940,115.748] | 2.711 [2.706,2.749] | 0.040 [0.040,0.050] | 30.613 [30.594,30.672] |
| chain-1 | clang | 335.523 [335.523,335.526] | 152.278 [145.763,153.707] | 2.203 [2.183,2.302] | 0.050 [0.050,0.050] | 76.027 [75.984,76.039] |
| chain-1 | reference | 820.023 [820.001,820.027] | 367.232 [361.067,367.252] | 2.233 [2.233,2.271] | 0.150 [0.150,0.160] | 50.145 [50.051,50.336] |
| chain-3 | before | 1746.158 [1746.158,1746.159] | 835.781 [825.914,862.745] | 2.089 [2.024,2.114] | 0.320 [0.320,0.320] | 117.590 [117.180,117.652] |
| chain-3 | after | 1601.575 [1601.575,1601.575] | 630.449 [619.635,635.359] | 2.540 [2.521,2.585] | 0.180 [0.170,0.180] | 5.645 [5.598,5.652] |
| chain-3 | gcc | 908.289 [908.289,908.290] | 316.577 [316.298,319.499] | 2.869 [2.843,2.872] | 0.130 [0.120,0.130] | 69.070 [69.062,69.109] |
| chain-3 | clang | 930.188 [930.186,930.196] | 356.076 [349.985,384.408] | 2.612 [2.420,2.658] | 0.110 [0.110,0.120] | 76.668 [76.668,76.699] |
| chain-3 | reference | 2227.950 [2227.947,2227.960] | 894.581 [894.426,896.727] | 2.490 [2.485,2.491] | 0.370 [0.360,0.370] | 132.027 [132.020,132.031] |
| full-triple | before | 7368.594 [7368.172,7368.595] | 3147.424 [3131.828,3156.358] | 2.341 [2.335,2.353] | 0.920 [0.870,0.970] | 18.715 [18.715,18.730] |
| full-triple | after | 5785.189 [5785.188,5788.829] | 2433.816 [2429.400,3701.167] | 2.378 [1.563,2.381] | 0.740 [0.680,1.100] | 18.648 [18.641,18.703] |
| full-triple | reference | 6084.624 [6084.603,6088.688] | 2493.908 [2487.425,2674.567] | 2.441 [2.275,2.446] | 0.780 [0.760,0.910] | 18.785 [18.738,18.836] |

Chain scale 300k→900k: final instructions scale 2.992x,
with two live facts and one operator at both sizes (source-buffer growth only).
Telemetry `pa3-audit/chain-3-final-telemetry.txt`: 1,799,999 logical nodes,
899,999 reductions, two live facts, one operator, no deferred domain errors.
Full triple: 492,075 lines, 6,889,051 PP tokens, 3,116,475 logical nodes,
1,640,250 reductions, max three facts/four operators; 170,712 deferred domain
errors are typed reduction facts, not independently emitted diagnostics.

### Attribution and branch/cache evidence

All triggers have separate `perf record -e cycles:u -F 99 --call-graph
dwarf,8192` profiles in `focused-audit-handoff/`; useful O3/g symbols and call
stacks, zero lost samples. Short profiles have limited statistical resolution:
percentages identify broad ownership, not sub-percent optimization claims.
Profiles show invalid-entry unwinding (initial independent sample 73% in
`__cxa_throw`/unwind path), removed by status returns; after-profile has no
expression throw path. Chain after profile is lexer/cursor/conversion dominated,
not tree evaluation or growth copies. Full triple after likewise belongs to
phase-1/2 lexer, literal conversion and output, with no reparse/global scans.
Nested profile attributes most work to Lexer/PostCursor/phase translation, plus
bounded parser push/pop. Lazy profile likewise lexing/conversion; folded domain
errors do not invoke trapping operations and no second evaluation walk remains.

Separate 3-repeat branch/cache groups for chain-3 are preserved verbatim:
`chain-3-{before,after}-{branch,cache}-{1,2,3}.perf`. CPU-specific cache events
must not be interpreted as universal cache levels. Median/ranges:

| Variant | branches | branch misses | cache references | cache misses |
|---|---:|---:|---:|---:|
| before | 335,617,390 [335,617,390,335,617,405] | 28,516 [28,401,29,140] | 5,677,293 [5,668,099,5,683,535] | 2,047,240 [2,039,490,2,050,025] |
| after | 303,688,679 [303,688,678,303,688,687] | 16,136 [16,097,16,238] | 98,570 [98,440,98,646] | 7,007 [2,683,11,617] |

### Inherited controls, same input hosts and output size

`pa2-perf/pa3-audit-accepted-*` reruns all five inherited workloads against the
independently audited PA2 baseline; `pa1-audit/pa3-audit-accepted-*` retains
three PA1 sources, hosts/reference and audited `pa2-final-audit` binaries.
PA1 first attempted the wrong baseline directory/label; that attempt failed
before measurements (`pa1-inherited-accepted.log`), then the verified existing
baseline was used (`pa1-inherited-retry.log`). Failed evidence is retained.

| PA2 workload | posttoken I M | baseline I M | gcc semantic I M / wall s | gcc lexical I M / wall s | posttoken wall s / RSS MiB |
|---|---:|---:|---:|---:|---:|
| declarations | 4294.696 | 4408.375 | 9599.078 / 2.900 | 1714.967 / 0.220 | 0.580 / 15.258 |
| raw | 1548.657 | 1559.896 | 1671.606 / 0.360 | 558.484 / 0.100 | 0.230 / 18.699 |
| hosted | 3994.284 | 4114.812 | 11849.706 / 4.270 | 1603.603 / 0.190 | 0.560 / 14.574 |
| literals | 2564.434 | 2617.913 | 3793.948 / 0.980 | 945.828 / 0.120 | 0.340 / 11.324 |
| concat | 1240.873 | 1239.364 | 656.026 / 0.120 | 593.344 / 0.070 | 0.160 / 11.285 |

PA2 posttoken / baseline instructions 0.971–1.001x; concat +0.12% is disclosed
and wall ranges overlap, not evidence of a material regression. GCC full
semantics and lexical hosts are separate columns because neither does exactly
PA2 diagnostic conversion/rendering work. Clang rejects the GCC-expanded hosted
TU semantically; `-E -P` accepts it and remains measured, not substituted for
semantic support. This is a host header-envelope gap, not PA3 hosted semantics.
Inherited disproportionate hosted/raw/concat workloads additionally sampled:
`pa3-audit/inherited-{hosted,raw,concat}.data` and corresponding profiles/logs.
Concat is ordinary-literal/take/encode work; hosted lexer/identifier/conversion;
raw raw-literal scanning/UTF8 encoding. These remain linear with bounded cursor
state; the owning punctuation dispatch was improved, not a timeout relaxed.

PA1 final diagnostic instructions relative to baseline drop ~8.6–10.1%; full
ranges, cycles/IPC/latency/RSS and same-input GCC/Clang are in its summary.
All inherited required fixture output and independent controls remain passing.

PA1–3 emit tokens/values, not ELF. Generated-executable runtime and emitted
text/object size are **not applicable** to this stage; no backend measurement
can be honestly attributed to PA3. Fixed loop/call/memory/FP/template/self-host
backend sources and quality thresholds are unchanged. Host-native qualification
through PA32 and 8/8 negative-control detections were rerun; PA3 native/PMU
gate entrypoints explicitly report N/A, not PASS. PA33 and PA34 still require
each fixed workload <=1.25x GCC user instructions at matching O2/O3, plus
behavior/MIR/debug and whole-self runtime where due.

No timeout is a performance budget. No prior numeric target was reclassified;
all measurements, required behavior, fixtures, references and comparison rules
are preserved. Final acceptance is stage-scoped architecture/correctness, not
future GCC runtime or instruction-budget acceptance.
