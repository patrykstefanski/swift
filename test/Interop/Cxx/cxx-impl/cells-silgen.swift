// Verifies how a `@cxx @implementation` function spelling C++ reference
// parameters with `Cell` and `ConstCell` is lowered: each is a `borrowing`
// parameter passed `@in_guaranteed`, the address of the C++ referent, which
// the body reaches only through the cell.

// RUN: %target-swift-emit-silgen \
// RUN:   -module-name main \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-feature Cells \
// RUN:   -enable-experimental-feature Lifetimes \
// RUN:   -target %target-swift-6.5-abi-triple \
// RUN:   -I %S/Inputs \
// RUN:   %s | %FileCheck %s

// REQUIRES: swift_feature_CxxImplementation
// REQUIRES: swift_feature_Cells
// REQUIRES: swift_feature_Lifetimes

import Cells

// int observe(int &a, int &b);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}observe{{.*}}"] [ossa] @$s4main7observeys5Int32Vs4CellVyADG_AGtFTo : $@convention(c) (@in_guaranteed Cell<Int32>, @in_guaranteed Cell<Int32>) -> Int32 {
// CHECK:       bb0(%0 : $*Cell<Int32>, %1 : $*Cell<Int32>):
// CHECK:         mark_unresolved_non_copyable_value [no_consume_or_assign] %0
// CHECK:         mark_unresolved_non_copyable_value [no_consume_or_assign] %1
// CHECK-NOT:     {{copy_addr|load \[copy\]|load \[take\]|destroy_addr}}
// CHECK:       } // end sil function '$s4main7observeys5Int32Vs4CellVyADG_AGtFTo'
@cxx @implementation
public func observe(_ a: borrowing Cell<CInt>, _ b: borrowing Cell<CInt>)
    -> CInt {
  a.value = 1
  b.value = 2
  return a.value
}

// int readTwice(const int &a, int &b);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}readTwice{{.*}}"] [ossa] @$s4main9readTwiceys5Int32Vs9ConstCellVyADG_s0F0VyADGtFTo : $@convention(c) (@in_guaranteed ConstCell<Int32>, @in_guaranteed Cell<Int32>) -> Int32 {
@cxx @implementation
public func readTwice(_ a: borrowing ConstCell<CInt>,
                      _ b: borrowing Cell<CInt>) -> CInt {
  let x = a.value
  b.value = x &+ 99
  return a.value &+ x
}

// A non-trivial or move-only referent is neither copied nor destroyed.

// int readTracked(const Tracked &t);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}readTracked{{.*}}"] [ossa] @$s4main11readTrackedys5Int32Vs9ConstCellVySo0C0VGFTo : $@convention(c) (@in_guaranteed ConstCell<Tracked>) -> Int32 {
// CHECK:       bb0(%0 : $*ConstCell<Tracked>):
// CHECK-NOT:     {{copy_addr|load \[copy\]|load \[take\]|destroy_addr}}
// CHECK:       } // end sil function '$s4main11readTrackedys5Int32Vs9ConstCellVySo0C0VGFTo'
@cxx @implementation
public func readTracked(_ t: borrowing ConstCell<Tracked>) -> CInt {
  return t.withUnsafePointer { $0.pointee.value }
}

// void bumpMoveOnly(MoveOnly &m);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}bumpMoveOnly{{.*}}"] [ossa] @$s4main12bumpMoveOnlyyys4CellVySo0cD0VGFTo : $@convention(c) (@in_guaranteed Cell<MoveOnly>) -> () {
// CHECK:       bb0(%0 : $*Cell<MoveOnly>):
// CHECK-NOT:     {{copy_addr|load \[copy\]|load \[take\]|destroy_addr}}
// CHECK:       } // end sil function '$s4main12bumpMoveOnlyyys4CellVySo0cD0VGFTo'
@cxx @implementation
public func bumpMoveOnly(_ m: borrowing Cell<MoveOnly>) {
  m.withUnsafeMutablePointer { $0.pointee.value += 1 }
}

// Pointer slots.

