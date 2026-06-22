// Swift implementations of non-virtual instance methods of a foreign-reference
// type, built into the executable for cxx-impl-frt-method-execution.cpp. `self`
// is the FRT reference; `add` (a non-const C++ method) is a non-mutating Swift
// method that mutates through the reference.
extension Box {
  @cxx @implementation
  public func get() -> Int32 { return value }

  @cxx @implementation
  public func add(_ n: Int32) { value += n }
}
