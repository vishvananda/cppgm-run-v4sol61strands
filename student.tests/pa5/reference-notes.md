# PA5 independent reference observations (no course output changed)

Observed bundle source revision: `0cfcea43c95bfb8a121f7b1e104a016560ebb3db`;
archive SHA-256 `1b7b2863858e7c265417084e758838efbae6cfb788a5d0dec349d255c329a26c`.
Both reducers below are accepted by GCC and Clang in C++11 mode. Agreement is
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
