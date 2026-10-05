# PA5 independent reference observations (no course output changed)

Observed bundle source revision: `0cfcea43c95bfb8a121f7b1e104a016560ebb3db`;
archive SHA-256 `1b7b2863858e7c265417084e758838efbae6cfb788a5d0dec349d255c329a26c`.
The reduced cases below are accepted by GCC and Clang in C++11 mode. Agreement is
additional qualification, not the proof. Required fixtures and comparison rules
are unchanged; these are new personal cases outside the course suite.

## Implicit branch/loop scopes

```cpp
typedef int C;
int f(int x) {
  if (x) int C = 1; else C b = 2;
  C a = 3;
  do int C = 4; while (x);
  C b = 5;
  return a + b;
}
```

Reference fails parsing the else declaration because the then declaration leaked
its value category. N3485 §6.4 [stmt.select]/1 explicitly gives **each**
selection substatement its own implicit block scope, including the else branch;
§6.5 [stmt.iter]/2 does the same for iteration substatements. Repository
`doc/n3485.txt:7382-7396` and `:7484-7496` contain these rules. Thus the inner
value `C` cannot replace the outer typedef in the sibling/after-body regions.
The personal control checks preservation of all three type-led declarations;
no erroneous reference result replaces this independent observation.

## Allocation initializer versus function type

```cpp
void f() { new int*(); }
```

Reference produces a `parameter-clause` under the abstract declarator, omitting
the empty new-initializer. N3485 §5.3.4 [expr.new]/1 grammar defines an
unparenthesized allocation as `new-type-id new-initializer`; the
`new-declarator` permits ptr operators and array declarators, **not** function
parameter clauses. Therefore `int*` is the new-type-id and `()` is the
new-initializer. See `doc/n3485.txt:6384-6399`. The student preserves an abstract
pointer declarator followed by `initializer/paren-initializer`. PA5's shared
grammar accepts these bytes; no checked-in fixture specifies a contradictory
output. This personal case checks that tree structure directly rather than
revising required reference outputs.

## Class/base and out-of-class member scopes (loop 11)

```cpp
struct base { typedef int word; };
int word;
struct lower : base { word f(word x) { return (word)x; } };
```

The reference rejects the parameter declaration. N3485 §3.4.1/8 requires member
lookup to search the class and its bases before enclosing namespaces;
§10.2/4-5 first checks the current class, then its direct bases when no declaration
is found. Thus `base::word` is the type at the parameter and cast, not the global
value `word`. See `doc/n3485.txt:3016-3057`, `:12771-12802`.

```cpp
struct lower { typedef int word; word f(word); };
int word;
lower::word lower::f(word x) { word y=x; return y; }
```

The reference again rejects the parameter. N3485 §3.3.7/1(5) explicitly extends
class scope through the portion of an out-of-class member definition following
its declarator-id, including the parameter-declaration-clause and function body.
`doc/n3485.txt:2780-2787` proves `lower::word` is visible in both positions. The
explicitly qualified return type precedes that boundary and needs no special rule.

## Parameter point-of-declaration inside deferred defaults (loop 11)

```cpp
typedef int kind;
struct lower { int f(int kind=sizeof(kind)); };
```

Both student and reference accept; the reference renders `sizeof` with a type-id,
whereas the student preserves an id-expression. N3485 §3.3.2/1 puts a name's
point-of-declaration immediately after its complete declarator and before its
initializer (`doc/n3485.txt:2630-2635`). The parameter `kind` has already hidden
the outer typedef when its default argument is parsed. §5.3.3/1
(`:6340-6347`) distinguishes the expression operand from a parenthesized type-id;
therefore this operand is a parameter expression, not a type. The same control
also checks a default containing a lambda parameter with this name, and that a
*later* function parameter does not retrospectively hide a typedef in an earlier
default. A compact declaration-prefix overlay freezes this visibility without
copying environments or chaining one scope per preceding parameter.

## Separate dependent-conversion capability observation

`coverage.py` also qualifies this unimplemented capability with both hosts:

```cpp
template<class value> struct lower {
  operator typename value::type() const;
};
template<class value>
lower<value>::operator typename value::type() const { return 0; }
```

The student and reference both reject it. This is an unfinished template/dependent
syntax group, not a fixed class-only behavior. The uninstantiated pattern is valid
C++11 conversion-function syntax (§12.3.2) with a dependent typename (§14.6).
Host agreement only qualifies this probe; no course reference is revised on that
basis. Raw exits/diagnostics remain in `$RALPH_ARTIFACT_DIR/pa5/remaining-capabilities.json`.

All loop-11 observations use the unchanged bundle revision/hash above. Reduced
source/AST/diagnostic observations also live under
`$RALPH_ARTIFACT_DIR/pa5/classes/reference-reducers/`. No checked-in output,
required comparison, coverage, or reference bundle was changed.
