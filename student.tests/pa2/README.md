# PA2 personal controls and performance evidence

Explicit validation (not fixture discovery):

```sh
python3 student.tests/pa1/check.py
python3 student.tests/pa2/check.py
python3 student.tests/pa2/measure.py "$RALPH_ARTIFACT_DIR/pa2-perf" --label accepted --against initial
```

`check.py` covers simple/invalid tokens, UD suffixes, numeric candidate tables,
character ranges, numeric-vs-UCN provenance (including delimiter/backslash UCNs),
raw text, maximal concatenation, recovery and literal-operator identifiers.
`cursor.cpp` tests typed payloads without source rendering and borrowed-byte/ID
ownership, including suffix physical locations across a splice. GCC and Clang
qualify portable C++11 assertions/execution; course-specific character rules
and invalid-token continuation use independent arithmetic/encoding oracles.
ASan+UBSan and Clang-built PA2 pass the same personal controls. Both host builds
are warning-free with `-std=c++11 -O2 -Wall -Wextra -pedantic`. A Clang-built
PA1 also passes all 54 fixtures. Results: 25 exact families, 1617 integer table
cases, valid/invalid 20,000-piece sequences, lexical rejection and typed controls.

## Frozen same-source measurement

Artifacts: `/home/vishvananda/work/private/v4sol61strands/artifacts/pa2-perf/`.
`accepted-manifest.json` freezes executable/source hashes, flags, host versions,
CPU and environment; `accepted-observations.json`, `.perf`, `.time`, `.telemetry`
preserve every observation. One unmeasured warmup, three interleaved repetitions,
CPU 0, warm-cache, simultaneous `{instructions:u,cycles:u}`, 100% running
counters. CPU: model name	: Intel(R) Xeon(R) CPU E5-2696 v4 @ 2.20GHz.
Compiler implementation built with GCC `-std=c++11 -O3 -g`; host same-source
commands are C++11/trigraph `-fsyntax-only` (more work than PA2) and `-E -P`
(less work than phase 5/6/7 conversion). No host pipeline is an exact PA2
bit-dump oracle. Reference posttoken provides matching stage work.
Inputs: 80k declaration namespaces, 40k adversarial raw strings, GCC-expanded
hosted TU with directives removed, 40k literal-rich namespaces, 1000 arrays
with 501 concatenated strings each. No filenames enter compiler behavior.

Each cell is median [min,max], except IPC (median). Instructions/cycles are
millions; latency seconds; RSS KiB. User/system times remain in raw JSON.

