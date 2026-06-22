// Executable end-to-end test: a C++ program calls non-virtual instance methods
// of a foreign-reference type whose bodies are provided in Swift via
// `@cxx @implementation`. Proves the C++-method `this` ABI reaches the Swift body
// with `self` bound as the FRT reference -- a const method reads through it and a
// non-const method mutates through it.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/frtm-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-impl-frt-method.swift -o %t/frtm -Xlinker %t/frtm-main.o -module-name CxxImplFRTMethod -enable-experimental-feature CxxImplementation -parse-as-library -disable-availability-checking -import-objc-header %S/Inputs/cxx-impl-frt-method.h
// RUN: %target-codesign %t/frtm
// RUN: %target-run %t/frtm | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-impl-frt-method.h"
#include <cstdio>
#include <cstdlib>

void Box_retain(Box *) {}
void Box_release(Box *) {}
Box *makeBox(int v) {
  Box *b = (Box *)std::malloc(sizeof(Box));
  b->value = v;
  return b;
}

int main() {
  Box *b = makeBox(10);

  // Const method: reads through `self`.
  printf("get=%d\n", b->get());
  // CHECK: get=10

  // Non-const method: mutates through `self`.
  b->add(5);
  printf("after=%d\n", b->get());
  // CHECK: after=15

  return 0;
}
