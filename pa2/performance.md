# PA2 final measurement ledger — October 4, 2026

Implementation/control commit: `83ca4ff2`. See [whole-stage audit](audit.md) for
ownership, interpretation, legality and validation; this ledger reports rather
than replaces raw observations. No measurements deleted or numeric target waived.

Artifact root: `/home/vishvananda/work/private/v4sol61strands/artifacts/`.
Signoff: `pa2-perf/audit-signoff-{manifest,observations,summary,parity}.json`;
all raw `audit-signoff-*.{perf,time,telemetry}` and frozen binaries remain there.
Other labels `initial`, `dispatch`, `final`, `accepted`, `audit`, `audit-filter`,
`audit-final`, `audit-confirm` retain historical A/B measurements and spread.
`pa2-audit/*-signoff.data`, `*-signoff-profile.txt`, profile manifests and raw
record logs retain symbol/call-stack sampling. `pa1-audit/pa2-audit-final-*`
retains inherited PA1 A/B against the frozen independently audited baseline.

CPU 0, Intel Xeon E5-2696 v4 @ 2.20GHz; warmed once, 3 interleaved runs each;
primary simultaneous `{instructions:u,cycles:u}`, task-scoped inheritance.
Compiler binaries built `g++ -std=c++11 -O3 -g`; measured GCC/Clang commands
`-std=c++11 -trigraphs -fsyntax-only -x c++ -` and
`-std=c++11 -trigraphs -E -P -x c++ -`. Cursor does typed PA2 work, posttoken adds
explicit views; semantic hosts do more work, preprocessing hosts less. Host
flags match the supported source mode; no O2/O3 native comparison is invented.
Counters exclude kernel/waiting, while raw user/system/wall/RSS expose those
costs. `pa2-audit/counter-availability.json` checks 1,720 events: all ran 100%,
none missing or unsupported; raw time-running fields are retained.
Clang's semantic rejection of GCC-expanded hosted extensions is preserved and
not represented as a successful equivalent-source measurement. Other hosted
comparisons include GCC semantics and both hosts' preprocessing on that input.

Each numeric cell is **median [min,max]**. Instructions/cycles are millions;
wall seconds; RSS KiB; IPC is separately computed per observation. All raw
user/system times remain in observation JSON. Output equality/checksum checks
precede timing. Debug output bytes are not object or generated text size.

## Fixed compiler workloads

