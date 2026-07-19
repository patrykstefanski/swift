// The Swift functions here are named differently from the C++ functions they
// implement; `@cxx(name:)` names the C++ function to match. The compiler emits
// each body under the matched C++ declaration's Itanium symbol so the C++ caller
// links against it.

// Swift `add` implements C++ `int addImpl(int, int)` -> _Z7addImplii.
@cxx(name: "addImpl") @implementation
public func add(_ a: Int32, _ b: Int32) -> Int32 { return a + b }

// Swift `combine` implements C++ `int Calc::combineImpl(int) const`
// -> _ZNK4Calc11combineImplEi (const instance method, `this` first).
extension Calc {
  @cxx(name: "combineImpl") @implementation
  public func combine(_ k: Int32) -> Int32 { return base + k }
}
