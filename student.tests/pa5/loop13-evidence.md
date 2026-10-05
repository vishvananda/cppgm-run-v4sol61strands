# PA5 loop 13 implementation evidence (2026-10-05)

This is implementation validation, not independent stage acceptance. Course
fixtures/references/coverage/comparisons are unchanged: PA5 188/188 (was 115/188),
PA1–PA4 205/205, through PA5 393/393; file audit 50 files.

## Frozen performance protocol

Artifacts: `$RALPH_ARTIFACT_DIR/pa5/loop13/`. Student executable SHA256:
`279689986eb4f293bfc26cbe8a546fddc3d160f5efb29668f1b8ba1209043c72`.
Build: GCC `-std=gnu++11 -Wall -O3`, registered frontend source sets. CPU0 Xeon
E5-2696 v4; warmup outside three interleaved affinity-pinned samples per variant,
primary `{instructions:u,cycles:u}` together, 100% running. Host same-byte
C++11 `-fsyntax-only` qualification and student/reference exact AST comparison
precede measurements. Host semantic work exceeds PA5 syntax work: these tables
measure supported frontend costs, not equal semantic capability or native-code
quality. Manifests freeze flags, versions, CPU, binary/input hashes. Raw `.perf`,
`.time`, `.stderr`, runs/summary JSON preserve every observation.

Numbers below are medians; instruction/cycle units are billions. RSS is KiB.

| Workload | Tool | Instructions G | Cycles G | IPC | Wall s (range) | RSS KiB |
|---|---|---:|---:|---:|---:|---:|
| ordinary20000 | student | 7.787 | 3.523 | 2.210 | 1.50 (1.43–1.53) | 322716 |
| ordinary20000 | gcc | 7.355 | 5.156 | 1.427 | 1.85 (1.82–1.91) | 276828 |
| ordinary20000 | clang | 7.200 | 10.006 | 0.720 | 3.32 (3.13–3.42) | 138900 |
| ordinary20000 | reference | 7.734 | 3.218 | 2.403 | 1.14 (1.14–1.21) | 164764 |
| scopes20000 | student | 12.640 | 6.141 | 2.058 | 2.63 (2.44–2.75) | 603688 |
| scopes20000 | gcc | 10.191 | 8.896 | 1.145 | 3.58 (3.54–3.64) | 786964 |
| scopes20000 | clang | 23.937 | 27.072 | 0.884 | 9.60 (9.28–9.62) | 261560 |
| scopes20000 | reference | 12.463 | 5.388 | 2.313 | 2.16 (1.95–2.20) | 289012 |
| classes20000 | student | 13.603 | 6.575 | 2.069 | 2.89 (2.79–2.89) | 605724 |
| classes20000 | gcc | 55.185 | 91.260 | 0.605 | 31.81 (31.76–32.97) | 2117084 |
| classes20000 | clang | 17.228 | 30.181 | 0.569 | 10.36 (10.12–10.63) | 364404 |
| classes20000 | reference | 14.931 | 6.211 | 2.404 | 2.39 (2.16–2.40) | 277832 |
| parameters60000 | student | 2.686 | 1.268 | 2.119 | 0.61 (0.59–0.65) | 194516 |
| parameters60000 | gcc | 1.673 | 0.818 | 2.046 | 0.36 (0.36–0.37) | 122348 |
| parameters60000 | clang | 2.107 | 1.494 | 1.415 | 0.57 (0.56–0.60) | 134336 |
| parameters60000 | reference | 3.654 | 1.629 | 2.244 | 0.66 (0.65–0.70) | 114444 |
| templates20000 | student | 27.172 | 13.744 | 1.977 | 6.30 (5.77–6.52) | 1181608 |
| templates20000 | gcc | 38.444 | 43.612 | 0.885 | 16.18 (16.01–17.07) | 1981048 |
| templates20000 | clang | 29.890 | 55.117 | 0.542 | 19.59 (19.40–19.98) | 992348 |
| templates20000 | reference | 28.522 | 12.281 | 2.322 | 4.80 (4.69–5.04) | 561012 |

