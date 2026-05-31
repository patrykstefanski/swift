#ifndef TEST_CXX_IMPL_VIRTUAL_MI_H
#define TEST_CXX_IMPL_VIRTUAL_MI_H
struct MA { int a; MA(int x) : a(x) {} virtual int af() const { return a; } };
struct MB { int b; MB(int x) : b(x) {} virtual int bf() const { return b; } };
// MC overrides MB::bf (a secondary base) -> the secondary vtable slot needs a
// `this`-adjusting thunk, which clang emits because Swift implements MC::bf.
struct MC : MA, MB {
  int c;
  MC(int x) : MA(x), MB(x * 10), c(x * 100) {}
  int bf() const override;   // implemented in Swift
};
#endif
