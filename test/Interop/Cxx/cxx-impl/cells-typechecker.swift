// C++ reference parameters spelled with the standard library's `Cell` and
// `ConstCell`.

// RUN: %target-typecheck-verify-swift \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-feature Cells \
// RUN:   -enable-experimental-feature Lifetimes \
// RUN:   -target %target-swift-6.5-abi-triple \
// RUN:   -verify-additional-file %S%{fs-sep}Inputs%{fs-sep}cells.h \
// RUN:   -I %S%{fs-sep}Inputs

// REQUIRES: swift_feature_CxxImplementation
// REQUIRES: swift_feature_Cells
// REQUIRES: swift_feature_Lifetimes

import Cells


// `T &` is implemented by a `borrowing Cell<T>`, `const T &` by a
// `borrowing ConstCell<T>`, without `@unsafe`.

@cxx @implementation
func observe(_ a: borrowing Cell<CInt>, _ b: borrowing Cell<CInt>) -> CInt {
  a.value = 1
  b.value = 2
  return a.value
}

@cxx @implementation
func readTwice(_ a: borrowing ConstCell<CInt>, _ b: borrowing Cell<CInt>)
    -> CInt {
  let x = a.value
  b.value = x &+ 99
  return a.value &+ x
}

@cxx @implementation
func observeGlobal(_ x: borrowing Cell<CInt>) -> CInt {
  global = 5
  x.value = 7
  return global
}

@cxx @implementation
func readAroundCallback(_ x: borrowing ConstCell<CInt>) -> CInt {
  let before = x.value
  bump()
  return x.value &- before
}


// Non-trivial and move-only referents are reached in place.

@cxx @implementation
func readTracked(_ t: borrowing ConstCell<Tracked>) -> CInt {
  return t.withUnsafePointer { $0.pointee.value }
}

@cxx @implementation
func assignTracked(_ dst: borrowing Cell<Tracked>,
                   _ src: borrowing ConstCell<Tracked>) {
  dst.value = src.value
}

@cxx @implementation
func bumpMoveOnly(_ m: borrowing Cell<MoveOnly>) {
  m.withUnsafeMutablePointer { $0.pointee.value += 1 }
}

@cxx @implementation
func replaceMoveOnly(_ m: borrowing Cell<MoveOnly>, _ v: CInt) {
  m.set(MoveOnly(v))
}


// Pointer slots. A parameter of foreign reference type requires `@unsafe`.

@cxx @implementation
func reseatPtr(_ p: borrowing Cell<UnsafeMutablePointer<CInt>?>,
               _ to: UnsafeMutablePointer<CInt>?) {
  p.value = to
}

@cxx @implementation
func readThroughPtr(_ p: borrowing ConstCell<UnsafeMutablePointer<CInt>>)
    -> CInt {
  return p.value.pointee
}

@unsafe @cxx @implementation
func reseatNode(_ p: borrowing Cell<Node>, _ to: Node) {
  p.value = to
}


// A static method and a namespace member need no `@unsafe`; an instance
// method and a member operator do.

extension Counter {
  @cxx @implementation
  static func reset(_ x: borrowing Cell<CInt>) {
    x.value = 0
  }

  @unsafe @cxx @implementation
  func addTo(_ x: borrowing Cell<CInt>) {
    x.value += count
  }

  @unsafe @cxx @implementation
  mutating func absorb(_ x: borrowing ConstCell<CInt>) {
    count += x.value
  }
}

extension ns {
  @cxx @implementation
  static func increment(_ x: borrowing Cell<CInt>) {
    x.value += 1
  }
}

extension Vector {
  @unsafe @cxx(`operator+=`) @implementation
  mutating func add(_ rhs: borrowing ConstCell<Vector>)
      -> UnsafeMutablePointer<Vector> {
    x += rhs.value.x
    y += rhs.value.y
    return withUnsafeMutablePointer(to: &self) { $0 }
  }
}

