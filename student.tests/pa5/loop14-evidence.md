# PA5 final audit performance evidence — October 5, 2026

This record certifies stage-due syntax/frontend measurements, not native-code performance. Raw evidence is under `$RALPH_ARTIFACT_DIR/pa5/loop14/`. Previous trials, targets and observations are preserved in `pa5/audit.md` (historical audit 12), `student.tests/pa5/loop13-evidence.md`, and the `audit12/` and `loop13/` artifact directories. No numeric target is reclassified.

## Frozen protocol

- Final code: `6e277374e823970a07b052fd9201beab4db0f82d`; executable SHA256 `6aa64191e3f435af86377f7caef0d4454221220df10c5c1e1534c5d46e03df5f`. Entry/A SHA256 `279689986eb4f293bfc26cbe8a546fddc3d160f5efb29668f1b8ba1209043c72` (loop 13 code at turn-entry HEAD `112d17ee`).
- GCC build `-std=gnu++11 -Wall -O3` with registered frontend sources. Hosts: GCC 15.2.0, Clang 22.1.2; identical source bytes, C++11 `-fsyntax-only`. Student/entry/reference emit ASTs, byte equality checked before timing. Hosts additionally perform semantic checking: these are supported frontend costs, not capability parity.
- Xeon E5-2696 v4; final2 CPU3, same affinity for every variant; one unmeasured warmup per command/input; three interleaved repeats. `isolated2/` uses five interleaved repeats after other measurement jobs finish, same binary/source/CPU3. No cold-cache claim. Inherited preprocessing uses CPU4 with same-input `-E -P -x c++ -std=c++11` hosts. Earlier trial affinities remain in their manifests and are not paired with CPU3 deltas.
- Primary group `{instructions:u,cycles:u}`; IPC computed per run, then summarized. Branch/miss and cache-reference/miss groups use separate three-run passes, not a multiplexed mega-group. Raw enabled durations and running percentages retained in every `.perf`; final2 144 files/288 events, isolated2 40/80, slow-fixture2 12/24, inherited-perf 84/168: **all 100.00% running; no unavailable events**. Delivery checks in `delivery-manifest.json` also freeze host paths/hashes/environment.
- `final2/manifest.json`, `runs.json`, `summary.json`, `.perf`, `.time`, `.stderr` freeze inputs, binaries, flags, environment/CPU/versions, counts, wall/user/system time and RSS. All ranges (including host instruction/cycle/IPC and RSS ranges) remain in summary JSON; rounded tables below do not replace raw observations. `final2-measurement.exit`, `isolated2.exit`, `slow-fixture2.exit`, `inherited.exit`, `inherited-profile.exit`: 0.

## Same-input compiler measurements

Medians; instructions/cycles in billions, RSS in KiB. Wall ranges in brackets. Each table includes hosts and reference; the entry row is A, student is B.

