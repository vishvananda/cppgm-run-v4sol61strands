struct base {
    typedef int word;
    int value;
    base() : value(0) {}
    virtual ~base() {}
    virtual word get() const { return value; }
};
struct derived : virtual public base {
    derived() : base(), extra{1} {}
    ~derived() {}
    operator int() const { return extra; }
    word get() const override { return value + extra; }
    int early(int x = sizeof(late)) noexcept(sizeof(late)>0) {
        late l{};
        struct local {
            int f() { inner x{}; return x.value; }
            struct inner { int value; };
        };
        return l.value + x;
    }
    int extra = sizeof(late);
    struct nested { int f() { late l{}; return l.value; } };
    struct late { int value; };
    unsigned bits:3;
    unsigned :0;
};
struct special {
    special() = default;
    special(const special&) = delete;
    special& operator=(const special&) = default;
    int operator[](int i) const { return i; }
};
namespace detail {
    struct lower {
        typedef int word;
        lower(); ~lower();
        word f(word);
        operator word() const;
        int data;
    };
    lower::lower() : data(1) {}
    lower::~lower() {}
    lower::word lower::f(word x) { word y=x; return y; }
    lower::operator word() const { return data; }
}
typedef struct { int x; } anonymous;
typedef union { int x; long y; } storage;
void member_pointer() { detail::lower::word (detail::lower::*p)(detail::lower::word); (void)p; }
struct attempts {
    attempts() try : x(1) {} catch (...) { throw; }
    int x;
};
struct alignas(16) aligned { alignas(8) int value; };
