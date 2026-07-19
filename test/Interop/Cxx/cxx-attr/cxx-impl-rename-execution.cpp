// Executable end-to-end test: a C++ program calls `addImpl` and
// `Calc::combineImpl`, whose bodies are provided in Swift by *differently-named*
// functions (`add` / `combine`) via `@cxx(name: "...")`. This proves the rename
// emits each body under the matched C++ declaration's Itanium symbol -- for both
// a free function and a const instance method -- so a C++ caller links and runs.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/cxx-impl-rename-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-impl-rename.swift -o %t/cxx-impl-rename -Xlinker %t/cxx-impl-rename-main.o -module-name CxxImplRename -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-impl-rename.h
// RUN: %target-codesign %t/cxx-impl-rename
// RUN: %target-run %t/cxx-impl-rename | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-impl-rename.h"
#include <cstdio>

int main() {
  printf("sum=%d\n", addImpl(40, 2));  // Swift `add`
  // CHECK: sum=42

  Calc c{100};
  printf("combine=%d\n", c.combineImpl(11));  // Swift `combine`: 100 + 11
  // CHECK: combine=111
  return 0;
}