@cxx @implementation
func == (a: borrowing ConstCell<Vector>, b: borrowing ConstCell<Vector>)
    -> Bool {
  return a.value.x == b.value.x && a.value.y == b.value.y
}


// A `Cell` or a `ConstCell` picks the reference among overloads.

// expected-note@+2{{previously implemented here}}
@cxx @implementation
func pick(_ x: borrowing Cell<CInt>) -> CInt { return x.value }

@cxx @implementation
func pick(_ x: UnsafeMutablePointer<CInt>) -> CInt { return x.pointee }

// The projection spells the same overload as the `Cell`.
// expected-error@+1{{duplicate implementation of imported global function 'pick'}}
@unsafe @cxx(pick) @implementation
func pickAgain(_ x: inout CInt) -> CInt { return x }

@cxx @implementation
func project(_ x: borrowing ConstCell<CInt>) -> CInt { return x.value }

// A by-value `CInt` matches both overloads, as without the `ConstCell`.
// expected-error@+1{{global function 'project' could implement any of several imported overloads of 'project' that have the same signature in Swift}}
@cxx @implementation
func project(_ x: CInt) -> CInt { return x }


// A lifetimebound `const T &` spelled `borrowing ConstCell<T>`: the
// dependence is the header's, and the view points at the C++ referent.

@cxx @implementation
@_lifetime(borrow x)
func viewInt(_ x: borrowing ConstCell<CInt>) -> IntView {
  let p = x.withUnsafePointer { $0 }
  return _overrideLifetime(IntView(value: p), borrowing: x)
}


// Rejected: a `Cell` or `ConstCell` that is not `borrowing`.

// expected-error@+2{{global function 'consumingCell' cannot implement C++ function 'consumingCell' because parameter 'x' must be 'borrowing' to match a C++ reference}}{{25-34=borrowing}}
@cxx @implementation
func consumingCell(_ x: consuming Cell<CInt>) {}

// expected-error@+2{{global function 'inoutCell' cannot implement C++ function 'inoutCell' because parameter 'x' must be 'borrowing' to match a C++ reference}}{{21-26=borrowing}}
@cxx @implementation
func inoutCell(_ x: inout Cell<CInt>) {}

// expected-error@+2{{global function 'consumingConstCell' cannot implement C++ function 'consumingConstCell' because parameter 'x' must be 'borrowing' to match a C++ 'const' reference}}{{30-39=borrowing}}
@cxx @implementation
func consumingConstCell(_ x: consuming ConstCell<CInt>) {}


// Rejected: a `Cell` or `ConstCell` that does not spell the parameter.

// expected-error@+2{{global function 'cellForConstRef' of type '(borrowing Cell<CInt>) -> ()' (aka '(borrowing Cell<Int32>) -> ()') does not match type '(CInt) -> Void' (aka '(Int32) -> ()') declared by the header}}
@cxx @implementation
func cellForConstRef(_ x: borrowing Cell<CInt>) {}

// expected-error@+2{{global function 'constCellForRef' of type '(borrowing ConstCell<CInt>) -> ()' (aka '(borrowing ConstCell<Int32>) -> ()') does not match type '(inout CInt) -> Void' (aka '(inout Int32) -> ()') declared by the header}}
@cxx @implementation
func constCellForRef(_ x: borrowing ConstCell<CInt>) {}

// expected-error@+2{{global function 'constCellForValue' of type '(borrowing ConstCell<CInt>) -> ()' (aka '(borrowing ConstCell<Int32>) -> ()') does not match type '(CInt) -> Void' (aka '(Int32) -> ()') declared by the header}}
@cxx @implementation
func constCellForValue(_ x: borrowing ConstCell<CInt>) {}

