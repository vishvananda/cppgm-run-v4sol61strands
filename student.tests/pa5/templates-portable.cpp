namespace patterns {
template<class value> struct box {
  typedef value type;
  box():data{} {}
  box(value x):data(x) {}
  template<class other> struct rebind { typedef other type; };
  template<int n> value get(value x=sizeof(late)) const noexcept(sizeof(late)>0) {return data+x+n;}
  struct late { int member; };
  value data;
};
template<template<class> class item, class value, int count=2> struct owner { item<value> data; };
template<class... values> void consume(values...);
template<class... values> void call(values... x) { consume(x...); }
template<class value> value identity(value x) { return x; }
extern template int identity<int>(int);
template int identity<int>(int);
template<int n> struct number {};
extern template struct number<1>;
template struct number<2>;
number<3> value;
template<class value> struct box<value*> { value* data; };
template<> struct box<void> {};
template<class value> using rebound=typename box<value>::template rebind<int>::type;
template<class value> auto dependent(value x)->decltype(x.template get<2>()) {return x.template get<2>();}
template<class value> struct conversion {operator typename value::type() const;};
template<class value> conversion<value>::operator typename value::type() const {return 0;}
struct tag {}; int next(); struct holder {holder(tag,int);};
void direct() {holder x(tag(),next());}
box<box<number<(8 >> 1)>>> nested;
}
