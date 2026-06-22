// Swift implementations of a foreign-reference type's same-arity instance-method
// overloads, selected by parameter type, built into the executable for
// cxx-impl-frt-overload-execution.cpp. `self` is the FRT reference; each impl
// binds to the C++ overload whose parameter type matches its Swift signature
// and is emitted under that overload's mangled symbol.
extension Calc {
  @cxx @implementation
  public func combine(_ x: Int32) -> Int32 { return base + x }

  @cxx @implementation
  public func combine(_ x: Double) -> Double { return Double(base) + x }
}
