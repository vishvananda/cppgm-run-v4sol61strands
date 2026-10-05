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

## Final independent audit controls

```sh
python3 student.tests/pa3/audit_check.py
python3 student.tests/pa3/measure.py "$RALPH_ARTIFACT_DIR/pa3-perf" --label audit-handoff --against audit-before
PA3_AUDIT_LABEL=audit-handoff python3 student.tests/pa3/audit_measure.py
```

`audit_check.py` builds 6,432 expression trees with an independent Python typed
oracle (all operators, nested lazy/type interactions), excludes evaluated C++
undefined arithmetic from portable evidence, checks GCC/Clang `#if` and reference
agreement, and checks 39 explicit course/boundary/phase cases. Host warnings are
retained in artifacts, not suppressed as correctness evidence. The inherited
`check.py` separately qualifies native static assertions. Both support `PA3_TOOL`.

The final parser uses compact typed constant facts and deferred domain-error
bits, not a retained tree plus second evaluation walk. It still parses and checks
all operands. Grammar/literal errors cannot be hidden by lazy selection. Every
fact carries a physical source range, and expected rejection uses status returns.
Metrics `nodes` count logical terminals/reductions, `max_nodes` counts live facts,
`evaluated` counts valid arithmetic reductions (including harmless speculative
constant work), and `deferred_errors` counts rejected reduction facts, not emitted
errors. `max_stack` is operator-stack depth; constant finishing is `evaluate_us`.

`audit_measure.py` uses the frozen `audit-before` and the selected final `audit-handoff` binaries;
`PA3_AUDIT_LABEL` selects another previously frozen final label. Focused artifacts
are under `pa3-audit/focused-<label>/`, preserving earlier runs. Portable triple
expressions contain the same source text for hosts; full course triple remains a
student/reference comparison because mock `defined` and course character types
are not host semantics. Malformed host directives measure diagnostic recovery,
not equivalent output. Three interleaved warm pinned PMU/time/RSS repeats include
1x/3x chain scaling, slowest fixture and error recovery, separate `perf record`
profiles, and branch/cache passes. Raw counts/spread and limitations are recorded
in `pa3/audit.md` and the final appendix to `pa3/performance.md`.
