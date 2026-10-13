// Executable end-to-end test: a C++ program calls functions declared in
// lifetimes.h whose bodies are provided in Swift via `@cxx @implementation`,
// and checks whether the views and pointers they return point at its objects.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx \
// RUN:   -c %s \
// RUN:   -I %S/Inputs \
// RUN:   -o %t/lifetimes-execution-main.o
// RUN: %target-interop-build-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-feature Lifetimes \
// RUN:   -enable-experimental-feature BuiltinModule \
// RUN:   -module-name LifetimesExecutionMain \
// RUN:   -parse-as-library \
// RUN:   -I %S/Inputs \
// RUN:   -Xlinker %t/lifetimes-execution-main.o \
// RUN:   %S/Inputs/lifetimes-execution.swift \
// RUN:   -o %t/lifetimes-execution
// RUN: %target-codesign %t/lifetimes-execution
// RUN: %target-run %t/lifetimes-execution | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation
// REQUIRES: swift_feature_Lifetimes
// REQUIRES: swift_feature_BuiltinModule

#include <stdio.h>

#include "lifetimes.h"

Tracked::Tracked(int v) : value(v) {}
Tracked::Tracked(const Tracked &other) : value(other.value) {}
Tracked::~Tracked() { value = -1; }

static const char *where(const void *p, const void *object) {
  return p == object ? "in place" : "copy";
}

int main() {
  Tracked t(1);
  TrackedView tv = viewTracked(t);
  printf("viewTracked: %s\n", where(tv.value, &t.value));
  // CHECK: viewTracked: in place

  int x = 2;
  IntView v1 = viewInt(x);
  printf("viewInt: %s\n", where(v1.value, &x));
  // CHECK: viewInt: in place

  int y = 3;
  IntView v2 = viewMutableInt(y);
  printf("viewMutableInt: %s\n", where(v2.value, &y));
  // CHECK: viewMutableInt: in place

  int a = 4, b = 5;
  printf("pick: %s\n", where(&pick(a, b), &a));
  // CHECK: pick: in place

  printf("pickMutable: %s\n", where(&pickMutable(a), &a));
  // CHECK: pickMutable: in place

  printf("pointerToInt: %s\n", where(pointerToInt(a), &a));
  // CHECK: pointerToInt: in place

  printf("pointerToTracked: %s\n", where(pointerToTracked(t), &t));
  // CHECK: pointerToTracked: in place

  Owner o{4};
  printf("Owner::get: %s\n", where(&o.get(), &o.data));
  // CHECK: Owner::get: in place

  IntView v3 = o.view();
  printf("Owner::view: %s\n", where(v3.value, &o.data));
  // CHECK: Owner::view: in place

  int z = 5;
  IntView v4 = o.viewOf(z);
  printf("Owner::viewOf: %s\n", where(v4.value, &z));
  // CHECK: Owner::viewOf: in place

  return 0;
}
