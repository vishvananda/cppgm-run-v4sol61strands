typedef unsigned long word;
using callback = int();
using pointer = const word*;
int source();
int expression_forms(int x, int y) {
  int (a);
  a = x;
  int(x + 1);
  word(fresh);
  int (*fp)() = source;
  if (int T1 = x) a += T1 < 2 ? x : y;
  for (int k = 0; k < 3; ++k) a += k;
  int values[3] = {1, 2, 3};
  for (auto value : values) a ^= value;
  auto f = [a, &y](int n) mutable noexcept(true) -> int { return a + n + y; };
  int* p = new int(1);
  int* q = new int[3]{1, 2, 3};
  ::delete[] q;
  delete p;
  return f(a) + fp() + sizeof(pointer) + ((word)(x + y) >> 2);
}
int reference_type(int& value) { return value; }
void may_throw() throw(int);
void no_throw() noexcept(true);
void handlers() try { throw 1; } catch (int x) { if (x) throw; } catch (...) {}
struct owner;
typedef int owner::*member_data;
typedef int (owner::*member_function)(int) const;
void* operator new[](decltype(sizeof(0)));
void operator delete[](void*) noexcept;
void* operator new(decltype(sizeof(0)), void*);
extern "C" int foreign(int, ...) throw();
void allocate(void* storage) {
  new (int);
  new (char*)();
  new (storage) int;
}
word scope(word x) {
  { int word = 2; word = x; }
  word result = (word)(x >> 1);
  auto f = [=, &result] { return result; };
  return f();
}

#define NUMBER 28
static_assert(NUMBER == 28, "owned" " message");
const char* text = "long enough borrowed " "string to cross inline capacity";
int literals() { return NUMBER + '\n'; }