| Workload | Variant | Instructions G | Cycles G | IPC | Wall s [min,max] | RSS KiB |
|---|---|---:|---:|---:|---:|---:|
| ordinary20000 | student | 7.800 | 3.596 | 2.169 | 1.65 [1.62,2.94] | 325708 |
| ordinary20000 | entry | 7.787 | 4.149 | 1.877 | 1.85 [1.61,2.19] | 322856 |
| ordinary20000 | gcc | 7.355 | 5.033 | 1.461 | 2.05 [2.02,2.07] | 276832 |
| ordinary20000 | clang | 7.196 | 9.451 | 0.759 | 3.50 [3.37,3.78] | 138940 |
| ordinary20000 | reference | 7.734 | 3.280 | 2.358 | 1.33 [1.31,1.95] | 164440 |
| scopes20000 | student | 12.662 | 6.224 | 2.035 | 2.67 [2.65,2.68] | 607092 |
| scopes20000 | entry | 12.640 | 6.209 | 2.036 | 2.75 [2.69,2.76] | 603688 |
| scopes20000 | gcc | 10.186 | 8.592 | 1.187 | 3.61 [3.53,3.70] | 786912 |
| scopes20000 | clang | 23.923 | 26.785 | 0.895 | 9.12 [9.07,9.22] | 261620 |
| scopes20000 | reference | 12.461 | 5.376 | 2.318 | 2.15 [2.07,2.17] | 289040 |
| classes20000 | student | 13.626 | 6.773 | 2.012 | 3.07 [2.71,3.69] | 610280 |
| classes20000 | entry | 13.602 | 7.020 | 1.938 | 3.20 [2.76,3.24] | 605744 |
| classes20000 | gcc | 55.125 | 86.613 | 0.636 | 33.08 [30.45,37.18] | 2117048 |
| classes20000 | clang | 17.143 | 29.352 | 0.584 | 10.89 [10.54,12.46] | 364416 |
| classes20000 | reference | 14.929 | 6.280 | 2.377 | 2.55 [2.35,2.57] | 278376 |
| parameters60000 | student | 2.691 | 1.234 | 2.180 | 0.61 [0.61,1.14] | 194564 |
| parameters60000 | entry | 2.686 | 1.247 | 2.153 | 0.62 [0.60,1.17] | 194544 |
| parameters60000 | gcc | 1.674 | 0.815 | 2.052 | 0.37 [0.36,0.37] | 122384 |
| parameters60000 | clang | 2.130 | 1.502 | 1.427 | 0.59 [0.56,0.59] | 134096 |
| parameters60000 | reference | 3.654 | 1.616 | 2.261 | 0.69 [0.69,0.87] | 114444 |
| templates20000 | student | 27.222 | 13.885 | 1.960 | 6.30 [5.97,10.62] | 1166712 |
| templates20000 | entry | 27.170 | 13.660 | 1.989 | 6.27 [5.66,7.17] | 1181112 |
| templates20000 | gcc | 38.614 | 42.928 | 0.900 | 16.75 [16.38,23.90] | 1981408 |
| templates20000 | clang | 29.803 | 53.620 | 0.556 | 19.84 [19.63,20.15] | 992008 |
| templates20000 | reference | 28.523 | 12.258 | 2.327 | 4.99 [4.91,5.06] | 561024 |

Student instruction ratios to GCC: ordinary 1.061×, scopes 1.243×, classes 0.247×, parameters 1.608×, templates 0.705×. Frontend RSS is at most about 1.59× GCC on these inputs; versus Clang/reference it can be substantially higher (e.g. templates ~1.18× Clang/~2.08× reference). The mandatory PA33/34 **executable** instruction gate is not a PA5 compiler-instruction target.

### Spread and isolated timing investigation

| Workload (final2 student) | Instructions G range | Cycles G range | IPC range |
|---|---:|---:|---:|
| ordinary20000 | 7.800038–7.800178 | 3.596–6.752 | 1.155–2.169 |
| scopes20000 | 12.662368–12.663268 | 6.175–6.227 | 2.034–2.051 |
| classes20000 | 13.626125–13.626985 | 6.694–8.148 | 1.672–2.035 |
| parameters60000 | 2.690680–2.690680 | 1.230–2.306 | 1.167–2.188 |
| templates20000 | 27.221584–27.221588 | 13.731–22.925 | 1.187–1.982 |

Ordinary final2 wall had a 2.94s outlier despite stable instructions; earlier `final/` ordinary cycles were also noisier. Nothing was discarded or labeled a speedup. Isolated observations resolve the comparison without replacing the initial trials:

| Workload | Variant | Instructions G | Cycles G | IPC | Wall s [min,max] | RSS KiB |
|---|---|---:|---:|---:|---:|---:|
| ordinary20000 | student | 7.800 | 3.538 | 2.205 | 1.58 [1.50,1.65] | 325832 |
| ordinary20000 | entry | 7.787 | 3.521 | 2.212 | 1.55 [1.48,1.59] | 322844 |
| ordinary20000 | reference | 7.734 | 3.204 | 2.414 | 1.23 [1.18,1.30] | 164440 |
| ordinary20000 | gcc | 7.355 | 4.959 | 1.483 | 1.93 [1.89,1.96] | 276828 |
| parameters60000 | student | 2.691 | 1.221 | 2.204 | 0.59 [0.56,0.59] | 194564 |
| parameters60000 | entry | 2.686 | 1.224 | 2.195 | 0.56 [0.55,0.60] | 194508 |
| parameters60000 | reference | 3.654 | 1.584 | 2.307 | 0.66 [0.63,0.68] | 114468 |
| parameters60000 | gcc | 1.672 | 0.779 | 2.146 | 0.33 [0.33,0.34] | 122400 |

Final correctness repairs add ~0.17–0.19% instructions across the fixed families (shared dependent productions and preserved scope frames), not a performance optimization. Final2 and isolated cycle/wall ranges overlap A/B, so **no latency speedup is claimed**. Compiler text increases 116 bytes: final text/data/bss **425172/4128/1568**, entry **425056/4128/1568**; reference **7188750/26864/6952**. These are compiler sections, not emitted program size.

