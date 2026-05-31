#ifndef TEST_CXX_IMPL_VIRTUAL_VI_H
#define TEST_CXX_IMPL_VIRTUAL_VI_H
struct VVB { int v; VVB(int x) : v(x) {} virtual int vf() const { return v; } };
// VVD has a *virtual* base -> the vtable needs a VTT and a virtual-base thunk,
// both emitted by clang because Swift implements VVD::vf.
struct VVD : virtual VVB {
  int d;
  VVD(int x) : VVB(x), d(x * 10) {}
  int vf() const override;   // implemented in Swift
};
#endif
