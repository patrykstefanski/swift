// Executable test: virtual inheritance. A C++ program dispatches virtually
// through a *virtual* base (`VVB`) to `VVD::vf()`, whose body is in Swift. This
// exercises the VTT and the virtual-base-adjusting thunk that clang emits for
// the Swift-implemented method.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/vi-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-implementation-virtual-vi.swift -o %t/vi -Xlinker %t/vi-main.o -module-name CxxImplVirtualVI -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-implementation-virtual-vi.h
// RUN: %target-codesign %t/vi
// RUN: %target-run %t/vi | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-implementation-virtual-vi.h"
#include <cstdio>

int main() {
  VVD vd(3);
  VVB *pv = &vd;                // upcast to the virtual base
  printf("vf=%d\n", pv->vf());  // virtual -> virtual-base vtable -> thunk -> VVD::vf (Swift)
  // CHECK: vf=7
  return pv->vf() == 7 ? 0 : 1;
}
