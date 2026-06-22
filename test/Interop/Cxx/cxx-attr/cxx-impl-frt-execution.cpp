// Executable end-to-end + refcount test: a C++ program calls Swift-implemented
// functions that take/return a foreign-reference type, and counts retain/release
// calls to verify the supported phase-1 ownership conventions:
//   * a borrowed (+0) parameter incurs no net retain;
//   * a `SWIFT_RETURNS_RETAINED` result is +1 (one net retain handed to the caller).

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx -c %s -I %S/Inputs -o %t/frt-main.o
// RUN: %target-interop-build-swift %S/Inputs/cxx-impl-frt-exec.swift -o %t/frt -Xlinker %t/frt-main.o -module-name CxxImplFRT -enable-experimental-feature CxxImplementation -parse-as-library -disable-availability-checking -import-objc-header %S/Inputs/cxx-impl-frt.h
// RUN: %target-codesign %t/frt
// RUN: %target-run %t/frt | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include "cxx-impl-frt.h"
#include <cstdio>

static int liveRetains = 0;
void Node_retain(Node *n) { ++liveRetains; }
void Node_release(Node *n) { --liveRetains; }

int main() {
  Node n{42};

  // Borrowed parameter: no net retain.
  printf("v=%d r=%d\n", valueOf(&n), liveRetains);
  // CHECK: v=42 r=0

  // SWIFT_RETURNS_RETAINED: caller receives +1 (exactly one retain).
  Node *d = dup(&n);
  printf("dup=%d r=%d\n", d->value, liveRetains);
  // CHECK: dup=42 r=1

  return 0;
}
