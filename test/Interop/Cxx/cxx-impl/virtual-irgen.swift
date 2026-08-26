// Verifies that a `@cxx @implementation` of a C++ virtual method is emitted
// under the method's own mangled symbol, and that alongside it Swift emits
// what C++ emits with the method's definition: for a key function the class's
// vtable, VTT, and RTTI, and for an override the this-adjusting thunks, and
// the return-adjusting thunk of a covariant return type. A class whose key
// function stays in C++ gets no vtable from Swift. Swift-side calls keep their
// dispatch: static (the plain method symbol) for a value record, dynamic (the
// importer's synthesized thunk) for a foreign reference type.

// RUN: %target-swift-emit-ir \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -disable-availability-checking \
// RUN:   -I %S/Inputs \
// RUN:   %s -o %t.ll
// RUN: %FileCheck %s --check-prefixes=CHECK,CHECK-%target-abi < %t.ll
// RUN: %FileCheck %s --check-prefix=NOVTABLE-%target-abi < %t.ll

// REQUIRES: swift_feature_CxxImplementation

import Virtual


// The vtables, VTTs, and RTTI of the classes whose key function is implemented
// below. The Microsoft ABI has no key functions: its vftables are emitted by
// every translation unit constructing an object, so Swift emits none.

// CHECK-SYSV: @_ZTV5Shape = {{(dso_local )?}}unnamed_addr constant { [5 x ptr] } { [5 x ptr] [ptr null, ptr @_ZTI5Shape, ptr @_ZNK5Shape4areaEv, ptr @_ZN5Shape5scaleEi, ptr @_ZNK5Shape9perimeterEv] }
// CHECK-SYSV: @_ZTI5Shape = {{(dso_local )?}}constant
// CHECK-SYSV: @_ZTS5Shape = {{(dso_local )?}}constant [7 x i8] c"5Shape\00"

// CHECK-SYSV: @_ZTV8Abstract = {{(dso_local )?}}unnamed_addr constant { [4 x ptr] } { [4 x ptr] [ptr null, ptr @_ZTI8Abstract, ptr @_ZNK8Abstract6anchorEv, ptr @__cxa_pure_virtual] }

// CHECK-SYSV: @_ZTV9MIDerived = {{(dso_local )?}}unnamed_addr constant { [7 x ptr], [3 x ptr] } { [7 x ptr] [ptr null, ptr @_ZTI9MIDerived, ptr @_ZN9MIDerivedD2Ev, ptr @_ZN9MIDerivedD0Ev, ptr @_ZN9MIDerived6firstAEv, ptr @_ZN9MIDerived8miAnchorEv, ptr @_ZNK9MIDerived5fromBEv], [3 x ptr] [ptr inttoptr (i64 -16 to ptr), ptr @_ZTI9MIDerived, ptr @_ZThn16_NK9MIDerived5fromBEv] }
// CHECK-SYSV: @_ZTI9MIDerived = {{(dso_local )?}}constant

// CHECK-SYSV: @_ZTV8VDerived = {{(dso_local )?}}unnamed_addr constant { [5 x ptr], [4 x ptr] } { [5 x ptr] [ptr inttoptr (i64 16 to ptr), ptr null, ptr @_ZTI8VDerived, ptr @_ZN8VDerived7vAnchorEv, ptr @_ZNK8VDerived8vbMethodEv], [4 x ptr] [ptr inttoptr (i64 -16 to ptr), ptr inttoptr (i64 -16 to ptr), ptr @_ZTI8VDerived, ptr @_ZTv0_n24_NK8VDerived8vbMethodEv] }
// CHECK-SYSV: @_ZTT8VDerived = {{(dso_local )?}}unnamed_addr constant [2 x ptr]

// CHECK-SYSV: @_ZTV6Engine = {{(dso_local )?}}unnamed_addr constant { [4 x ptr] } { [4 x ptr] [ptr null, ptr @_ZTI6Engine, ptr @_ZNK6Engine6statusEv, ptr @_ZN6Engine5boostEi] }

// CHECK-SYSV: @_ZTV14AbstractEngine = {{(dso_local )?}}unnamed_addr constant { [4 x ptr] } { [4 x ptr] [ptr null, ptr @_ZTI14AbstractEngine, ptr @_ZNK14AbstractEngine8aeAnchorEv, ptr @__cxa_pure_virtual] }

