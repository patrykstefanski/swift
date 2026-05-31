// Executable test: deep non-virtual inheritance chain. A C++ program dispatches
// virtually through the *root* base pointer to `L2::f()` (three levels down),
// whose body is provided in Swift. Single inheritance needs no thunks; this
// pins down that Swift emits the leaf's vtable and dispatch reaches the body.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/deep-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-implementation-virtual-deep.swift -o %t/deep -Xlinker %t/deep-main.o -module-name CxxImplVirtualDeep -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-implementation-virtual-deep.h
// RUN: %target-codesign %t/deep
// RUN: %target-run %t/deep | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-implementation-virtual-deep.h"
#include <cstdio>

int main() {
  L2 obj(5);
  const L0 *p = &obj;          // dispatch through the ROOT base
  printf("f=%d\n", p->f());    // virtual -> Swift L2::f
  // CHECK: f=10
  return p->f() == 10 ? 0 : 1;
}
