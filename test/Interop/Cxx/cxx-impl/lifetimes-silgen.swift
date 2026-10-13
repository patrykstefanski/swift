// Verifies how a `@cxx @implementation` function whose C++ declaration carries
// lifetime annotations is lowered. A lifetimebound reference parameter keeps
// the C++ convention, and a non-escapable result depends on it.

// RUN: %target-swift-emit-silgen \
// RUN:   -module-name main \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-feature Lifetimes \
// RUN:   -I %S/Inputs \
// RUN:   %s | %FileCheck %s

// REQUIRES: swift_feature_CxxImplementation
// REQUIRES: swift_feature_Lifetimes

import Lifetimes

// TrackedView viewTracked(const Tracked &t [[clang::lifetimebound]]);
// CHECK-LABEL: sil [asmname "{{.*}}viewTracked{{.*}}"] [ossa] @$s4main11viewTrackedySo0C4ViewVSo0C0VFTo : $@convention(c) (@in_guaranteed Tracked) -> @lifetime(borrow {{.*}}0) @owned TrackedView {
// CHECK:       bb0(%0 : @noImplicitCopy $*Tracked):
// CHECK-NOT:     {{copy_addr|load \[copy\]|load \[take\]|destroy_addr}}
// CHECK:       } // end sil function '$s4main11viewTrackedySo0C4ViewVSo0C0VFTo'
@unsafe @cxx @implementation
@_lifetime(borrow t)
public func viewTracked(_ t: borrowing Tracked) -> TrackedView {
  return TrackedView(value: nil)
}

// IntView viewInt(const int &x [[clang::lifetimebound]]);
// CHECK-LABEL: sil [asmname "{{.*}}viewInt{{.*}}"] [ossa] @$s4main7viewIntySo0C4ViewVs5Int32VFTo : $@convention(c) (@in_guaranteed Int32) -> @lifetime(borrow 0) @owned IntView {
// CHECK:       bb0(%0 : @noImplicitCopy $*Int32):
// CHECK-NEXT:    load [trivial] %0
@unsafe @cxx @implementation
@_lifetime(borrow x)
public func viewInt(_ x: borrowing CInt) -> IntView {
  return IntView(value: nil)
}

// IntView viewMutableInt(int &x [[clang::lifetimebound]]);
// CHECK-LABEL: sil [asmname "{{.*}}viewMutableInt{{.*}}"] [ossa] @$s4main14viewMutableIntySo0D4ViewVs5Int32VzFTo : $@convention(c) (@inout Int32) -> @lifetime(borrow 0) @owned IntView {
@unsafe @cxx @implementation
@_lifetime(&x)
public func viewMutableInt(_ x: inout CInt) -> IntView {
  return IntView(value: nil)
}

extension Owner {
  // const int &Owner::get() const [[clang::lifetimebound]];
  // CHECK-LABEL: sil [asmname "{{.*}}get{{.*}}"] [ossa] @$sSo5OwnerV4mainE3getSPys5Int32VGyFTo : $@convention(cxx_method) (@in_guaranteed Owner) -> UnsafePointer<Int32> {
  // CHECK:       bb0(%0 : $*Owner):
  // CHECK-NEXT:    load [trivial] %0
  @cxx @implementation
  public func get() -> UnsafePointer<CInt> {
    return UnsafePointer(bitPattern: 1)!
  }

  // IntView Owner::view() const [[clang::lifetimebound]];
  // CHECK-LABEL: sil [asmname "{{.*}}view{{.*}}"] [ossa] @$sSo5OwnerV4mainE4viewSo7IntViewVyFTo : $@convention(cxx_method) (@in_guaranteed Owner) -> @lifetime(borrow address_for_deps 0) @owned IntView {
  // CHECK:       bb0(%0 : $*Owner):
  // CHECK-NOT:     load [trivial] %0
  // CHECK:       } // end sil function '$sSo5OwnerV4mainE4viewSo7IntViewVyFTo'
  @cxx @implementation
  @_lifetime(borrow self)
  public func view() -> IntView {
    return IntView(value: nil)
  }

  // IntView Owner::viewOf(const int &x [[clang::lifetimebound]]) const;
  // CHECK-LABEL: sil [asmname "{{.*}}viewOf{{.*}}"] [ossa] @$sSo5OwnerV4mainE6viewOfySo7IntViewVs5Int32VFTo : $@convention(cxx_method) (@in_guaranteed Int32, @in_guaranteed Owner) -> @lifetime(borrow 0) @owned IntView {
  // CHECK:       bb0(%0 : @noImplicitCopy $*Int32, %1 : $*Owner):
  // CHECK-NEXT:    load [trivial] %0
  @unsafe @cxx @implementation
  @_lifetime(borrow x)
  public func viewOf(_ x: borrowing CInt) -> IntView {
    return IntView(value: nil)
  }
}

// A foreign reference type is passed as the reference itself.
// IntView viewNode(Node &n [[clang::lifetimebound]]);
// CHECK-LABEL: sil [asmname "{{.*}}viewNode{{.*}}"] [ossa] @$s4main8viewNodeySo7IntViewVSo0C0VFTo : $@convention(c) (Node) -> @lifetime(borrow 0) @owned IntView {
@cxx @implementation
@_lifetime(borrow n)
public func viewNode(_ n: borrowing Node) -> IntView {
  return IntView(value: nil)
}