## Scaling, phase work and budgets

`telemetry/runs.json` retains ten fixed runs. Ordinary 20k: 1440012 tokens, 1600017 nodes, 220002 scopes, parse/dump 1.44486/0.180308s. Scopes20k: 2700000 tokens, 2380001 nodes, 340001 scopes, 2.83821/0.245962s. Classes20k: 2460000 tokens, 2760001 nodes, 440001 scopes, 2.644/0.330666s. Templates20k: 5320299 tokens, 6160342 nodes, 760048 scopes, 2140092 queries, 5.60027/0.69364s. Parameters60k: 480022 tokens, 660029 nodes, 60006 scopes, 240009 queries, 0.548612/0.0563805s. Observed maximum lookahead 2–3; broader graph control bound ≤4. Phase timings are separate observations, not medians from the PMU trials.

2k→20k ordinary/template and 2k→60k parameter inputs preserve approximately proportional work and memory, not quadratic default-prefix or name-binding copies. Budgets are structural: fixed lexical lookahead, bounded parser common-prefix probes, one grammar parse per region, geometric arena/index growth, visited namespace/base/import edges, and compact buffers only for required complete-class regions. No global retry, abandoned alternative tree, all-TU lookup scan or token-history cache is authorized by a timeout. Long single declarators naturally consume their parameter count once; graph/control checks cover publication and overlays.

## Slowest fixtures and profile attribution

- `fixture-times.json` retains all 188 final fixture observations (~2–3.5ms observed), including the malformed `100-bad.t` startup outlier. Rankings at this scale are unstable, not a measured suite algorithm bottleneck. `exact-slowest/` freezes the top six unchanged inputs, three pinned interleaved same-byte runs each of student/reference/GCC/Clang (72 observations), including failures and all spreads. `/usr/bin/time` rounds student wall to 0.00s: this is **below timing resolution**, not zero compiler work. Most are syntax-only course cases rejected by semantic hosts; they are not used to claim portable parity.
- The partial-specialization fixture amplifier initially failed host qualification: `template<class...> enable_if_t` was passed a boolean. `slow-fixture-initial-failure.log` retains the failure. `slow-fixture2/amplified.cpp` makes the explicit companion change to `template<bool B>` and uses 2000 namespace-isolated copies. The original fixture/reference is unchanged. Both hosts qualify the companion and student/reference ASTs agree. Frozen manifest identifies both byte hashes; this is not mislabeled an unchanged exact-fixture benchmark.

| Portable companion | Variant | Instructions G | Cycles G | IPC | Wall s [min,max] | RSS KiB |
|---|---|---:|---:|---:|---:|---:|
| specialization ×2000 | student | 1.802 | 0.865 | 2.083 | 0.39 [0.38,0.39] | 79356 |
| specialization ×2000 | gcc | 1.292 | 1.000 | 1.291 | 0.44 [0.44,0.45] | 124832 |
| specialization ×2000 | clang | 1.362 | 1.913 | 0.712 | 0.74 [0.73,0.75] | 123956 |
| specialization ×2000 | reference | 1.922 | 0.816 | 2.355 | 0.33 [0.33,0.33] | 50348 |

Profiles use separate `perf record -e cycles:u -F 99 --call-graph dwarf,8192` runs with the comparison affinity, then `perf report --stdio --no-children`. Useful student symbols/call stacks and **zero lost samples** verified. `final2/templates20000-student-report.txt`: 690 samples; parser peek 11.46%+5.69% self, macro next 4.85%, lexer peek 4.40%, lexer next 4.08%, postcursor next 3.54%, dump 3.45%. GCC ~1K samples: GC marking/allocation dominates its larger semantic workload. `isolated2/parameters60000-student-report.txt`: 647 samples over ten unchanged invocations (GCC 361); parser peek 12.63%+9.23%, lexer peek 6.09%, lookup 4.81%, arena reallocation 4.31%, memmove 4.02%. `isolated2/ordinary20000-student-report.txt`: 612 samples over four unchanged invocations; peek 12.51%+6.68%, lexer peek 5.55%, macro next 4.69%, raw 4.61%. Sampling invocation counts are disclosed, not compared as single-compilation counts.

