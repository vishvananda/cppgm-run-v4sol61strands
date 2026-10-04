# Explicit PA1 controls and measurement evidence

Run from checkout root:

```sh
python3 student.tests/pa1/check.py
for host in g++ clang++; do
  "$host" -std=c++11 -O2 -Idev/src student.tests/pa1/cursor.cpp \
    dev/src/preprocess/lex/lexer.cpp dev/src/preprocess/lex/unicode.cpp \
    dev/src/preprocess/lex/identifiers.cpp -o "$RALPH_ARTIFACT_DIR/cursor-$host"
  "$RALPH_ARTIFACT_DIR/cursor-$host"
done
# Also run the same cursor with -O1 -g -fsanitize=address,undefined
# -fno-omit-frame-pointer (verified with GCC).
python3 student.tests/pa1/measure.py "$RALPH_ARTIFACT_DIR/pa1-perf-final" \
  --prepare --label parity --runs 5
```

`check.py`: 39 exact byte-output cases and 24 rejections; portable phase
interaction source qualified with both GCC and Clang. `cursor.cpp`: locations,
physical ranges, stable IDs/spellings across 100,000 insertions and an oversized
name, Unicode scalar/identifier boundaries, and 1k/10k/100k repeated units.
The latter produce 9,001/90,001/900,001 tokens, 24k/240k/2.4M decoded characters,
3k/30k/300k raw candidates and one interned name. GCC/Clang cursor and GCC
ASan/UBSan controls pass. No course fixtures, outputs or comparisons changed.

## Frozen performance observations (October 4, 2026)

Artifacts: `$RALPH_ARTIFACT_DIR/pa1-perf{,-final}`. Each includes manifests with
input/binary hashes and commands, every raw `perf stat`/time/telemetry result,
JSON observations, summary tables and sampled profiles. References: bundle
source `0cfcea43c95bfb8a121f7b1e104a016560ebb3db`, unchanged.
Host: Xeon E5-2696 v4; GCC 15.2.0 / Clang 22.1.2; CPU 0; warm-cache, one
unmeasured warmup, five interleaved observations per variant, grouped
`{instructions:u,cycles:u}`, 100% event running. Student build C++11 O3 -g;
hosts `-std=c++11 -trigraphs -E -P -x c++ -`; outputs discarded. Exact student /
reference bytes agree on every measured source before timing. GCC/Clang also
accept the portable declaration/raw inputs and the original hosted TU under
`-fsyntax-only`. Hosted input freezes GCC-expanded vector/string/tuple/algorithm
headers with 80k ordinary functions; no stage-unsupported include/macro work is
charged to the student. Inputs: declarations 7,257,780 bytes; raw 8,468,890;
hosted 6,446,630. These are lexical comparisons, not semantic/codegen parity.

Final `parity` medians (instructions/cycles in billions, wall seconds, RSS KiB):

| Input | Variant | Instructions | Cycles | IPC | Wall | RSS |
|---|---|---:|---:|---:|---:|---:|
| declarations | typed cursor | 2.459 | 0.950 | 2.588 | 0.27 | 15,776 |
|  | pptoken rendering | 4.940 | 1.919 | 2.572 | 0.53 | 15,664 |
|  | GCC | 1.715 | 0.608 | 2.822 | 0.17 | 25,660 |
|  | Clang | 1.320 | 0.540 | 2.445 | 0.16 | 89,484 |
|  | reference rendering | 4.537 | 1.725 | 2.630 | 0.48 | 11,576 |
| raw | typed cursor | 0.932 | 0.335 | 2.784 | 0.11 | 19,284 |
|  | pptoken rendering | 1.290 | 0.475 | 2.714 | 0.16 | 19,184 |
|  | GCC | 0.558 | 0.242 | 2.306 | 0.09 | 34,796 |
|  | Clang | 0.373 | 0.186 | 2.012 | 0.07 | 91,208 |
|  | reference rendering | 1.498 | 0.517 | 2.897 | 0.17 | 19,160 |
| hosted | typed cursor | 2.344 | 0.902 | 2.598 | 0.25 | 14,948 |
|  | pptoken rendering | 4.886 | 1.899 | 2.573 | 0.53 | 14,964 |
|  | GCC | 1.606 | 0.575 | 2.792 | 0.16 | 25,012 |
|  | Clang | 1.456 | 0.594 | 2.452 | 0.18 | 89,400 |
|  | reference rendering | 4.508 | 1.722 | 2.618 | 0.47 | 11,556 |

Cursor instruction ratios to GCC: 1.43x / 1.67x / 1.46x; latency 1.59x /
1.22x / 1.56x, RSS .61x / .55x / .60x. Tool rendering instruction ratios:
2.88x / 2.31x / 3.04x. The excess is bounded per-token formatting, absent from
the production cursor; GCC/Clang output source instead of token kind/length
records. Rendering/reference ratios are about 1.09x/.86x/1.08x. No claim of
semantic equivalence or full compiler parity is made.

Final instruction spread <.3%; cycle ranges for cursor: .938-.961B,
.333-.336B, .890-.905B. Wall ranges .26-.28/.10-.12/.25-.25 seconds, with coarse
10ms time resolution; small latency differences remain inconclusive. All
spreads and host observations remain in `parity-summary.txt` and raw results.
Baseline small hosted timing and some reference runs had interference; retained
without claiming improvement. Final hosted workload enlarged to dominate startup.

Profiles identified raw per-character string append and cursor bookkeeping.
Batch raw copying reduced instructions 25.16% and cycles about 22% versus the
frozen baseline; no instruction regression on other same-input workloads.
A ring-buffer experiment increased declaration/hosted instructions ~6/5% and
was discarded. Flat slab interning then reduced instructions another ~2% and
cycles ~8-9% on all three compared with `final` node-map build. The last correctness fix preserves escape parity during speculative splice
lookahead; its frozen `parity` build increases instructions ~4% on declarations
and hosted, ~1.5% on raw versus `flat` (no budget reclassification). Final hosted
profile: 205 samples, no lost samples, useful symbol/call-stack data; leading
self costs peek 25.1%, next 17.8%, take 12.0%, phase1 10.1%, matches 6.1%, intern
4.6%. These are local bounded operations, not global scans. Baseline/batch raw
branch/cache auxiliary passes are retained; branch count fell 249M ->188M,
misses .707M ->.384M, cache misses .129M ->.128M. Cache events are CPU-specific.
Final names/probes/slabs: declarations 80,007/1,144,270/8; raw
40,002/252,639/4; hosted 82,871/1,166,890/14. Probe work includes repeated tokens
and geometric rehash, not just unique-name insertions.

Generated-program runtime and text/object size: **not applicable at PA1**.
Benchmark compiler binary text is separately 32,551 bytes (cursor) and 41,316
bytes (pptoken); not a generated-code-size measurement. PA33/34's mandatory
per-workload 1.25x GCC instruction gate remains due and unmodified; these early
lexical diagnostic ratios do not certify or waive it.
