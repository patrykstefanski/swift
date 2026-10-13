// Executable end-to-end test: a C++ program calls functions declared in
// cells.h whose reference parameters are spelled `Cell` and `ConstCell` in
// their Swift `@cxx @implementation` bodies, passing aliasing references and
// references to storage the bodies reach otherwise, and checks that every
// call has the C++ result.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx \
// RUN:   -c %s \
// RUN:   -I %S/Inputs \
// RUN:   -o %t/cells-execution-main.o
// RUN: %target-interop-build-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-feature Cells \
// RUN:   -enable-experimental-feature Lifetimes \
// RUN:   -target %target-swift-6.5-abi-triple \
// RUN:   -module-name CellsExecutionMain \
// RUN:   -parse-as-library \
// RUN:   -I %S/Inputs \
// RUN:   -Xlinker %t/cells-execution-main.o \
// RUN:   %S/Inputs/cells-execution.swift \
// RUN:   -o %t/cells-execution
// RUN: %target-codesign %t/cells-execution
// RUN: %target-run %t/cells-execution | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: stdlib_6_5_runtime
// REQUIRES: swift_feature_CxxImplementation
// REQUIRES: swift_feature_Cells
// REQUIRES: swift_feature_Lifetimes

#include <stdio.h>

#include "cells.h"

int global = 0;
int *_Nullable bumpTarget = nullptr;
void bump() { ++*bumpTarget; }

static int copies = 0;
static int live = 0;

Tracked::Tracked(int v) : value(v) { ++live; }
Tracked::Tracked(const Tracked &other) : value(other.value) {
  ++copies;
  ++live;
}
Tracked::~Tracked() { --live; }

MoveOnly::MoveOnly(int v) : value(v) { ++live; }
MoveOnly::MoveOnly(MoveOnly &&other) : value(other.value) { ++live; }
MoveOnly::~MoveOnly() { --live; }

int main() {
  // Aliasing references and storage the body reaches otherwise.

  int o = 0;
  printf("observe(o, o): %d\n", observe(o, o));
  // CHECK: observe(o, o): 2

  int d = 100;
  printf("readTwice(d, d): %d\n", readTwice(d, d));
  // CHECK-NEXT: readTwice(d, d): 299

  printf("observeGlobal(global): %d\n", observeGlobal(global));
  // CHECK-NEXT: observeGlobal(global): 7

  global = 40;
  printf("readGlobal(global): %d\n", readGlobal(global));
  // CHECK-NEXT: readGlobal(global): 41

  int h = 3;
  bumpTarget = &h;
  printf("readAroundCallback(h): %d\n", readAroundCallback(h));
  // CHECK-NEXT: readAroundCallback(h): 1
  printf("writeAroundCallback(h): %d\n", writeAroundCallback(h));
  // CHECK-NEXT: writeAroundCallback(h): 11

  global = 0;
  bumpTarget = &global;
  printf("readAroundCallback(global): %d\n", readAroundCallback(global));
  // CHECK-NEXT: readAroundCallback(global): 1
  printf("writeAroundCallback(global): %d\n", writeAroundCallback(global));
  // CHECK-NEXT: writeAroundCallback(global): 11

  // Non-trivial and move-only referents are reached in place.

  {
    Tracked t(7);
    copies = 0;
    int read = readTracked(t);
    printf("readTracked: %d copies=%d\n", read, copies);
    // CHECK-NEXT: readTracked: 7 copies=0
    bumpTracked(t);
    printf("bumpTracked: %d copies=%d\n", t.value, copies);
    // CHECK-NEXT: bumpTracked: 8 copies=0
    assignTracked(t, t);
    printf("assignTracked(t, t): %d live=%d\n", t.value, live);
    // CHECK-NEXT: assignTracked(t, t): 8 live=1
    Tracked u(9);
    assignTracked(t, u);
    printf("assignTracked(t, u): %d live=%d\n", t.value, live);
    // CHECK-NEXT: assignTracked(t, u): 9 live=2
  }
  printf("live: %d\n", live);
  // CHECK-NEXT: live: 0

  {
    MoveOnly m(14);
    int read = readMoveOnly(m);
    printf("readMoveOnly: %d\n", read);
    // CHECK-NEXT: readMoveOnly: 14
    bumpMoveOnly(m);
    printf("bumpMoveOnly: %d\n", m.value);
    // CHECK-NEXT: bumpMoveOnly: 15
    replaceMoveOnly(m, 16);
    printf("replaceMoveOnly: %d live=%d\n", m.value, live);
    // CHECK-NEXT: replaceMoveOnly: 16 live=1
  }
  printf("live: %d\n", live);
  // CHECK-NEXT: live: 0

  // Pointer slots.

  int a = 1, b = 2;
  int *_Nullable p = &a;
  reseatPtr(p, &b);
  printf("reseatPtr: %s\n", p == &b ? "points at b" : "unchanged");
  // CHECK-NEXT: reseatPtr: points at b

  int answer = 42;
  int *_Nonnull q = &answer;
  printf("readThroughPtr: %d\n", readThroughPtr(q));
  // CHECK-NEXT: readThroughPtr: 42

  Node first{1}, second{2};
  Node *_Nonnull n = &first;
  reseatNode(n, &second);
  printf("reseatNode: %d\n", n->value);
  // CHECK-NEXT: reseatNode: 2

  // Methods, a namespace member and operators.

  int r = 5;
  Counter::reset(r);
  printf("Counter::reset: %d\n", r);
  // CHECK-NEXT: Counter::reset: 0

  Counter c{12};
  c.addTo(r);
  printf("Counter::addTo: %d\n", r);
  // CHECK-NEXT: Counter::addTo: 12
  c.absorb(r);
  printf("Counter::absorb: %d\n", c.count);
  // CHECK-NEXT: Counter::absorb: 24

  int i = 5;
  ns::increment(i);
  printf("ns::increment: %d\n", i);
  // CHECK-NEXT: ns::increment: 6

  Vector v{1, 2}, w{3, 4};
  v += w;
  printf("v += w: %d %d\n", v.x, v.y);
  // CHECK-NEXT: v += w: 4 6
  printf("v == w: %d, v == v: %d\n", v == w, v == v);
  // CHECK-NEXT: v == w: 0, v == v: 1

  // Overloads.

  int x = 3;
  int (*pickRef)(int &) = pick;
  int (*pickPtr)(int *_Nonnull) = pick;
  printf("pick: %d %d\n", pickRef(x), pickPtr(&x));
  // CHECK-NEXT: pick: 30 300
  int (*projectRef)(const int &) = project;
  printf("project: %d\n", projectRef(x));
  // CHECK-NEXT: project: 4

  // A lifetimebound reference.

  IntView view = viewInt(x);
  printf("viewInt: %s\n", view.value == &x ? "in place" : "copy");
  // CHECK-NEXT: viewInt: in place

  return 0;
}
