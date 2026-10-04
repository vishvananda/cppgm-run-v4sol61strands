# PA3 explicit personal controls

From the root:

```sh
python3 student.tests/pa3/check.py
python3 student.tests/pa3/measure.py "$RALPH_ARTIFACT_DIR/pa3-perf" --label experiment --against initial
```

`check.py` verifies lazy evaluation versus full syntax/literal validation,
course-promoted signed/unsigned values, precedence/conditional associativity,
logical-line/lexical error ownership, canonical mock-defined behavior and
100k-depth/chain scalability. It qualifies 1200 portable expressions both as
native C++ static assertions and preprocessing expressions with GCC and Clang.
`PA3_TOOL=/path/to/ppexpr` selects a sanitizer/other-host-built implementation.
Course-specific promotion cases are kept separate from host-native semantics.

`measure.py` generates independently checked fixed expression workloads,
freezes input/command/binary/host hashes and emits raw perf/time/telemetry plus
parity and median/range JSON. It compiles the actual implementation and the
non-rendering `bench.cpp` value-checksum consumer with C++11 O3/g. All host
#if envelopes contain exactly the same expressions; both GCC and Clang verify
expected values and emit equivalent output before warmup/timing. The checksum
consumer validates against those expected values, not a cached implementation.
One warmup and three pinned interleaved repeats measure instructions/cycles/IPC,
wall/user/system and peak RSS; there is no native emitted program at PA3.
Optional `--against` adds frozen prior binaries in the same measurement loop.
Use new labels to preserve all observations; never turn uncounted/unstable
hardware events or rejection gaps into passing performance evidence.

See `pa3/performance.md` for accepted run evidence and comparison limitations.
Personal controls do not replace the 20 course fixtures or independent audit.
