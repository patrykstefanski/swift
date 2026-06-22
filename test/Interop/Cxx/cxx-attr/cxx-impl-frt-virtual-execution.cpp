// Executable end-to-end test: a C++ program dispatches *virtually* (through the
// vtable) to `Animal::sound()` on a foreign-reference type, whose body is provided
// in Swift via `@cxx @implementation`. Proves the Swift-emitted vtable links (no
// undefined `_ZTV6Animal`) and that virtual dispatch reaches the Swift body with
// `self` bound as the FRT reference.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/frtv-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-impl-frt-virtual.swift -o %t/frtv -Xlinker %t/frtv-main.o -module-name CxxImplFRTVirtual -enable-experimental-feature CxxImplementation -parse-as-library -disable-availability-checking -import-objc-header %S/Inputs/cxx-impl-frt-virtual.h
// RUN: %target-codesign %t/frtv
// RUN: %target-run %t/frtv | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-impl-frt-virtual.h"
#include <cstdio>

void Animal_retain(Animal *) {}
void Animal_release(Animal *) {}
Animal *makeAnimal(int id) {
  Animal *a = new Animal();  // sets the vtable pointer to the Swift-emitted vtable
  a->id = id;
  return a;
}

int main() {
  Animal *a = makeAnimal(4);
  // Virtual call through the pointer -> the Swift-emitted vtable slot -> Swift body.
  printf("sound=%d\n", a->sound());
  // CHECK: sound=40
  return 0;
}
