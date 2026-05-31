#ifndef TEST_CXX_IMPL_VIRTUAL_DEEP_H
#define TEST_CXX_IMPL_VIRTUAL_DEEP_H
// Deep *non-virtual* single-inheritance chain (3 levels). Swift implements the
// leaf override (the leaf's key function); C++ dispatches through the root base.
struct L0 { int a; L0(int x) : a(x) {} virtual int f() const { return a; } };
struct L1 : L0 { L1(int x) : L0(x) {} };                          // inherits f
struct L2 : L1 { L2(int x) : L1(x) {} int f() const override; };  // Swift impl
#endif
