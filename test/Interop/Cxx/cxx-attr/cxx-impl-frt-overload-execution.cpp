// Executable end-to-end test: a C++ program calls a foreign-reference type's
// same-arity instance-method overloads -- `combine(int)` and `combine(double)`,
// distinguished by parameter type -- whose bodies are provided in Swift via
// `@cxx @implementation`. Proves that FRT receiver methods and same-arity
// overload disambiguation compose: each Swift impl binds to the C++ overload
// whose parameter type matches, both link under their distinct Itanium symbols
// (`_ZNK4Calc7combineEi` / `_ZNK4Calc7combineEd`), and `self` reaches each body
// as the FRT reference.
//
// (A const/non-const overload pair with *identical* parameters cannot be
// disambiguated for an FRT and is instead a compile-time error -- see
// decl/ext/cxx_implementation_foreign_reference.swift -- so it has no executable
// counterpart.)

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/frto-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-impl-frt-overload.swift -o %t/frto -Xlinker %t/frto-main.o -module-name CxxImplFRTOverload -enable-experimental-feature CxxImplementation -parse-as-library -disable-availability-checking -import-objc-header %S/Inputs/cxx-impl-frt-overload.h
// RUN: %target-codesign %t/frto
// RUN: %target-run %t/frto | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-impl-frt-overload.h"
#include <cstdio>
#include <cstdlib>

void Calc_retain(Calc *) {}
void Calc_release(Calc *) {}
Calc *makeCalc(int v) {
  Calc *c = (Calc *)std::malloc(sizeof(Calc));
  c->base = v;
  return c;
}

int main() {
  Calc *c = makeCalc(100);

  // int overload: Swift combine(Int32) -> base + x.
  printf("i=%d\n", c->combine(41));
  // CHECK: i=141

  // double overload: Swift combine(Double) -> Double(base) + x.
  printf("d=%.1f\n", c->combine(2.5));
  // CHECK: d=102.5

  return 0;
}
