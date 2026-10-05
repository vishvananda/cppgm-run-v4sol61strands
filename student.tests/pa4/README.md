# PA4 explicit controls

Run from the root (these are not discovered by the course harness):

```sh
python3 student.tests/pa4/check.py
python3 student.tests/pa4/measure.py "$RALPH_ARTIFACT_DIR/pa4-perf" --label new-run
# Optional frozen A/B comparison on identical inputs:
python3 student.tests/pa4/measure.py "$RALPH_ARTIFACT_DIR/pa4-perf" --label paired --against new-run

g++ -std=c++11 -O2 -Idev/src student.tests/pa4/cursor.cpp \
  dev/src/preprocess/lex/*.cpp dev/src/preprocess/post/*.cpp \
  dev/src/preprocess/expr/*.cpp dev/src/preprocess/engine/*.cpp \
  -o "$RALPH_ARTIFACT_DIR/pa4-cursor-check"
"$RALPH_ARTIFACT_DIR/pa4-cursor-check" "$RALPH_ARTIFACT_DIR/pa4-location.cc"
```

`check.py` tests raw/expanded arguments, paste/placemarkers, recursion suppression,
conditionals, line directives, diagnostics, concatenation across directives,
includes/once identities, TU resets, random DAG expansion, 30k alias chains,
100k repeated aliases, 20k define/undef cycles and 100k nested arguments.
Portable cases are qualified with both GCC and Clang; course-specific paint
cases stay in the unchanged course suite. `PA4_TOOL` selects sanitizer or
alternate-host-built executables. `cursor.cpp` verifies physical source range,
presumed filename/line, typed values, and source-buffer lifetime after EOF.

`measure.py` builds frozen O3/g binaries for the actual tool and a non-rendering
structured cursor; runs GCC, Clang and reference on identical source bytes;
checks all produced tokens before measuring; saves raw instructions/cycles,
IPC, enabled/running percentages, wall/user/system/RSS and work telemetry.
One warmup and three pinned, interleaved repeats use the first allowed CPU.
`--against` includes frozen prior cursor/tool binaries. Nine workloads cover
text, dense lookup, repeated arguments, pasting, deep aliases, conditionals,
repeated includes, repeated aliases and nested arguments. No native executable
is generated at PA4: program runtime/text size and PA33/34's executable
instruction gate are N/A here, not waived for their owning stages.

Clang on this host crashes at 3k single nested identity-macro invocations;
GCC and the reference accept the same 10k input. The common nested benchmark
uses sixty 1k-depth invocations, qualified with all tools. The 100k student
control is additional scaling evidence, not a host-parity timing claim.
All failed initial runs and capability investigations remain in artifacts.
