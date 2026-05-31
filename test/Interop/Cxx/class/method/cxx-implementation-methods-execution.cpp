// Executable end-to-end test: a C++ program calls methods of `Vec` whose bodies
// are provided in Swift via `@cxx @implementation`. This proves the emitted
// symbols use the C++ method ABI (static method with no `this`, const/non-const
// instance methods passing `this` first) so that a C++ caller links and runs.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/cxx-impl-methods-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-implementation-methods.swift -o %t/cxx-impl-methods -Xlinker %t/cxx-impl-methods-main.o -module-name CxxImplMethods -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-implementation-methods.h
// RUN: %target-codesign %t/cxx-impl-methods
// RUN: %target-run %t/cxx-impl-methods | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-implementation-methods.h"
#include <cstdio>

int main() {
  Vec a = Vec::make(6);    // static method implemented in Swift
  Vec b{7};
  printf("dot=%d\n", a.dot(b));  // const instance method: 6 * 7
  // CHECK: dot=42

  Vec c{5};
  c.scale(4);              // non-const instance method: 5 * 4
  printf("scaled=%d\n", c.x);
  // CHECK: scaled=20
  return 0;
}