| Workload | Variant | Instructions M | Cycles M | IPC | Latency s | RSS KiB |
|---|---|---:|---:|---:|---:|---:|
| declarations | cursor | 3109.02 [3107.66,3112.31] | 1383.36 [1372.84,1394.12] | 2.25 | 0.41 [0.38,0.42] | 15628 [15568,15744] |
| declarations | posttoken | 4644.09 [4644.09,4659.97] | 2041.11 [2037.21,2057.35] | 2.28 | 0.63 [0.58,0.64] | 15720 [15708,15728] |
| declarations | gcc | 9598.20 [9598.10,9611.24] | 7856.29 [7641.95,7967.07] | 1.22 | 2.78 [2.75,2.85] | 743036 [743028,743724] |
| declarations | clang | 9307.92 [9289.23,9406.26] | 18805.71 [18581.65,18811.02] | 0.50 | 5.47 [5.45,5.58] | 186980 [186876,186988] |
| declarations | gcc-lex | 1714.97 [1714.97,1714.97] | 610.27 [606.50,638.89] | 2.81 | 0.19 [0.18,0.22] | 25664 [25664,25748] |
| declarations | clang-lex | 1320.05 [1320.04,1320.05] | 533.04 [531.81,542.07] | 2.48 | 0.17 [0.16,0.19] | 89468 [89440,89512] |
| declarations | reference | 4073.18 [4073.18,4073.18] | 1623.64 [1616.13,1638.97] | 2.51 | 0.49 [0.49,0.51] | 11452 [11452,11552] |
| raw | cursor | 1265.49 [1265.49,1265.49] | 558.54 [554.99,558.60] | 2.27 | 0.16 [0.16,0.17] | 19148 [19144,19236] |
| raw | posttoken | 1591.21 [1591.21,1591.21] | 681.36 [679.37,687.62] | 2.34 | 0.21 [0.19,0.22] | 19144 [19136,19268] |
| raw | gcc | 1671.66 [1671.66,1671.67] | 951.65 [950.85,953.34] | 1.76 | 0.32 [0.30,0.33] | 76336 [76268,77016] |
| raw | clang | 1731.10 [1730.09,1734.51] | 1792.34 [1783.79,1885.53] | 0.97 | 0.53 [0.52,0.59] | 111336 [111328,111476] |
| raw | gcc-lex | 558.48 [558.48,558.48] | 240.87 [240.41,242.31] | 2.32 | 0.08 [0.08,0.08] | 34828 [34796,35468] |
| raw | clang-lex | 373.42 [373.41,373.42] | 186.54 [185.64,188.69] | 2.00 | 0.07 [0.07,0.07] | 91204 [91200,91232] |
| raw | reference | 2046.72 [2046.72,2046.72] | 690.85 [690.80,706.79] | 2.96 | 0.20 [0.19,0.22] | 19184 [19180,19188] |
| hosted | cursor | 2986.96 [2986.96,2986.96] | 1300.89 [1300.02,1303.00] | 2.30 | 0.36 [0.36,0.36] | 14924 [14868,14924] |
| hosted | posttoken | 4382.55 [4382.55,4382.55] | 1889.34 [1888.22,1913.93] | 2.32 | 0.52 [0.52,0.53] | 15008 [14924,15020] |
| hosted | gcc | 11849.63 [11838.49,11854.95] | 11884.20 [11784.52,11893.23] | 1.00 | 3.76 [3.70,3.92] | 748868 [748816,748896] |
| hosted | clang | inconclusive | — | — | — | — |
| hosted | gcc-lex | 1603.60 [1603.60,1603.60] | 574.15 [572.72,574.65] | 2.79 | 0.17 [0.16,0.18] | 25648 [24944,25648] |
| hosted | clang-lex | 1454.51 [1454.51,1454.51] | 588.07 [575.64,588.55] | 2.47 | 0.18 [0.17,0.21] | 89408 [89384,89424] |
| hosted | reference | 3830.06 [3830.06,3830.06] | 1516.64 [1499.03,1516.81] | 2.53 | 0.42 [0.41,0.44] | 11636 [11632,11644] |
| literals | cursor | 1863.81 [1863.81,1863.81] | 809.09 [806.91,809.91] | 2.30 | 0.23 [0.22,0.24] | 11612 [11588,11620] |
| literals | posttoken | 2719.30 [2719.30,2719.30] | 1187.07 [1182.32,1187.29] | 2.29 | 0.33 [0.33,0.36] | 11560 [11488,11592] |
| literals | gcc | 3793.74 [3793.60,3794.86] | 2610.13 [2605.97,2610.44] | 1.45 | 0.89 [0.89,0.89] | 302276 [302208,302296] |
| literals | clang | 4282.43 [4266.91,4285.10] | 5484.13 [5394.14,5736.81] | 0.78 | 1.66 [1.61,1.66] | 183332 [183324,183352] |
| literals | gcc-lex | 945.83 [945.83,945.83] | 347.57 [346.63,351.24] | 2.72 | 0.10 [0.10,0.12] | 20732 [20716,20748] |
| literals | clang-lex | 742.05 [742.04,742.05] | 300.17 [298.22,309.76] | 2.47 | 0.09 [0.09,0.10] | 84896 [84868,84900] |
| literals | reference | 2423.46 [2423.46,2423.46] | 1007.04 [979.50,1181.24] | 2.41 | 0.28 [0.27,0.34] | 11636 [11584,11636] |
| concat | cursor | 1211.64 [1211.64,1211.65] | 495.52 [486.03,498.80] | 2.45 | 0.14 [0.13,0.14] | 11592 [11436,11596] |
| concat | posttoken | 1274.20 [1274.19,1274.21] | 507.42 [507.33,551.16] | 2.51 | 0.14 [0.14,0.15] | 11576 [11536,11588] |
| concat | gcc | 656.03 [656.03,656.05] | 283.02 [276.73,287.27] | 2.32 | 0.11 [0.10,0.11] | 67972 [67944,67976] |
| concat | clang | 552.41 [552.39,552.45] | 263.21 [260.39,272.18] | 2.10 | 0.10 [0.10,0.10] | 117496 [117484,117544] |
| concat | gcc-lex | 593.34 [593.34,593.34] | 205.47 [205.05,206.92] | 2.89 | 0.06 [0.06,0.07] | 18632 [18604,18656] |
| concat | clang-lex | 346.94 [346.94,346.94] | 145.79 [135.16,160.77] | 2.38 | 0.05 [0.05,0.05] | 82776 [82768,82784] |
| concat | reference | 1218.31 [1218.31,1218.33] | 397.95 [397.86,399.64] | 3.06 | 0.11 [0.11,0.11] | 11548 [11448,11632] |

