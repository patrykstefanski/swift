// Executable end-to-end test: a C++ program dispatches *virtually* (through a
// base pointer) to `Animal::sound()`, whose body is provided in Swift via
// `@cxx @implementation`. This proves the Swift-emitted vtable links (no
// undefined `_ZTV6Animal`) and that virtual dispatch reaches the Swift body.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/cxx-impl-virtual-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-implementation-virtual.swift -o %t/cxx-impl-virtual -Xlinker %t/cxx-impl-virtual-main.o -module-name CxxImplVirtual -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-implementation-virtual.h
// RUN: %target-codesign %t/cxx-impl-virtual
// RUN: %target-run %t/cxx-impl-virtual | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-implementation-virtual.h"
#include <cstdio>

int main() {
  Animal a(4);
  Animal *p = &a;                 // dispatch through a pointer -> via the vtable
  printf("sound=%d\n", p->sound());
  // CHECK: sound=40
  return p->sound() == 40 ? 0 : 1;
}
