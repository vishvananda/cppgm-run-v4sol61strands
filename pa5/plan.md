# PA5 compact implementation plan

Stage base commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb
Last reviewed commit: 615f033c0a6c4fd106edc7b922173196397a48a3

## Current implementation boundary (loop 13, 2026-10-05)
Turn-entry HEAD: `3d2ef351ddff358b85a2b24ab4ea9efa8fe748fa`.
The existing stage/review markers above are preserved. Audit 12's whole-range
review and historical measurements remain in `pa5/audit.md`; this implementation
handoff does not update its independent-review marker or certify acceptance.
Unchanged PA5: **188/188**, up from 115/188 (all 73 inherited failures resolved).
PA1–PA4: **205/205**; through PA5: **393/393**; file audit: **50 files, pass**.
No fixture, reference, grammar, harness, coverage or comparison rule changed.

## Design / spec alignment and completed ownership group
Owner: `dev/src/syntax`, especially templates/names/scopes/classes/parser;
TU-owned node/edge/literal/scope arenas feed an explicit deterministic AST view.
Data flow: PA1–PA4 streaming tokens/source identity -> indexed syntax categories
and structured clauses/arguments/declarators -> retained graph -> dump view.
Known declarations override lexical hints; categories are not canonical types,
overload results, deduction or instantiated semantics (owned by later PAs).

Loop 13 implements type/non-type/template-template parameters, packs/defaults,
specializations/instantiations, operator/literal-operator template IDs, dependent
qualified/member/typename syntax, conversions and integrated class completion.
Parameter environments are retained but do not leak on publication; explicit
instantiation does not overwrite template categories. Delimiter-aware logical
angle splitting retains token identity and preserves expression shifts.
Mixed initializer/declarator prefixes are factored once, retaining typed graph
wrappers rather than abandoning nodes. Deferred region parsing always restores
scope/input/angle context on rejection; incomplete templates are not published.

Complexity: bounded lookahead (observed <=4), one parse per source region,
amortized arena/index growth and actual visited scope/base/import edges; no
rendered lookup keys, whole-TU token vector/scans, semantic tree copy or replay.
Class-completion buffers only the regions whose syntax needs completion.
Later semantics must consume the graph directly; PA24/26 representations should
retain compact canonical IDs/demand indexes/typed lowering, not syntax text.

## Validation / performance evidence
Full frozen hashes, host tables, ranges, runtime/size distinctions and artifact
paths: `student.tests/pa5/loop13-evidence.md` and `$RALPH_ARTIFACT_DIR/pa5/loop13/`.
Portable integrations and ordinary 13/scope 23/class 30/template 32 families pass
GCC/Clang qualification and exact reference-supported AST comparisons. Each
group retains six grammar rejections; driver/TU reset controls pass. Eleven
inherited capability probes pass; dependent conversion remains a documented
reference gap, independently qualified/graph-checked, not a reference revision.

ASan/UBSan: all 188 fixture invocations, integrations/families, 211 prefixes and
three explicit deferred-context reducers pass. O2 and sanitized retained-graph
controls pass; 181 successful fixture trees + 32 template families have single
node ownership. Flat index: 171,529 keys. Amplified template graph 2k->20k:
532,299->5,320,299 tokens; 616,342->6,160,342 nodes;
214,092->2,140,092 queries; lookahead 3.

Final code binary SHA256 `279689986eb4f293bfc26cbe8a546fddc3d160f5efb29668f1b8ba1209043c72`.
CPU0 Xeon E5-2696 v4; warmup then three interleaved grouped user-mode PMU runs,
100% running; separate branch/cache triples and symbolized profiles. Large
student/GCC compiler instruction ratios: ordinary 1.059x, scopes 1.240x,
classes 0.246x, templates 0.707x, 60k parameters 1.606x. All 2k->20k work is
approximately linear. Template medians: 27.172G instructions, 13.744G cycles,
IPC 1.977, wall 6.30s (5.77–6.52), RSS 1,181,608 KiB. Isolated class/parameter
repeats preserve initial overlapping observations and resolve focused timing;
small/startup-scale latency claims remain inconclusive where spread dominates.
Parameter factor and arena RSS are disclosed/profiled; no growing scan factor.
Historical numbers/targets remain recorded, not reclassified or waived.

PA5 emits no executable/native object/LowIR: generated runtime/text/object size
unavailable, not passed. Compiler text/data/bss: 425,056/4,128/1,568 bytes.
PA33/PA34's per-workload <=1.25x GCC executable-instruction gate remains
mandatory; diagnostic PA5 compiler ratios do not substitute for that gate.

## Handoff ledger: separate implementation and independent review
- Completed: cohesive template/dependent group plus cross-family correctness,
  ownership, rejection and cost controls; commits `d59b9532` and `ef9d937d`.
  The final evidence/control commit accompanies this ledger. This is the
  full-fixture implementation handoff to Ralph, not stage advancement.
- Unfinished implementation/capability work: vendor hosted extensions/builtin
  transforms/traits, `__type_pack_element`, `unsigned __int128`, `_Float32`;
  raw/normalized same-source host gaps are classified in `reference-notes.md`.
  These are not portable passing benchmarks or required C++11 fixture gaps.
  Broad arena peak-memory/cursor tuning remains possible, with all costs kept.
  Further extension work needs an explicit extension vocabulary/AST contract,
  rather than claiming compiler agreement supplies the course grammar.
- Independent audit questions (not waived): whole-stage grammar/category
  coverage beyond fixtures; deep mixed/dependent syntax and retained factored
  graph consumption by PA6/7; architecture/large-hosted cost review. Audit must
  distinguish supported C++11 from vendor builtin capability and assess the
  disclosed parameter constant-factor/RSS costs. No known required fixture
  failure or required-stage correctness defect remains open at this boundary.
- Later assignment work: canonical semantic facts, overload/demand/instantiation,
  lowering, ABI/ELF and executable quality. These remain later-stage owners;
  source-to-ELF validation is unavailable at PA5, not certified.
