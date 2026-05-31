// Executable test: abstract base (pure virtual). A C++ program dispatches
// virtually through an *abstract base* pointer to `Square::area()`, whose body
// is provided in Swift. This proves Swift emits the derived class's vtable +
// RTTI (with the abstract base's RTTI referenced) and that dispatch through the
// abstract base reaches the Swift body.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/abstract-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-implementation-virtual-abstract.swift -o %t/abstract -Xlinker %t/abstract-main.o -module-name CxxImplVirtualAbstract -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-implementation-virtual-abstract.h
// RUN: %target-codesign %t/abstract
// RUN: %target-run %t/abstract | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-implementation-virtual-abstract.h"
#include <cstdio>

int main() {
  Square s(5);
  const Shape *p = &s;             // dispatch through the ABSTRACT base pointer
  printf("area=%d\n", p->area());  // virtual -> Swift Square::area
  // CHECK: area=25
  return p->area() == 25 ? 0 : 1;
}