// A class whose key function stays in C++ gets no vtable from Swift.
// NOVTABLE-SYSV-NOT: @_ZTV10SimpleBase
// NOVTABLE-SYSV-NOT: @_ZTV13SimpleDerived
// NOVTABLE-SYSV-NOT: @_ZTV12CloneDerived
// NOVTABLE-WIN-NOT: @"??_7


extension Shape {
  // virtual int Shape::area() const; the key function.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK5Shape4areaEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?area@Shape@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func area() -> Int32 { return sides * sides }

  // virtual void Shape::scale(int factor);
  // CHECK-SYSV-LABEL: define{{.*}} void @_ZN5Shape5scaleEi(ptr {{.*}}%0, i32 {{.*}}%1)
  // CHECK-WIN-LABEL: define{{.*}} void @"?scale@Shape@@UEAAXH@Z"(ptr {{.*}}%0, i32 {{.*}}%1)
  @cxx @implementation
  public mutating func scale(_ factor: Int32) { sides *= factor }

  // The vtable names the inline virtual int Shape::perimeter() const, so its
  // body is emitted too.
  // CHECK-SYSV: define linkonce_odr{{.*}} i32 @_ZNK5Shape9perimeterEv(
}

extension SimpleBase {
  // virtual int SimpleBase::simple() const;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK10SimpleBase6simpleEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?simple@SimpleBase@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func simple() -> Int32 { return stored }
}

// An override along single, non-virtual inheritance needs no thunk.
extension SimpleDerived {
  // int SimpleDerived::simple() const override;
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK13SimpleDerived6simpleEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?simple@SimpleDerived@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func simple() -> Int32 { return stored * 2 }
}

extension Abstract {
  // virtual int Abstract::anchor() const; the key function.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK8Abstract6anchorEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?anchor@Abstract@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func anchor() -> Int32 { return 7 }
}

extension CloneDerived {
  // RetC *CloneDerived::clone() override; the return-adjusting thunk to the
  // RetB base, named by the vtable C++ emits, follows.
  // CHECK-SYSV-LABEL: define{{.*}} ptr @_ZN12CloneDerived5cloneEv(ptr {{.*}}%0)
  // CHECK-SYSV-LABEL: define{{.*}} ptr @_ZTch0_h4_N12CloneDerived5cloneEv(ptr {{.*}}%this)
  // CHECK-SYSV:   call {{.*}}ptr @_ZN12CloneDerived5cloneEv(ptr {{.*}}%this1)
  // CHECK-SYSV:   getelementptr inbounds i8, ptr %call, i64 4
  // CHECK-WIN-LABEL: define{{.*}} ptr @"?clone@CloneDerived@@UEAAPEAURetC@@XZ"(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define weak_odr{{.*}} ptr @"?clone@CloneDerived@@QEAAPEAURetB@@XZ"(ptr {{.*}}%0)
  @cxx @implementation
  public mutating func clone() -> UnsafeMutablePointer<RetC> {
    return sharedRetC()
  }
}

extension MIDerived {
  // virtual void MIDerived::miAnchor(); the key function. The vtable names
  // the class's implicit destructor, which the primary base's virtual
  // destructor makes virtual, so its body is emitted too.
  // CHECK-SYSV-LABEL: define{{.*}} void @_ZN9MIDerived8miAnchorEv(ptr {{.*}}%0)
  // CHECK-SYSV: define linkonce_odr{{.*}} void @_ZN9MIDerivedD2Ev(
  // CHECK-SYSV: define linkonce_odr{{.*}} void @_ZN9MIDerivedD0Ev(
  // CHECK-WIN-LABEL: define{{.*}} void @"?miAnchor@MIDerived@@UEAAXXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public mutating func miAnchor() {}

  // void MIDerived::firstA() override; overrides the primary base, at offset
  // zero: no thunk.
  // CHECK-SYSV-LABEL: define{{.*}} void @_ZN9MIDerived6firstAEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} void @"?firstA@MIDerived@@UEAAXXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public mutating func firstA() { a += 100 }

  // int MIDerived::fromB() const override; overrides the non-primary base:
  // the this-adjusting thunk in MIBaseB's secondary vtable follows.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK9MIDerived5fromBEv(ptr {{.*}}%0)
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZThn16_NK9MIDerived5fromBEv(ptr {{.*}}%this)
  // CHECK-SYSV:   [[ADJUSTED:%.*]] = getelementptr inbounds i8, ptr %this1, i64 -16
  // CHECK-SYSV:   call {{.*}}i32 @_ZNK9MIDerived5fromBEv(ptr {{.*}}[[ADJUSTED]])
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?fromB@MIDerived@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func fromB() -> Int32 { return a + b }
}

