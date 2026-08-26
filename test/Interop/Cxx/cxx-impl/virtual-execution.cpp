// Executable end-to-end test: the Swift module implements the key functions of
// most classes in virtual.h and so emits their vtables, RTTI, and adjusting
// thunks; this C++ file defines the remaining key functions and observes
// dispatch through base-class pointers, RTTI, and destruction through a
// Swift-emitted vtable.

// RUN: %empty-directory(%t)
// RUN: %target-interop-build-clangxx \
// RUN:   -c %s \
// RUN:   -I %S/Inputs \
// RUN:   -o %t/virtual-execution-main.o
// RUN: %target-interop-build-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -Xfrontend -disable-availability-checking \
// RUN:   -module-name VirtualExecutionMain \
// RUN:   -parse-as-library \
// RUN:   -I %S/Inputs \
// RUN:   -Xlinker %t/virtual-execution-main.o \
// RUN:   %S/Inputs/virtual-execution.swift \
// RUN:   -o %t/virtual-execution
// RUN: %target-codesign %t/virtual-execution
// RUN: %target-run %t/virtual-execution | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

#include <stdio.h>
#include <typeinfo>

#include "virtual.h"

// The key functions that stay in C++: this translation unit emits the vtables
// of SimpleBase, SimpleDerived, CloneDerived, and the base classes.
void SimpleBase::sbAnchor() {}
void SimpleDerived::sdAnchor() {}
RetB *CloneBase::clone() { return nullptr; }
void CloneDerived::cloneAnchor() {}
void MIBaseA::firstA() {}
int MIBaseB::fromB() const { return -1; }
int VBase::vbMethod() const { return -1; }

static RetC sharedRetCStorage;
RetC *sharedRetC() { return &sharedRetCStorage; }

int destroyedMIBaseA = 0;

// Retains minus releases.
static int liveEngines = 0;

void retainEngine(Engine *) { ++liveEngines; }
void releaseEngine(Engine *) { --liveEngines; }
void retainAbstractEngine(AbstractEngine *) { ++liveEngines; }
void releaseAbstractEngine(AbstractEngine *) { --liveEngines; }

// Dispatch virtually through a pointer whose dynamic type the compiler cannot
// see through.
__attribute__((noinline)) static int callArea(const Shape *shape) {
  return shape->area();
}
__attribute__((noinline)) static void callScale(Shape *shape, int factor) {
  shape->scale(factor);
}
__attribute__((noinline)) static int callPerimeter(const Shape *shape) {
  return shape->perimeter();
}
__attribute__((noinline)) static int callSimple(const SimpleBase *simple) {
  return simple->simple();
}
__attribute__((noinline)) static int callAnchor(const Abstract *abstract) {
  return abstract->anchor();
}
__attribute__((noinline)) static int callPureMethod(const Abstract *abstract) {
  return abstract->pureMethod();
}
__attribute__((noinline)) static RetB *callClone(CloneBase *clone) {
  return clone->clone();
}
__attribute__((noinline)) static void callFirstA(MIBaseA *a) { a->firstA(); }
__attribute__((noinline)) static int callFromB(const MIBaseB *b) {
  return b->fromB();
}
__attribute__((noinline)) static int callVbMethod(const VBase *base) {
  return base->vbMethod();
}
__attribute__((noinline)) static int callStatus(const Engine *engine) {
  return engine->status();
}
__attribute__((noinline)) static void callBoost(Engine *engine, int amount) {
  engine->boost(amount);
}
__attribute__((noinline)) static int callAeAnchor(const AbstractEngine *e) {
  return e->aeAnchor();
}
__attribute__((noinline)) static int callPureStatus(const AbstractEngine *e) {
  return e->pureStatus();
}

// A C++ override of a Swift-implemented method: Pentagon's vtable overrides
// area() in C++ and inherits the slot for the Swift-implemented scale().
struct Pentagon : Shape {
  int area() const override { return 5 * sides; }
};

// C++ classes defining the pure virtual methods.
struct Concrete : Abstract {
  int pureMethod() const override { return 1; }
};
struct ConcreteEngine : AbstractEngine {
  int pureStatus() const override { return 2; }
};