// void reseatPtr(int *_Nullable &p, int *_Nullable to);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}reseatPtr{{.*}}"] [ossa] @$s4main9reseatPtryys4CellVySpys5Int32VGSgG_AHtFTo : $@convention(c) (@in_guaranteed Cell<Optional<UnsafeMutablePointer<Int32>>>, Optional<UnsafeMutablePointer<Int32>>) -> () {
@cxx @implementation
public func reseatPtr(_ p: borrowing Cell<UnsafeMutablePointer<CInt>?>,
                      _ to: UnsafeMutablePointer<CInt>?) {
  p.value = to
}

// void reseatNode(Node *_Nonnull &p, Node *_Nonnull to);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}reseatNode{{.*}}"] [ossa] @$s4main10reseatNodeyys4CellVySo0C0VG_AFtFTo : $@convention(c) (@in_guaranteed Cell<Node>, Node) -> () {
@unsafe @cxx @implementation
public func reseatNode(_ p: borrowing Cell<Node>, _ to: Node) {
  p.value = to
}

// Methods and a namespace member: `self` keeps the C++ convention.

extension Counter {
  // static void reset(int &x);
  // CHECK-LABEL: sil {{.*}}[asmname "{{.*}}reset{{.*}}"] [ossa] @$sSo7CounterV4mainE5resetyys4CellVys5Int32VGFZTo : $@convention(c) (@in_guaranteed Cell<Int32>) -> () {
  @cxx @implementation
  public static func reset(_ x: borrowing Cell<CInt>) {
    x.value = 0
  }

  // void addTo(int &x) const;
  // CHECK-LABEL: sil {{.*}}[asmname "{{.*}}addTo{{.*}}"] [ossa] @$sSo7CounterV4mainE5addToyys4CellVys5Int32VGFTo : $@convention(cxx_method) (@in_guaranteed Cell<Int32>, @in_guaranteed Counter) -> () {
  @unsafe @cxx @implementation
  public func addTo(_ x: borrowing Cell<CInt>) {
    x.value += count
  }

  // void absorb(const int &x);
  // CHECK-LABEL: sil {{.*}}[asmname "{{.*}}absorb{{.*}}"] [ossa] @$sSo7CounterV4mainE6absorbyys9ConstCellVys5Int32VGFTo : $@convention(cxx_method) (@in_guaranteed ConstCell<Int32>, @inout Counter) -> () {
  @unsafe @cxx @implementation
  public mutating func absorb(_ x: borrowing ConstCell<CInt>) {
    count += x.value
  }
}

extension ns {
  // void increment(int &x);
  // CHECK-LABEL: sil {{.*}}[asmname "{{.*}}increment{{.*}}"] [ossa] @$sSo2nsO4mainE9incrementyys4CellVys5Int32VGFZTo : $@convention(c) (@in_guaranteed Cell<Int32>) -> () {
  @cxx @implementation
  public static func increment(_ x: borrowing Cell<CInt>) {
    x.value += 1
  }
}

// bool operator==(const Vector &a, const Vector &b);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}"] [ossa] @$s4main2eeoiySbs9ConstCellVySo6VectorVG_AGtFTo : $@convention(c) (@in_guaranteed ConstCell<Vector>, @in_guaranteed ConstCell<Vector>) -> Bool {
@cxx @implementation
public func == (a: borrowing ConstCell<Vector>, b: borrowing ConstCell<Vector>)
    -> Bool {
  return a.value.x == b.value.x && a.value.y == b.value.y
}

// A lifetimebound `const T &` has the dependence of the imported declaration.

// IntView viewInt(const int &x [[clang::lifetimebound]]);
// CHECK-LABEL: sil {{.*}}[asmname "{{.*}}viewInt{{.*}}"] [ossa] @$s4main7viewIntySo0C4ViewVs9ConstCellVys5Int32VGFTo : $@convention(c) (@in_guaranteed ConstCell<Int32>) -> @lifetime(borrow address 0) @owned IntView {
@cxx @implementation
@_lifetime(borrow x)
public func viewInt(_ x: borrowing ConstCell<CInt>) -> IntView {
  let p = x.withUnsafePointer { $0 }
  return _overrideLifetime(IntView(value: p), borrowing: x)
}
