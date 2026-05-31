// Executable test: multiple inheritance. A C++ program dispatches virtually
// through a *secondary* base subobject (`MB`) to `MC::bf()`, whose body is in
// Swift. This exercises the `this`-adjusting vtable thunk that clang emits for
// the Swift-implemented method (otherwise it would be an undefined reference).

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/mi-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-implementation-virtual-mi.swift -o %t/mi -Xlinker %t/mi-main.o -module-name CxxImplVirtualMI -enable-experimental-feature CxxImplementation -parse-as-library -import-objc-header %S/Inputs/cxx-implementation-virtual-mi.h
// RUN: %target-codesign %t/mi
// RUN: %target-run %t/mi | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-implementation-virtual-mi.h"
#include <cstdio>

int main() {
  MC c(1);
  MB *pb = &c;                  // adjusted to the MB subobject
  printf("bf=%d\n", pb->bf());  // virtual -> MB-subobject vtable -> thunk -> MC::bf (Swift)
  // CHECK: bf=42
  return pb->bf() == 42 ? 0 : 1;
}