## Equivalence, profile and cost interpretation

Four portable workload outputs exactly match reference hashes; all five typed
cursor checksums match initial/final and all repetitions. Hosted output has
24 fully enumerated extension differences in `accepted-parity.json`: 19
`__decltype`, three `__typeof__`, one `__typeof`, and one `0.0bf16`.
PA2 follows its C++11 token inventory; the reference supports extra GNU keywords
and BF16. No reference correction, normalization or fixture edit was made.
GCC accepts its own hosted TU; Clang syntax rejects GCC-specific bf16,
`__remove_reference`, `__integer_pack` and `_Float*` types. Diagnostic logs
`hosted-clang-gap.log`/`hosted-gcc-gap.log` record the capability gap. Clang
preprocessing still runs on exactly the same hosted source. Its semantic timing
is unavailable, not zero or a claimed speedup. All four portable workloads
qualify with both hosts. Hosted preprocessing directives are removed only
to honor PA2 input restrictions, not to hide token conversion defects.

Initial concat profile: `ordinary_literal` 29.5%, `matches` 15.7%; dispatching
literal prefixes by first character removes repeated inventory probes.
Accepted concat cursor instructions/cycles improve 22.1%/14.8%; renderer
21.3%/16.5%. Separating PA2 mandated stream scans into the debug renderer
leaves allocation-free typed scalar scanning (`strtof/strtod/strtold`):
declaration/literal cursor instructions improve 4.3%/7.1%, cycles 3.0%/5.4%.
Regressions disclosed: raw cursor instructions +0.50%, cycles +1.14%; hosted
cursor instructions +0.07%, cycles +0.77%; declaration renderer instructions
+0.87%, cycles +0.66% because it performs the explicit compatibility scan.
Small timing differences overlap ranges and are inconclusive. No claim that
all workloads got faster. Cursor is 0.32–1.85x GCC semantic instructions;
1.81–2.27x GCC preprocessing instructions, bounded costs at lower peak RSS.
Against Clang preprocessing, raw is 3.39x and concat 3.49x instructions:
Clang does no literal decoding/concatenation, so capability and output work
must not be conflated with full-frontend parity. Renderer adds explicit output
cost, measured independently of the future object pipeline.

Full/half inputs: declaration/concat instructions ratios 2.004/1.996;
work counters scale linearly. Raw max pending sequence is 150 elements, concat
501; no TU-wide token vector or per-element heap allocation. Work remains
proportional to lexer bytes/tokens and literal elements, not declarations².
Accepted cycles profiles: `accepted-{declarations,concat,hosted}.data` and
`*-profile.txt`, 519/168/436 useful symbol-attributed samples, zero lost.
Declaration/hosted owners are bounded lexer decoding and fixed metadata
classification; concat now primarily ordinary decoding/phase1/take, not
repeated prefixes or allocator churn. Separate concat branch/cache passes
record 219.7M branches, 31,960 misses; 240,275 cache references, 19,839 misses.
Event meanings are CPU-specific and these are diagnostic, not acceptance limits.

Inherited PA1 control rerun: `artifacts/pa1-audit/pa2-*` freezes the unchanged
inputs and audited baseline. Cursor instruction deltas declarations/raw/hosted
+0.71%/+2.94%/+0.64%, cycles +0.43%/+3.61%/+0.29%; renderer instructions
+0.57%/+2.61%/+0.51%. Raw overhead comes from literal metadata/provenance
bookkeeping; PA1 collection remains off. Costs stay bounded and all outputs
exactly match the inherited reference control. Hosted baseline cycle spread
is large (0.924–1.38B); small latency claims are inconclusive.

## Executable metrics and later gates

PA2 has no generated executable: generated runtime/text/object size are N/A,
not inferred from compiler instructions or renderer output bytes. PA24+ typed
lowering/MIR must avoid reparsing text, whole-TU scans and blanket spills.
The PA33/PA34 per-workload 1.25x GCC generated-executable instruction limit
is mandatory when due; early frontend measurements do not waive or satisfy it.
Independent architecture/capability audit remains a separate review.
