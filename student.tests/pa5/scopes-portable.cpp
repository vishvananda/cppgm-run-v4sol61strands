namespace original {
  using word = unsigned long;
  enum class mode : word { zero, one=1, two=word(one)+1 };
  enum status { ready, busy=ready+1, done=busy+1, };
  word fetch(word x) { return x+done; }
}
namespace original { using word_again = word; word next(word_again x) { return fetch(x); } }
namespace shortcut = original;
namespace imported { using namespace shortcut; using shortcut::word; using shortcut::fetch; }
namespace transitive { using namespace imported; }
namespace cycle_a { using word = unsigned long; }
namespace cycle_b { using namespace cycle_a; }
namespace cycle_a { using namespace cycle_b; }
namespace library { inline namespace version { using count = int; count f(count x) { return x; } } }
namespace hidden { namespace { using local = int; } namespace { local a=1; } }
typedef enum { start, finish=start+1 } phase;
typedef original::mode alias_mode;
using second_mode = alias_mode;
int evaluate(int value) {
  using namespace transitive;
  word a=fetch(value);
  { int word=1; word*=value; }
  word b=original::word(value);
  int shortcut=1;
  shortcut::word c=shortcut::fetch(value);
  library::count d=library::f(value);
  second_mode m=second_mode::two;
  if (value) { using namespace cycle_b; word z=1; a+=z; }
  return int(a+b+c+d)+int(m)+shortcut+hidden::a;
}