| Workload | Variant | Instructions M | Cycles M | IPC | Wall s | RSS KiB |
|---|---|---:|---:|---:|---:|---:|
| declarations | cursor | 2872.79 [2871.14,2873.30] | 1292.11 [1266.25,1315.11] | 2.224 [2.184,2.267] | 0.37 [0.36,0.41] | 15660 [15628,15772] |
| declarations | posttoken | 4408.37 [4408.37,4408.77] | 1906.26 [1903.63,2190.95] | 2.313 [2.012,2.316] | 0.57 [0.54,0.67] | 15624 [15616,15692] |
| declarations | gcc | 9597.90 [9597.69,9598.81] | 7919.20 [7751.43,7991.72] | 1.212 [1.201,1.238] | 2.71 [2.70,2.76] | 743140 [743088,743748] |
| declarations | clang | 9320.72 [9294.35,9322.82] | 19137.48 [18708.47,19180.34] | 0.486 [0.486,0.498] | 5.50 [5.44,5.58] | 186928 [186880,186976] |
| declarations | gcc-lex | 1714.97 [1714.97,1714.97] | 609.93 [608.84,617.61] | 2.812 [2.777,2.817] | 0.20 [0.19,0.21] | 25676 [25588,25748] |
| declarations | clang-lex | 1320.05 [1320.04,1320.06] | 540.59 [540.13,550.09] | 2.442 [2.400,2.444] | 0.18 [0.17,0.18] | 89440 [89408,89452] |
| declarations | reference | 4075.83 [4073.18,4079.03] | 1635.35 [1612.12,1636.84] | 2.492 [2.491,2.528] | 0.48 [0.45,0.52] | 11548 [11456,11636] |
| raw | cursor | 1233.72 [1233.72,1233.72] | 554.06 [545.51,623.65] | 2.227 [1.978,2.262] | 0.17 [0.16,0.20] | 19148 [19144,19252] |
| raw | posttoken | 1559.90 [1559.90,1559.90] | 665.56 [659.83,672.43] | 2.344 [2.320,2.364] | 0.20 [0.20,0.21] | 19240 [19092,19252] |
| raw | gcc | 1671.61 [1671.61,1671.64] | 967.94 [967.07,969.34] | 1.727 [1.724,1.729] | 0.32 [0.31,0.32] | 76340 [76252,76972] |
| raw | clang | 1732.38 [1728.38,1736.31] | 1974.93 [1916.72,2015.04] | 0.877 [0.862,0.902] | 0.61 [0.56,0.61] | 111300 [111292,111300] |
| raw | gcc-lex | 558.48 [558.48,558.49] | 241.86 [240.90,248.76] | 2.309 [2.245,2.318] | 0.09 [0.09,0.09] | 34908 [34796,34916] |
| raw | clang-lex | 373.42 [373.42,373.43] | 185.35 [172.23,186.35] | 2.015 [2.004,2.168] | 0.07 [0.07,0.08] | 91204 [91136,91224] |
| raw | reference | 2046.72 [2046.72,2046.72] | 699.84 [696.19,700.51] | 2.925 [2.922,2.940] | 0.22 [0.22,0.23] | 19188 [19104,19188] |
| hosted | cursor | 2719.22 [2719.22,2719.22] | 1198.82 [1182.82,1220.05] | 2.268 [2.229,2.299] | 0.33 [0.33,0.38] | 14904 [14868,14924] |
| hosted | posttoken | 4114.81 [4114.81,4114.81] | 1816.41 [1788.70,1816.87] | 2.265 [2.265,2.300] | 0.53 [0.53,0.58] | 15004 [14920,15012] |
| hosted | gcc | 11840.96 [11838.55,11855.68] | 12078.71 [12075.36,12088.16] | 0.981 [0.980,0.981] | 4.02 [3.86,4.10] | 748888 [748832,748944] |
| hosted | clang | inconclusive: GCC-only extensions rejected | — | — | — | — |
| hosted | gcc-lex | 1603.60 [1603.60,1603.60] | 585.92 [585.25,589.01] | 2.737 [2.723,2.740] | 0.17 [0.17,0.18] | 25012 [24924,25060] |
| hosted | clang-lex | 1454.51 [1454.51,1454.53] | 602.56 [599.60,610.51] | 2.414 [2.382,2.426] | 0.19 [0.19,0.19] | 89376 [89340,89404] |
| hosted | reference | 3830.06 [3830.06,3830.06] | 1536.68 [1525.90,1602.47] | 2.492 [2.390,2.510] | 0.45 [0.44,0.49] | 11628 [11448,11640] |
| literals | cursor | 1762.42 [1762.42,1762.42] | 794.03 [790.03,801.20] | 2.220 [2.200,2.231] | 0.23 [0.22,0.24] | 11596 [11584,11612] |
| literals | posttoken | 2617.91 [2617.91,2617.91] | 1172.86 [1161.53,1230.10] | 2.232 [2.128,2.254] | 0.34 [0.34,0.35] | 11592 [11592,11592] |
| literals | gcc | 3793.88 [3793.59,3794.06] | 2663.04 [2584.27,2709.62] | 1.425 [1.400,1.468] | 0.93 [0.92,0.94] | 302256 [302196,302264] |
| literals | clang | 4278.03 [4268.02,4290.03] | 5701.55 [5328.66,5917.99] | 0.752 [0.723,0.801] | 1.74 [1.56,1.75] | 183332 [183228,183412] |
| literals | gcc-lex | 945.83 [945.83,945.83] | 348.31 [346.91,348.58] | 2.716 [2.713,2.726] | 0.10 [0.10,0.10] | 20784 [20768,20832] |
| literals | clang-lex | 742.05 [742.04,742.05] | 324.09 [322.70,334.56] | 2.290 [2.218,2.299] | 0.10 [0.10,0.11] | 84864 [84848,84876] |
| literals | reference | 2423.46 [2423.46,2423.46] | 1009.34 [996.50,1053.10] | 2.401 [2.301,2.432] | 0.28 [0.28,0.29] | 11604 [11544,11620] |
| concat | cursor | 1176.81 [1176.81,1176.81] | 498.42 [493.67,500.70] | 2.361 [2.350,2.384] | 0.14 [0.14,0.15] | 11568 [11436,11596] |
| concat | posttoken | 1239.36 [1239.36,1239.36] | 527.24 [522.05,535.20] | 2.351 [2.316,2.374] | 0.15 [0.14,0.17] | 11592 [11588,11596] |
| concat | gcc | 656.03 [656.03,656.03] | 282.34 [280.61,287.70] | 2.324 [2.280,2.338] | 0.11 [0.11,0.11] | 67860 [67860,67924] |
| concat | clang | 552.44 [552.40,552.47] | 269.38 [266.42,276.08] | 2.051 [2.001,2.074] | 0.10 [0.10,0.11] | 117488 [117368,117492] |
| concat | gcc-lex | 593.34 [593.34,593.34] | 210.61 [205.74,213.48] | 2.817 [2.779,2.884] | 0.06 [0.06,0.07] | 18648 [18632,18724] |
| concat | clang-lex | 346.94 [346.94,346.94] | 141.74 [137.90,143.94] | 2.448 [2.410,2.516] | 0.05 [0.05,0.05] | 82760 [82688,82768] |
| concat | reference | 1218.31 [1218.31,1218.31] | 400.70 [397.63,410.55] | 3.040 [2.967,3.064] | 0.12 [0.11,0.12] | 11632 [11448,11632] |