int main() {
  Shape shape;
  shape.sides = 4;

  int area = callArea(&shape);
  printf("area=%d\n", area);
  // CHECK: area=16

  callScale(&shape, 3);
  printf("scaled=%d\n", shape.sides);
  // CHECK: scaled=12

  int perimeter = callPerimeter(&shape);
  printf("perimeter=%d\n", perimeter);
  // CHECK: perimeter=48

  // The C++ override wins on a Pentagon; the inherited scale() slot still
  // dispatches to the Swift implementation.
  Pentagon pentagon;
  pentagon.sides = 4;

  int pentagonArea = callArea(&pentagon);
  printf("pentagonArea=%d\n", pentagonArea);
  // CHECK: pentagonArea=20

  callScale(&pentagon, 2);
  printf("pentagonScaled=%d\n", pentagon.sides);
  // CHECK: pentagonScaled=8

  // Shape's RTTI, emitted by the Swift module, serves dynamic_cast and typeid.
  Shape *shapes[] = {&pentagon, &shape};
  int isPentagon0 = dynamic_cast<Pentagon *>(shapes[0]) != nullptr;
  int isPentagon1 = dynamic_cast<Pentagon *>(shapes[1]) != nullptr;
  int isShape1 = typeid(*shapes[1]) == typeid(Shape);
  printf("isPentagon=%d %d isShape=%d\n", isPentagon0, isPentagon1, isShape1);
  // CHECK: isPentagon=1 0 isShape=1

  // The Swift-implemented key function of an abstract class dispatches like
  // any other; the pure slot dispatches to the C++ override.
  Concrete concrete;

  int anchor = callAnchor(&concrete);
  int pureMethod = callPureMethod(&concrete);
  printf("anchor=%d pureMethod=%d\n", anchor, pureMethod);
  // CHECK: anchor=7 pureMethod=1

  // Both the base method and its Swift-implemented override dispatch through
  // the same slot to the dynamic type's implementation.
  SimpleBase base;
  base.stored = 5;

  int baseSimple = callSimple(&base);
  printf("baseSimple=%d\n", baseSimple);
  // CHECK: baseSimple=5

  SimpleDerived derived;
  derived.stored = 7;

  int derivedSimple = callSimple(&derived);
  printf("derivedSimple=%d\n", derivedSimple);
  // CHECK: derivedSimple=14

  // The covariant override, called through the base class, returns through
  // the Swift-emitted return-adjusting thunk: the RetB base of the RetC.
  sharedRetCStorage.a = 1;
  sharedRetCStorage.b = 2;
  CloneDerived cloneDerived;

  RetB *cloned = callClone(&cloneDerived);
  int adjusted = cloned == static_cast<RetB *>(sharedRetC());
  printf("cloneB=%d adjusted=%d\n", cloned->b, adjusted);
  // CHECK: cloneB=2 adjusted=1

  // The override of the non-primary base's method, called through that base,
  // reaches the Swift body through the this-adjusting thunk; the derived
  // class's RTTI serves the cross-cast between the bases.
  MIDerived mi;
  mi.a = 3;
  mi.b = 5;

  callFirstA(&mi);
  int fromB = callFromB(&mi);
  MIBaseB *crossCast = dynamic_cast<MIBaseB *>(static_cast<MIBaseA *>(&mi));
  int crossCastOK = crossCast == static_cast<MIBaseB *>(&mi);
  printf("firstA=%d fromB=%d crossCast=%d\n", mi.a, fromB, crossCastOK);
  // CHECK: firstA=103 fromB=108 crossCast=1

  // Deleting through the base runs the derived class's implicit destructor,
  // which the Swift-emitted vtable names.
  MIBaseA *heap = new MIDerived;
  delete heap;
  printf("destroyed=%d\n", destroyedMIBaseA);
  // CHECK: destroyed=1

  // The override of the virtual base's method, called through that base,
  // reaches the Swift body through the virtual this-adjusting thunk.
  VDerived vDerived;
  vDerived.vd = 9;

  int vbMethod = callVbMethod(&vDerived);
  printf("vbMethod=%d\n", vbMethod);
  // CHECK: vbMethod=9

  Engine engine;
  engine.rpm = 1000;

  int status = callStatus(&engine);
  printf("status=%d live=%d\n", status, liveEngines);
  // CHECK: status=1000 live=0

  callBoost(&engine, 500);
  printf("boosted=%d live=%d\n", engine.rpm, liveEngines);
  // CHECK: boosted=1500 live=0

  ConcreteEngine concreteEngine;

  int aeAnchor = callAeAnchor(&concreteEngine);
  int pureStatus = callPureStatus(&concreteEngine);
  printf("aeAnchor=%d pureStatus=%d live=%d\n", aeAnchor, pureStatus,
         liveEngines);
  // CHECK: aeAnchor=11 pureStatus=2 live=0

  return 0;
}
