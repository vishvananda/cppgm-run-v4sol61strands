# PA5 implementation ledger

Stage base commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb
Last reviewed commit: 08275a628da86ffd921633806a3f9ca72fcaaaeb

## Design / groups
- Initial state: PA4 streaming preprocessing/post-token conversion exists; PA5 driver is a scaffold (0/188 passing).
- Owner `syntax`: stream post-tokens into a compact TU arena of typed syntax nodes; interned identifier/category bindings, indexed lexical scopes; AST output is a view, never phase transport. Preserve source identities and literal values for later semantics. No whole-TU lookup or grammar replay.
- First coherent group: driver, ordinary declaration/declarator/initializer and expression/statement grammar, including precedence, conditions, parameter shadowing and bounded ambiguity. Data flow: immutable PP sources -> PostCursor -> bounded parser cursor -> node/edge arena -> iterative dump. Target O(tokens + nodes + required scope depth), O(TU graph + bounded lookahead) memory. Validation: exact PA5 fixtures plus personal structural/rejection tests, GCC/Clang qualification and repeated same-input measurements.
- Extend within shared understanding to structured type-id, casts and declaration-derived syntax. Class/template category and dependent-name support are separate later groups if they require a distinct deferred-body environment.
- Future semantics extend this graph in place with compact fact IDs; no syntax-to-semantic tree copy. PA24/26 per-function typed MIR and allocation must anticipate the mandatory PA33/34 per-workload 1.25x GCC instruction gate; no early-stage generated-code claim.

## Performance / controls
Pending: frozen stage-supported workload, host/compiler/reference comparison, >=3 pinned runs with instructions:u/cycles:u/IPC, latency/RSS separate from executable runtime/size. Hardware errors are missing evidence, not zero. PA5 has no executable emission; generated runtime/size are not applicable.

## Handoff ledger
Implementation unfinished: all PA5 parsing, then namespaces/classes/enums/templates and dependent ambiguity coverage.
Independent review questions: inspect bounded rollback, category lifetime, source/literal ownership and allocation/lookup complexity on large supported TUs. No question waived.
Boundary: not yet established. Required validation: make test-pa5, make test-report-through-pa4, file audit, personal tests.
