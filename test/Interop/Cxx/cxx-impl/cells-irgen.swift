// Verifies that a `@cxx @implementation` function spelling C++ reference
// parameters with `Cell` and `ConstCell` is emitted under the matched
// declaration's mangled symbol, each cell being the reference's pointer, and
// that the body accesses the C++ referent through it.

// RUN: %target-swift-emit-ir \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-feature Cells \
// RUN:   -target %target-swift-6.5-abi-triple \
// RUN:   -I %S/Inputs \
// RUN:   %s | %FileCheck %s --check-prefixes=CHECK,CHECK-%target-abi

// REQUIRES: swift_feature_CxxImplementation
// REQUIRES: swift_feature_Cells

import Cells

// int observe(int &a, int &b);
// CHECK-SYSV-LABEL: define{{.*}} i32 @_Z7observeRiS_(ptr %0, ptr %1)
// CHECK-WIN-LABEL: define{{.*}} i32 @"?observe@@YAHAEAH0@Z"(ptr %0, ptr %1)
// CHECK:         [[A:%.*]] = getelementptr inbounds{{.*}} %Ts5Int32V, ptr %0
// CHECK-NEXT:    store i32 1, ptr [[A]]
// CHECK:         [[B:%.*]] = getelementptr inbounds{{.*}} %Ts5Int32V, ptr %1
// CHECK-NEXT:    store i32 2, ptr [[B]]
// CHECK:         [[A_AGAIN:%.*]] = getelementptr inbounds{{.*}} %Ts5Int32V, ptr %0
// CHECK-NEXT:    load i32, ptr [[A_AGAIN]]
@cxx @implementation
public func observe(_ a: borrowing Cell<CInt>, _ b: borrowing Cell<CInt>)
    -> CInt {
  a.value = 1
  b.value = 2
  return a.value
}

// int readTwice(const int &a, int &b);
// CHECK-SYSV-LABEL: define{{.*}} i32 @_Z9readTwiceRKiRi(ptr %0, ptr %1)
// CHECK-WIN-LABEL: define{{.*}} i32 @"?readTwice@@YAHAEBHAEAH@Z"(ptr %0, ptr %1)
@cxx @implementation
public func readTwice(_ a: borrowing ConstCell<CInt>,
                      _ b: borrowing Cell<CInt>) -> CInt {
  let x = a.value
  b.value = x &+ 99
  return a.value &+ x
}

// int readTracked(const Tracked &t);
// CHECK-SYSV-LABEL: define{{.*}} i32 @_Z11readTrackedRK7Tracked(ptr %0)
// CHECK-WIN-LABEL: define{{.*}} i32 @"?readTracked@@YAHAEBUTracked@@@Z"(ptr %0)
@cxx @implementation
public func readTracked(_ t: borrowing ConstCell<Tracked>) -> CInt {
  return t.withUnsafePointer { $0.pointee.value }
}

// void reseatNode(Node *_Nonnull &p, Node *_Nonnull to);
// CHECK-SYSV-LABEL: define{{.*}} void @_Z10reseatNodeRP4NodeS0_(ptr %0, ptr %1)
// CHECK-WIN-LABEL: define{{.*}} void @"?reseatNode@@YAXAEAPEAUNode@@PEAU1@@Z"(ptr %0, ptr %1)
@unsafe @cxx @implementation
public func reseatNode(_ p: borrowing Cell<Node>, _ to: Node) {
  p.value = to
}

extension Counter {
  // void addTo(int &x) const;
  // CHECK-SYSV-LABEL: define{{.*}} void @_ZNK7Counter5addToERi(ptr %0, ptr %1)
  // CHECK-WIN-LABEL: define{{.*}} void @"?addTo@Counter@@QEBAXAEAH@Z"(ptr %0, ptr %1)
  @unsafe @cxx @implementation
  public func addTo(_ x: borrowing Cell<CInt>) {
    x.value += count
  }
}

extension ns {
  // void increment(int &x);
  // CHECK-SYSV-LABEL: define{{.*}} void @_ZN2ns9incrementERi(ptr %0)
  // CHECK-WIN-LABEL: define{{.*}} void @"?increment@ns@@YAXAEAH@Z"(ptr %0)
  @cxx @implementation
  public static func increment(_ x: borrowing Cell<CInt>) {
    x.value += 1
  }
}
