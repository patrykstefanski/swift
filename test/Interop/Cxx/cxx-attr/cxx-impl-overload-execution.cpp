// Executable end-to-end test: a C++ program calls both `pick(int)` and
// `pick(double)`, each implemented in Swift via @cxx @implementation. Proves
// same-arity overloads are disambiguated by Swift signature and both bodies
// link under their distinct Itanium symbols.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/cxx-impl-overload-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-impl-overload.swift -o %t/cxx-impl-overload -Xlinker %t/cxx-impl-overload-main.o -module-name CxxImplOverload -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-impl-overload.h
// RUN: %target-codesign %t/cxx-impl-overload
// RUN: %target-run %t/cxx-impl-overload | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-impl-overload.h"
#include <cstdio>

int main() {
  printf("i=%d\n", pick(41));      // Swift pick(Int32): 41 + 1
  // CHECK: i=42
  printf("d=%.1f\n", pick(2.0));   // Swift pick(Double): 2.0 + 0.5
  // CHECK: d=2.5
  return 0;
}
