import Cells

// Aliasing references and storage the body reaches otherwise.

@cxx @implementation
public func observe(_ a: borrowing Cell<CInt>, _ b: borrowing Cell<CInt>)
    -> CInt {
  a.value = 1
  b.value = 2
  return a.value
}

@cxx @implementation
public func readTwice(_ a: borrowing ConstCell<CInt>,
                      _ b: borrowing Cell<CInt>) -> CInt {
  let x = a.value
  b.value = x &+ 99
  return a.value &+ x
}

@cxx @implementation
public func observeGlobal(_ x: borrowing Cell<CInt>) -> CInt {
  global = 5
  x.value = 7
  return global
}

@cxx @implementation
public func readGlobal(_ x: borrowing ConstCell<CInt>) -> CInt {
  let before = x.value
  global = before &+ 1
  return x.value
}

@cxx @implementation
public func readAroundCallback(_ x: borrowing ConstCell<CInt>) -> CInt {
  let before = x.value
  bump()
  return x.value &- before
}

@cxx @implementation
public func writeAroundCallback(_ x: borrowing Cell<CInt>) -> CInt {
  x.value = 10
  bump()
  return x.value
}

// Non-trivial and move-only referents.

@cxx @implementation
public func readTracked(_ t: borrowing ConstCell<Tracked>) -> CInt {
  return t.withUnsafePointer { $0.pointee.value }
}

@cxx @implementation
public func bumpTracked(_ t: borrowing Cell<Tracked>) {
  t.withUnsafeMutablePointer { $0.pointee.value += 1 }
}

@cxx @implementation
public func assignTracked(_ dst: borrowing Cell<Tracked>,
                          _ src: borrowing ConstCell<Tracked>) {
  dst.value = src.value
}

@cxx @implementation
public func readMoveOnly(_ m: borrowing ConstCell<MoveOnly>) -> CInt {
  return m.withUnsafePointer { $0.pointee.value }
}

@cxx @implementation
public func bumpMoveOnly(_ m: borrowing Cell<MoveOnly>) {
  m.withUnsafeMutablePointer { $0.pointee.value += 1 }
}

@cxx @implementation
public func replaceMoveOnly(_ m: borrowing Cell<MoveOnly>, _ v: CInt) {
  m.set(MoveOnly(v))
}

// Pointer slots.

@cxx @implementation
public func reseatPtr(_ p: borrowing Cell<UnsafeMutablePointer<CInt>?>,
                      _ to: UnsafeMutablePointer<CInt>?) {
  p.value = to
}

@cxx @implementation
public func readThroughPtr(_ p: borrowing ConstCell<UnsafeMutablePointer<CInt>>)
    -> CInt {
  return p.value.pointee
}

@unsafe @cxx @implementation
public func reseatNode(_ p: borrowing Cell<Node>, _ to: Node) {
  p.value = to
}

// Methods, a namespace member and operators.

extension Counter {
  @cxx @implementation
  public static func reset(_ x: borrowing Cell<CInt>) {
    x.value = 0
  }

  @unsafe @cxx @implementation
  public func addTo(_ x: borrowing Cell<CInt>) {
    x.value += count
  }

  @unsafe @cxx @implementation
  public mutating func absorb(_ x: borrowing ConstCell<CInt>) {
    count += x.value
  }
}

extension ns {
  @cxx @implementation
  public static func increment(_ x: borrowing Cell<CInt>) {
    x.value += 1
  }
}

extension Vector {
  @unsafe @cxx(`operator+=`) @implementation
  public mutating func add(_ rhs: borrowing ConstCell<Vector>)
      -> UnsafeMutablePointer<Vector> {
    x += rhs.value.x
    y += rhs.value.y
    return withUnsafeMutablePointer(to: &self) { $0 }
  }
}

@cxx @implementation
public func == (a: borrowing ConstCell<Vector>, b: borrowing ConstCell<Vector>)
    -> Bool {
  return a.value.x == b.value.x && a.value.y == b.value.y
}

// Overloads.

@cxx @implementation
public func pick(_ x: borrowing Cell<CInt>) -> CInt { return x.value &* 10 }

@cxx @implementation
public func pick(_ x: UnsafeMutablePointer<CInt>) -> CInt {
  return x.pointee &* 100
}

@cxx @implementation
public func project(_ x: borrowing ConstCell<CInt>) -> CInt {
  return x.value &+ 1
}

// A lifetimebound reference: the view points at the C++ referent.

@cxx @implementation
@_lifetime(borrow x)
public func viewInt(_ x: borrowing ConstCell<CInt>) -> IntView {
  let p = x.withUnsafePointer { $0 }
  return _overrideLifetime(IntView(value: p), borrowing: x)
}
