// Verifies that a `@cxx @implementation` of an overriding virtual method of a
// foreign reference type is emitted under the method's own mangled symbol, and
// that `super` calls the base class's method directly.

// RUN: %target-swift-emit-ir \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -disable-availability-checking \
// RUN:   -I %S/Inputs \
// RUN:   %s -o %t.ll
// RUN: %FileCheck %s --check-prefixes=CHECK,CHECK-%target-abi,CHECK-%target-abi-%target-ptrsize < %t.ll
// RUN: %FileCheck %s --check-prefix=VTABLE-%target-abi < %t.ll

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

extension KeyDerived {
  // int KeyDerived::describe() const override; the key function.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK10KeyDerived8describeEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?describe@KeyDerived@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() + 100 }
}

extension MI {
  // int MI::side() const override; its this-adjusting thunk follows it.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK2MI4sideEv(ptr {{.*}}%0)
  // CHECK-SYSV-64-LABEL: define{{.*}} i32 @_ZThn16_NK2MI4sideEv(ptr {{.*}}%this)
  // CHECK-SYSV-64: getelementptr inbounds i8, ptr %this{{.*}}, i64 -16
  // CHECK-SYSV-32-LABEL: define{{.*}} i32 @_ZThn8_NK2MI4sideEv(ptr {{.*}}%this)
  // CHECK-SYSV-32: getelementptr inbounds i8, ptr %this{{.*}}, i32 -8
  // CHECK-SYSV: call {{.*}}i32 @_ZNK2MI4sideEv(ptr
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?side@MI@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func side() -> Int32 { return value * 10 }

  // int MI::describe() const override;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK2MI8describeEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?describe@MI@@UEBAHXZ"(ptr {{.*}}%0)
  // CHECK-SYSV: invoke {{.*}}i32 @_ZNK4Base8describeEv(ptr
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() * 3 }
}

// KeyDerived's vtable and RTTI; the other key functions stay in C++.
// VTABLE-SYSV-DAG: @_ZTV10KeyDerived = {{(dso_local )?}}constant { [5 x ptr] } { [5 x ptr] [ptr null, ptr @_ZTI10KeyDerived, ptr @_ZN4Base10baseAnchorEv, ptr @_ZNK10KeyDerived8describeEv, ptr @_ZNK4Base6scaledEi] }
// VTABLE-SYSV-DAG: @_ZTI10KeyDerived = {{(dso_local )?}}constant { ptr, ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv120__si_class_type_infoE, i{{32|64}} 2), ptr @_ZTS10KeyDerived, ptr @_ZTI4Base }
// VTABLE-SYSV-DAG: @_ZTS10KeyDerived = {{(dso_local )?}}constant [13 x i8] c"10KeyDerived\00"
// VTABLE-SYSV-DAG: @_ZTI4Base = external {{(dso_local )?}}constant ptr
// VTABLE-SYSV-NOT: @_ZTV4Base
// VTABLE-SYSV-NOT: @_ZTV2MI
// The Microsoft ABI has no key functions.
// VTABLE-WIN-NOT: @"??_7
