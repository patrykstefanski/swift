// Verifies that a `@cxx @implementation` of an overriding virtual method of a
// foreign reference type is emitted under the method's own mangled symbol, and
// that `super` calls the base class's method directly.

// RUN: %target-swift-emit-ir \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -disable-availability-checking \
// RUN:   -I %S/Inputs \
// RUN:   %s -o %t.ll
// RUN: %FileCheck %s --check-prefixes=CHECK,CHECK-%target-abi < %t.ll
// RUN: %FileCheck %s --check-prefix=NOVTABLE < %t.ll

// REQUIRES: swift_feature_CxxImplementation

import ForeignReferenceVirtual


// `super` references Base::describe before Swift defines it.
extension Derived {
  // int Derived::describe() const override;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK7Derived8describeEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?describe@Derived@@UEBAHXZ"(ptr {{.*}}%0)
  // CHECK-SYSV: invoke {{.*}}i32 @_ZNK4Base8describeEv(ptr
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() * 2 }
}

extension Base {
  // virtual int Base::describe() const;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK4Base8describeEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?describe@Base@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func describe() -> Int32 { return value }
}

extension Leaf {
  // int Leaf::describe() const override;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK4Leaf8describeEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?describe@Leaf@@UEBAHXZ"(ptr {{.*}}%0)
  // CHECK-SYSV: invoke {{.*}}i32 @_ZNK7Derived8describeEv(ptr
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() + 1 }

  // int Leaf::scaled(int factor) const override;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK4Leaf6scaledEi(ptr {{.*}}%0, i32 %1)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?scaled@Leaf@@UEBAHH@Z"(ptr {{.*}}%0, i32 %1)
  // CHECK-SYSV: invoke {{.*}}i32 @_ZNK4Base6scaledEi(ptr
  @cxx @implementation
  public func scaled(_ factor: Int32) -> Int32 { return super.scaled(factor) * 3 }
}

extension Concrete {
  // int Concrete::run() const override;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK8Concrete3runEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?run@Concrete@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func run() -> Int32 { return 1 }
}

// The key functions stay in C++.
// NOVTABLE-NOT: @_ZTV
// NOVTABLE-NOT: @"??_7