Primary raw directories respectively: `final-perf-ordinary`,
`final-perf-scopes`, `isolated-perf-classes`, `isolated-perf-parameters`,
`final-perf-templates`. Initial `final-perf-classes`/`final-perf-parameters` runs
overlap compilation/controls on other CPUs; both are preserved. Isolated
repeats confirm instruction work and give focused wall/RSS results. Small
ordinary/template runs have transient cycle spread; no small latency improvement
is claimed. Fixture startup-scale wall timing rounds to zero and is inconclusive.

## Work model and costs

- ordinary: large student/GCC instructions 1.059x; 2k→20k instruction scaling 9.958x.
- scopes: large student/GCC instructions 1.240x; 2k→20k instruction scaling 10.046x.
- classes: large student/GCC instructions 0.246x; 2k→20k instruction scaling 9.964x.
- parameters: large student/GCC instructions 1.606x; 2k→20k instruction scaling 9.785x.
- templates: large student/GCC instructions 0.707x; 2k→20k instruction scaling 9.967x.

Template 2k→20k graph work: 532,299→5,320,299 tokens,
616,342→6,160,342 nodes, 214,092→2,140,092 category queries; lookahead 3.
Template output is 168,580,170 bytes at 20k; source 16,610,980 bytes.
Arena/index retention explains substantial RSS relative to the reference: this
is disclosed, not a zero-cost claim. Compiler text/data/bss bytes are
425,056/4,128/1,568 (not generated-program size).

Separate branch/cache triples and symbolized cycle profiles remain in each
measurement directory. `final-perf-templates/profile.txt` and supplemental
`cycles.report` have useful frontend stacks (585 supplemental samples, zero
lost); `isolated-perf-parameters/cycles.report` identifies cursor/lexer/category
work and arena growth. Template peak lookahead/cursor costs dominate sampled
self time; lookup is about 3% self in the supplemental template profile.
Parameter instruction factor (~1.6x GCC) is bounded/linear, not a mandated PA5
limit miss; its same-source AST emits and retains nodes where GCC need not.
No growing scan factor or rendered-name hot path is indicated by measured work.
Historical evidence and targets in `pa5/audit.md` are preserved; no mandated
limit or previous observation has been reclassified.

PA5 emits no executable, native object or LowIR. Generated-program runtime/text/
object size are unavailable, not passed. The PA33 and PA34 per-workload ≤1.25x
GCC executable-instruction gate remains mandatory; compiler instruction ratios
above do not substitute for it. Plan canonical types, compact demand indexes
and typed lowering before PA24/26 to avoid reconstructing PA5 syntax or
committing representation costs to later codegen.

## Behavior and ownership validation

- Ordinary 13, scope 23, class 30 and template 32 portable families plus
  integrations; each new portable case GCC/Clang qualified. Four groups retain
  six rejected grammar cases apiece, plus driver/TU reset controls.
- Ordinary, scope, class and template retained graph tests pass in O2 and
  Clang ASan/UBSan builds. Ownership survives parser destruction; flat scope
  index passes 171,529 keys in both builds.
- `graph-sweep-final.json`: all 181 successful fixture roots and 32 portable
  template families have exactly one owning path per node in both builds.
- `fixture-san-final.json`: all 188 fixture invocations clean, with expected
  failure statuses preserved. Prefix sweep 211 + three deferred-region reducers
  is clean; no sanitizer acceptance claimed from unsanitized runs.
- Eleven inherited capability probes pass the student. Dependent conversion
  is independently checked because the reference rejects it; local N3485 proof
  is in `reference-notes.md`. Required references were not changed.
- Expanded GCC headers retain extension gaps (builtin transforms/traits,
  `__type_pack_element`, `unsigned __int128`, `_Float32`); same-source host
  disagreement is classified in `reference-notes.md`. Failed or nonportable
  headers are not timing evidence. `cstddef`/`initializer_list` exact views
  and amplified portable hosted/template mix are successful controls.

## Review boundary

No known required fixture failure remains. Independent audit must assess whole-
stage grammar/category coverage, deep mixed constructs and later semantic
consumption of retained factored syntax. Vendor builtin parsing and large-arena
peak-memory tuning remain separate implementation/capability work, not silently
waived. See the compact plan's separate handoff ledger.