`slow-fixture2/student-report.txt` / `gcc-report.txt`: 40/45 samples, zero lost (coarse attribution only); templates/parameter accumulated profiles supply stronger evidence. Class/scope history: `classes/cycles.report` 565 samples, zero lost, parser peek 11.11%+6.05%, lexer peek 9.14%; `scopes/profile.txt` ~9K samples, zero lost. These earlier binaries/affinities are identified in their manifests, not falsely attributed to final2. No unexplained dominant retry or lookup scan was found.

Separate final2 branch/cache triples support memory/control-flow attribution: parameters student branch misses median 1.120M [1.119,1.206], GCC 0.886M [0.884,0.892]; cache misses student 4.423M [4.346,4.432], GCC 1.728M [1.461,1.988]. Templates student branch misses 40.982M [40.727,42.336], GCC 302.750M [301.783,323.021]; cache misses student 29.376M [28.764,29.450], GCC 99.955M [96.850,102.702]. These CPU-specific events are not universal L1 counts. Flat indexes, shared immutable template/default overlays and batched rendering resolve identified allocation/copy/render algorithms; the residual parameter cost is a small stable constant factor with arena/output work, not an unbounded architecture path.

## Inherited disproportionate workload retained

PA1–PA4 controls and budgets remain active. `inherited-perf/` freezes the standalone streaming cursor (same production preprocessing), GCC/Clang normalized same-byte output qualification, scale inputs, three pinned repetitions and raw counts; `inherited-perf/profiles/` contains separate branch/cache triples and symbolic cycle profiles. This cost is not hidden by the smaller PA5 syntax ratios.

| Workload | Variant | Instructions G | Cycles G | IPC | Wall s [min,max] | RSS KiB |
|---|---|---:|---:|---:|---:|---:|
| conditional340000 | student | 18.018 | 7.470 | 2.412 | 2.73 [2.70,2.84] | 51928 |
| conditional340000 | gcc | 5.328 | 1.839 | 2.898 | 0.71 [0.71,0.71] | 65096 |
| conditional340000 | clang | 6.893 | 2.704 | 2.549 | 1.02 [1.02,1.05] | 140168 |
| replacement16000 | student | 0.429 | 0.398 | 1.079 | 0.17 [0.16,0.17] | 12956 |
| replacement16000 | gcc | 0.248 | 0.184 | 1.346 | 0.10 [0.09,0.10] | 18636 |
| replacement16000 | clang | 0.161 | 0.130 | 1.240 | 0.08 [0.08,0.08] | 77248 |
| headers8000 | student | 0.437 | 0.450 | 0.971 | 0.36 [0.35,0.40] | 8604 |
| headers8000 | gcc | 1.906 | 2.211 | 0.862 | 1.04 [1.04,1.09] | 19348 |
| headers8000 | clang | 0.367 | 0.561 | 0.655 | 0.41 [0.41,0.42] | 84304 |

Conditional instructions are **3.382× GCC** (replacement 1.729×, headers 0.229×). Conditional21k/85k/340k instructions 1.114/4.509/18.018G and RSS 6480/15872/51928 KiB are proportional. Student profile 263 samples, GCC 68, zero lost: lexer peek 16.92%+2.33%, raw 11.68%, macro next 8.76%, lexer next 6.68%, take 5.10%, phase1 5.09%, postcursor 3.91%, expression advance 3.13%. The previously audited PA4 owner fixes removed redundant identifier reindexing, expression/scratch allocation, macro paint/compaction and repeated invocation ownership; `pa4/audit.md` and `pa4/performance.md` retain their A/B benefits and residual ~3.35×. This rerun confirms the same bounded streaming translation/expansion/evaluation cost, not a new inherited control regression or permission to weaken a budget. Further lexer/cursor constant-factor tuning is possible; no identified unfixed nonlinear or prohibited ownership algorithm remains.

## Native and later-stage boundary

PA5 provides no LowIR/MIR, object writer or generated executable. Generated-program runtime, executable instructions/cycles/IPC, and emitted text/object sizes therefore remain **unavailable**, not passed or substituted with compiler/AST size. Fixed native workloads (loops, calls, memory, floating point and self-hosting), legality/effect/alias facts, profitability, invalidation and work/growth budgets activate at their owning stages. Preserve every inherited control and the PA33/PA34 **per-workload ≤1.25× GCC O2/O3 executable-instruction** gate; PA34 whole-self runtime is separate. No reference/fixture correction or numeric-target reclassification was used.
