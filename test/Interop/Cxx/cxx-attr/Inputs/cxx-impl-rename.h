#ifndef TEST_INTEROP_CXX_IMPL_RENAME_H
#define TEST_INTEROP_CXX_IMPL_RENAME_H

// A free function implemented in Swift by a *differently-named* Swift function
// via `@cxx(name: "addImpl")`.
int addImpl(int a, int b);

// A record whose const instance method is implemented in Swift by a
// differently-named Swift method via `@cxx(name: "combineImpl")`.
struct Calc {
  int base;
  int combineImpl(int k) const;
};

#endif
