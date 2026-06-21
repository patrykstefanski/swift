// Each Swift function binds to the overload whose parameter type matches and is
// emitted under that overload's mangled symbol (_Z4picki / _Z4pickd).
@cxx @implementation
public func pick(_ x: Int32) -> Int32 { return x + 1 }

@cxx @implementation
public func pick(_ x: Double) -> Double { return x + 0.5 }
