import Builtin
import Lifetimes

// The bodies take the address of a borrowed parameter with
// `Builtin.addressOfBorrow`: `withUnsafePointer(to:)` would hand its closure a
// copy, as its own parameter is not addressable.

@unsafe @cxx @implementation
@_lifetime(borrow t)
public func viewTracked(_ t: borrowing Tracked) -> TrackedView {
  let p = UnsafeRawPointer(Builtin.addressOfBorrow(t))
  return _overrideLifetime(
    TrackedView(value: p.assumingMemoryBound(to: CInt.self)), borrowing: t)
}

@unsafe @cxx @implementation
@_lifetime(borrow x)
public func viewInt(_ x: borrowing CInt) -> IntView {
  let p = UnsafePointer<CInt>(Builtin.addressOfBorrow(x))
  return _overrideLifetime(IntView(value: p), borrowing: x)
}

@unsafe @cxx @implementation
@_lifetime(&x)
public func viewMutableInt(_ x: inout CInt) -> IntView {
  let p = withUnsafeMutablePointer(to: &x) { UnsafePointer($0) }
  return _overrideLifetime(IntView(value: p), mutating: &x)
}

extension Owner {
  @cxx @implementation
  public func get() -> UnsafePointer<CInt> {
    return UnsafeRawPointer(Builtin.addressOfBorrow(self))
      .assumingMemoryBound(to: CInt.self)
  }

  @cxx @implementation
  @_lifetime(borrow self)
  public func view() -> IntView {
    let p = UnsafeRawPointer(Builtin.addressOfBorrow(self))
      .assumingMemoryBound(to: CInt.self)
    return _overrideLifetime(IntView(value: p), borrowing: self)
  }

  @unsafe @cxx @implementation
  @_lifetime(borrow x)
  public func viewOf(_ x: borrowing CInt) -> IntView {
    let p = UnsafePointer<CInt>(Builtin.addressOfBorrow(x))
    return _overrideLifetime(IntView(value: p), borrowing: x)
  }
}
