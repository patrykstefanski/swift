// Executable end-to-end test: a C++ program calls overriding virtual methods
// of foreign reference types whose bodies are provided in Swift via
// `@cxx @implementation`. Built twice: with Base's methods defined in C++, and
// in Swift (SWIFT_BASE).

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx \
// RUN:   -c %s \
// RUN:   -I %S/Inputs \
// RUN:   -o %t/foreign-reference-virtual-execution-main.o
// RUN: %target-interop-build-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -Xfrontend -disable-availability-checking \
// RUN:   -module-name ForeignReferenceVirtualExecutionMain \
// RUN:   -parse-as-library \
// RUN:   -I %S/Inputs \
// RUN:   -Xlinker %t/foreign-reference-virtual-execution-main.o \
// RUN:   %S/Inputs/foreign-reference-virtual-execution.swift \
// RUN:   -o %t/foreign-reference-virtual-execution
// RUN: %target-codesign %t/foreign-reference-virtual-execution
// RUN: %target-run %t/foreign-reference-virtual-execution \
// RUN:   | %FileCheck %s --check-prefixes=CHECK,CXX-BASE
//
// RUN: %target-interop-build-clangxx \
// RUN:   -c %s \
// RUN:   -DSWIFT_BASE \
// RUN:   -I %S/Inputs \
// RUN:   -o %t/foreign-reference-virtual-execution-swift-base-main.o
// RUN: %target-interop-build-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -Xfrontend -disable-availability-checking \
// RUN:   -D SWIFT_BASE \
// RUN:   -module-name ForeignReferenceVirtualExecutionMain \
// RUN:   -parse-as-library \
// RUN:   -I %S/Inputs \
// RUN:   -Xlinker %t/foreign-reference-virtual-execution-swift-base-main.o \
// RUN:   %S/Inputs/foreign-reference-virtual-execution.swift \
// RUN:   -o %t/foreign-reference-virtual-execution-swift-base
// RUN: %target-codesign %t/foreign-reference-virtual-execution-swift-base
// RUN: %target-run %t/foreign-reference-virtual-execution-swift-base \
// RUN:   | %FileCheck %s --check-prefixes=CHECK,SWIFT-BASE

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include <stdio.h>

#include "foreign-reference-virtual.h"

// The key functions.
void Base::baseAnchor() {}
void Derived::derivedAnchor() {}
void Leaf::leafAnchor() {}
void AbstractBase::abAnchor() {}
void Concrete::concreteAnchor() {}

#ifndef SWIFT_BASE
// The Swift implementations return 1000 more.
int Base::describe() const { return value; }
int Base::scaled(int factor) const { return value * factor; }
#endif

// Retains minus releases.
static int liveBases = 0;
static int liveAbstractBases = 0;

void retainBase(Base *) { ++liveBases; }
void releaseBase(Base *) { --liveBases; }
void retainAbstractBase(AbstractBase *) { ++liveAbstractBases; }
void releaseAbstractBase(AbstractBase *) { --liveAbstractBases; }

// Dispatch through the vtable.
__attribute__((noinline)) static int callDescribe(const Base *base) {
  return base->describe();
}
__attribute__((noinline)) static int callScaled(const Base *base, int factor) {
  return base->scaled(factor);
}
__attribute__((noinline)) static int callRun(const AbstractBase *base) {
  return base->run();
}

int main() {
  // Derived::describe doubles Base::describe.
  Derived derived;
  derived.value = 21;
  int describe = callDescribe(&derived);
  printf("derived describe=%d live=%d\n", describe, liveBases);
  // CXX-BASE: derived describe=42 live=0
  // SWIFT-BASE: derived describe=2042 live=0

  // Leaf::describe adds one to Derived::describe.
  Leaf leaf;
  leaf.value = 3;
  describe = callDescribe(&leaf);
  printf("leaf describe=%d live=%d\n", describe, liveBases);
  // CXX-BASE: leaf describe=7 live=0
  // SWIFT-BASE: leaf describe=2007 live=0

  // Leaf::scaled triples Base::scaled; Derived does not override it.
  int scaled = callScaled(&leaf, 5);
  printf("leaf scaled=%d live=%d\n", scaled, liveBases);
  // CXX-BASE: leaf scaled=45 live=0
  // SWIFT-BASE: leaf scaled=3045 live=0

  // Concrete::run overrides a pure virtual method.
  Concrete concrete;
  int run = callRun(&concrete);
  printf("run=%d live=%d\n", run, liveAbstractBases);
  // CHECK: run=1 live=0

  return 0;
}