## Paired A/B signoff deltas and absolute host control

Baseline is frozen pre-audit implementation `23013af0` (binaries labeled `audit`),
not the course reference. Each workload also includes the host in the preceding
table; host ratios below prevent an A/B-only acceptability claim. Cycle percentages
are not interpreted as improvement when spread overlaps. Negative means less work.

| Workload | Cursor instructions Δ | Cursor cycles Δ | Renderer instructions Δ | Renderer cycles Δ | Cursor/GCC semantic instructions | Cursor/GCC preprocessing instructions |
|---|---:|---:|---:|---:|---:|---:|
| declarations | -7.54% | -5.95% | -5.09% | -7.09% | 0.30x | 1.68x |
| raw | -2.51% | -1.68% | -1.97% | -2.66% | 0.74x | 2.21x |
| hosted | -8.96% | -11.42% | -6.11% | -7.56% | 0.23x | 1.70x |
| literals | -5.44% | -2.35% | -3.73% | -3.11% | 0.46x | 1.86x |
| concat | -2.87% | +0.67% | -2.73% | +0.46% | 1.79x | 1.98x |

Concat cycles vary from slight reductions in confirm to slight increases at
signoff; its ±small cycle difference is inconclusive. No renderer or cursor
instruction regression occurred on these workloads. Prior raw/literal dispatch
experiments and their regressions remain in the earlier ledger README/artifacts.
`audit-final` declaration cursor cycle median 1.735B [1.274B,1.773B] under
concurrent diagnostics was retained; serial confirm 1.269B [1.269B,1.285B],
signoff 1.292B. Literal confirm renderer cycle outlier 1.417B and wall regressions
remain recorded. No broad latency claim is based on such overlapping variation.

Full/half declaration/concat cursor instructions are 2.003x/1.996x;
`pa2-audit/scale-{manifest,observations}.json` and raw scale perf/time/telemetry
contain the paired same-input GCC half controls. Concat full work: 4,032,890 input
bytes, 1,013,001 PP tokens, 8,001 post tokens, 501,000 literal elements, 1,000
sequences, 1,006,000 converted bytes, max pending 501 elements. Scratch stays
bounded by current maximal sequence and retains capacity, not the full TU stream.