// expected-error@+2{{global function 'cellForPointer' of type '(borrowing Cell<CInt>) -> ()' (aka '(borrowing Cell<Int32>) -> ()') does not match type '(UnsafeMutablePointer<CInt>) -> Void' (aka '(UnsafeMutablePointer<Int32>) -> ()') declared by the header}}
@cxx @implementation
func cellForPointer(_ x: borrowing Cell<CInt>) {}

// expected-error@+2{{global function 'cellWrongReferent' of type '(borrowing Cell<CLongLong>) -> ()' (aka '(borrowing Cell<Int64>) -> ()') does not match type '(inout CInt) -> Void' (aka '(inout Int32) -> ()') declared by the header}}
@cxx @implementation
func cellWrongReferent(_ x: borrowing Cell<CLongLong>) {}

// A reference to an object of foreign reference type imports as the object.
// expected-error@+2{{global function 'cellForNodeRef' of type '(borrowing Cell<Node>) -> ()' does not match type '(Node) -> Void' declared by the header}}
@cxx @implementation
func cellForNodeRef(_ n: borrowing Cell<Node>) {}

// expected-error@+2{{global function 'constCellForNodeRef' of type '(borrowing ConstCell<Node>) -> ()' does not match type '(Node) -> Void' declared by the header}}
@cxx @implementation
func constCellForNodeRef(_ n: borrowing ConstCell<Node>) {}


// Rejected: implementations that are not marked `@unsafe`.

// expected-error@+2{{global function 'mixedSpellings' must be marked '@unsafe' to implement C++ function 'mixedSpellings'}}{{-1:1-1=@unsafe }}
@cxx @implementation
func mixedSpellings(
  _ a: borrowing Cell<CInt>,
  _ b: inout CInt // expected-note{{C++ callers may violate exclusive access to 'inout' parameter 'b'}}
) -> CInt { return 0 }

// expected-error@+2{{global function 'takesNode' must be marked '@unsafe' to implement C++ function 'takesNode'}}{{-1:1-1=@unsafe }}
@cxx @implementation
func takesNode(
  _ x: borrowing Cell<CInt>, // expected-note{{C++ callers may pass a reference into an object of foreign reference type as parameter 'x'}}
  _ n: Node
) -> CInt { return 0 }

// expected-error@+2{{global function 'takesNodeSlot' must be marked '@unsafe' to implement C++ function 'takesNodeSlot'}}{{-1:1-1=@unsafe }}
@cxx @implementation
func takesNodeSlot(
  _ slot: borrowing Cell<Node>, // expected-note{{C++ callers may pass a reference into an object of foreign reference type as parameter 'slot'}}
  _ x: borrowing Cell<CInt> // expected-note{{C++ callers may pass a reference into an object of foreign reference type as parameter 'x'}}
) {}

extension Rejections {
  // expected-error@+2{{instance method 'addTo' must be marked '@unsafe' to implement C++ function 'addTo'}}{{-1:3-3=@unsafe }}
  @cxx @implementation
  func addTo(
    _ x: borrowing Cell<CInt> // expected-note{{C++ callers may pass a reference into 'self' as parameter 'x'}}
  ) {}

  // expected-error@+2{{instance method 'absorb' must be marked '@unsafe' to implement C++ function 'absorb'}}{{-1:3-3=@unsafe }}
  @cxx @implementation
  mutating func absorb(
    _ x: borrowing ConstCell<CInt> // expected-note{{C++ callers may pass a reference into 'self' as parameter 'x'}}
  ) {}

  // expected-error@+2{{instance method 'add' must be marked '@unsafe' to implement C++ function 'operator+='}}{{-1:3-3=@unsafe }}
  @cxx(`operator+=`) @implementation
  mutating func add(
    _ rhs: borrowing ConstCell<Rejections> // expected-note{{C++ callers may pass a reference into 'self' as parameter 'rhs'}}
  ) -> UnsafeMutablePointer<Rejections> {
    return withUnsafeMutablePointer(to: &self) { $0 }
  }
}