extension VDerived {
  // virtual void VDerived::vAnchor(); the key function.
  // CHECK-SYSV-LABEL: define{{.*}} void @_ZN8VDerived7vAnchorEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} void @"?vAnchor@VDerived@@UEAAXXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public mutating func vAnchor() {}

  // int VDerived::vbMethod() const override; overrides the virtual base: the
  // this-adjusting thunk through the vcall offset in VBase's vtable follows.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK8VDerived8vbMethodEv(ptr {{.*}}%0)
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZTv0_n24_NK8VDerived8vbMethodEv(ptr {{.*}}%this)
  // CHECK-SYSV:   [[VTABLE:%.*]] = load ptr, ptr %this1
  // CHECK-SYSV:   [[VCALL_OFFSET_ADDR:%.*]] = getelementptr inbounds i8, ptr [[VTABLE]], i64 -24
  // CHECK-SYSV:   [[VCALL_OFFSET:%.*]] = load i64, ptr [[VCALL_OFFSET_ADDR]]
  // CHECK-SYSV:   [[ADJUSTED:%.*]] = getelementptr inbounds i8, ptr %this1, i64 [[VCALL_OFFSET]]
  // CHECK-SYSV:   call {{.*}}i32 @_ZNK8VDerived8vbMethodEv(ptr {{.*}}[[ADJUSTED]])
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?vbMethod@VDerived@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func vbMethod() -> Int32 { return vd }
}

extension Engine {
  // virtual int Engine::status() const; the key function. `self` is the
  // reference, i.e. `this`.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK6Engine6statusEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?status@Engine@@UEBAHXZ"(ptr {{.*}}%0)
  // CHECK: getelementptr inbounds{{.*}} %TSo6EngineV, ptr %0
  @cxx @implementation
  public func status() -> Int32 { return rpm }

  // virtual void Engine::boost(int amount);
  // CHECK-SYSV-LABEL: define{{.*}} void @_ZN6Engine5boostEi(ptr {{.*}}%0, i32 {{.*}}%1)
  // CHECK-WIN-LABEL: define{{.*}} void @"?boost@Engine@@UEAAXH@Z"(ptr {{.*}}%0, i32 {{.*}}%1)
  @cxx @implementation
  public func boost(_ amount: Int32) { rpm += amount }
}

extension AbstractEngine {
  // virtual int AbstractEngine::aeAnchor() const; the key function.
  // CHECK-SYSV-LABEL: define{{.*}} i32 @_ZNK14AbstractEngine8aeAnchorEv(ptr {{.*}}%0)
  // CHECK-WIN-LABEL: define{{.*}} i32 @"?aeAnchor@AbstractEngine@@UEBAHXZ"(ptr {{.*}}%0)
  @cxx @implementation
  public func aeAnchor() -> Int32 { return 11 }
}


// Swift-side calls: a value record's virtual method dispatches statically to
// the method symbol itself; a foreign reference type's dispatches dynamically
// through the importer's synthesized thunk.

// CHECK-LABEL: define{{.*}} swiftcc i32 @"$s{{.*}}16callVirtualFuncsys5Int32VSo5ShapeVz_So6EngineVtF"
// CHECK-SYSV:   invoke void @_ZN5Shape5scaleEi(ptr {{.*}}%0, i32 {{.*}}2)
// CHECK-SYSV:   invoke void @_ZN6Engine30__synthesizedVirtualCall_boostEi(ptr %1, i32 3)
// CHECK-SYSV:   invoke i32 @_ZNK5Shape4areaEv(ptr %0)
// CHECK-SYSV:   invoke i32 @_ZNK6Engine31__synthesizedVirtualCall_statusEv(ptr %1)
public func callVirtualFuncs(_ s: inout Shape, _ e: Engine) -> Int32 {
  s.scale(2)
  e.boost(3)
  return s.area() + e.status()
}

// CHECK-SYSV: define linkonce_odr{{.*}} void @_ZN6Engine30__synthesizedVirtualCall_boostEi
// CHECK-SYSV: define linkonce_odr{{.*}} i32 @_ZNK6Engine31__synthesizedVirtualCall_statusEv