## Slowest required fixture

`pa2/tests/700-hard-string-concat.t`, 429,984 bytes. Raw three-run observations
for **every** PA1/PA2 fixture: `pa2-audit/fixture-observations.json`. The other
fixtures are startup-scale; do not treat their harness timeout as a work budget.
Pinned slowest evidence: `pa2-audit/slowest-signoff-{manifest,observations,summary}.json`,
raw `.perf/.time/.telemetry`; initial and filter/final variants preserved.
GCC/Clang -E do not perform PA2 phase-5/6 conversion; this fixture deliberately
contains invalid suffixes, and Clang rejects some at lexing (exit 1), so its row
is an explicit diagnostic, not equivalent successful work or a correctness oracle.
The larger concat workload supplies reliable timing beyond fixture startup and
/usr/bin/time's 10ms display granularity.

| Variant | Instructions M | Cycles M | IPC | Wall s | RSS KiB |
|---|---:|---:|---:|---:|---:|
| cursor | 130.43 [130.43,130.43] | 56.25 [55.34,62.17] | 2.319 [2.098,2.357] | 0.01 [0.01,0.01] | 4312 [4308,4368] |
| posttoken | 167.77 [167.77,167.77] | 71.25 [70.40,71.42] | 2.355 [2.349,2.383] | 0.02 [0.02,0.02] | 4360 [4356,4368] |
| reference | 146.10 [146.10,146.10] | 55.08 [55.06,56.01] | 2.652 [2.608,2.654] | 0.01 [0.01,0.02] | 4396 [4320,4400] |
| gcc-E | 91.99 [91.98,91.99] | 44.90 [44.79,45.38] | 2.048 [2.027,2.053] | 0.01 [0.01,0.01] | 12324 [12252,12324] |
| clang-E (rejection diagnostic) | 124.37 [124.36,124.38] | 83.75 [71.31,85.81] | 1.485 [1.449,1.744] | 0.03 [0.03,0.03] | 77192 [77180,77192] |

Slowest initial renderer 173.69M [173.69M,173.86M] instructions, 73.58M
[73.52M,73.85M] cycles; final lower work remains bounded vs reference. Thirty
separate sampling executions (`slowest-signoff.data`, `slowest-signoff-profile.txt`)
give 67 attributed samples, zero lost; lexical decoding, suffix handling and
explicit render/iostream routines dominate, not an accumulating IR or token vector.

## Attribution, counters and sizes

All signoff profiles use `perf record -e cycles:u -F 99 --call-graph dwarf,8192`
at CPU 0, separate from comparison runs; twelve independent executions give
451/421/183/218/284 samples for declaration/hosted/concat/raw/literals, zero lost.
Signoff concat ordinary decoding/take dominate (30.63%/18.02%), with required
phase1/provenance/encoding work; no repeated literal grammar or per-element
heap nodes. The shared impossible grammar probes were fixed at their owner.
Extra concat passes (`pa2-audit/signoff-concat-{branch,cache}.perf`):
213,086,882 branches, 31,372 branch misses; 244,711 cache references, 3,405 misses,
100% running. Cache events are CPU-specific, not portable acceptance limits.

Compiler text sizes are **not generated text**: cursor 74,305 -> 74,113 bytes;
posttoken 73,499 -> 73,307 bytes; files with debug data 1,316,344 -> 1,316,136 and
1,315,432 -> 1,315,216 bytes. `pa2-audit/compiler-size.json` preserves observations,
while manifests freeze binary hashes. Generated native runtime/text/object sizes
and fixed loop/call/memory/FP/self-hosting executable comparisons are N/A at PA2;
this compiler only produces token views. Do not substitute renderer output size.
No final-backend claim or omission waiver follows from frontend counters.

PA33 and PA34 retain the mandatory per-workload <=1.25x GCC instruction gate at
matching O2/O3 with complete pipelines; `student.tests/backend-quality/check_instructions.py`
remains unchanged. Missing or unstable later measurements are non-passing, not
zero. PA34 whole-self runtime is a separate required observation.
