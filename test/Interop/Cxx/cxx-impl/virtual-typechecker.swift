// C++ virtual methods implemented in Swift via `@cxx @implementation`: any
// virtual method of a value record or a foreign reference type is accepted --
// the class's key function, an override needing an adjusting thunk under
// multiple or virtual inheritance or with a covariant return type -- except a
// pure virtual method.

// RUN: %target-typecheck-verify-swift \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -disable-availability-checking \
// RUN:   -I %S/Inputs

// REQUIRES: swift_feature_CxxImplementation

import Virtual


// Virtual methods of a record, the key function included: a const one is
// implemented by a non-mutating method, a non-const one by a `mutating`
// method, like their non-virtual siblings.

extension Shape {
  @cxx @implementation
  public func area() -> Int32 { return sides * sides }

  @cxx @implementation
  public mutating func scale(_ factor: Int32) { sides *= factor }
}


// An override along single, non-virtual inheritance, and the base class's
// method it overrides.

extension SimpleBase {
  @cxx @implementation
  public func simple() -> Int32 { return stored }
}

extension SimpleDerived {
  @cxx @implementation
  public func simple() -> Int32 { return stored * 2 }
}


// A pure virtual method is rejected; the key function of its class is not.

// expected-warning@+1{{'Abstract' is deprecated: abstract C++ classes cannot be used as values in Swift}}
extension Abstract {
  @cxx @implementation
  public func anchor() -> Int32 { return 7 }

  // expected-error@+2{{instance method 'pureMethod()' cannot implement C++ function 'pureMethod' because it is a pure virtual method; a pure virtual method's vtable slot dispatches to an overriding method, never to a definition of 'pureMethod' itself}}
  @cxx @implementation
  public func pureMethod() -> Int32 { return 0 }
}


// An override with a covariant return type.

extension CloneDerived {
  @cxx @implementation
  public mutating func clone() -> UnsafeMutablePointer<RetC> {
    return sharedRetC()
  }
}


// Overrides under multiple inheritance, of the primary base's method and of
// the non-primary base's, and the key function.

extension MIDerived {
  @cxx @implementation
  public mutating func miAnchor() {}

  @cxx @implementation
  public mutating func firstA() { a += 100 }

  @cxx @implementation
  public func fromB() -> Int32 { return a + b }
}


// An override of a method of a virtual base, and the key function.

extension VDerived {
  @cxx @implementation
  public mutating func vAnchor() {}

  @cxx @implementation
  public func vbMethod() -> Int32 { return vd }
}


// A virtual method of a foreign reference type resolves through the importer's
// synthesized dispatch thunk to the underlying virtual method; the same rules
// apply to it.

extension Engine {
  @cxx @implementation
  public func status() -> Int32 { return rpm }

  @cxx @implementation
  public func boost(_ amount: Int32) { rpm += amount }
}


// A pure virtual method of a foreign reference type imports (as a dispatch
// thunk), so it reaches the virtual-specific check and is rejected there.

extension AbstractEngine {
  @cxx @implementation
  public func aeAnchor() -> Int32 { return 11 }

  // expected-error@+2{{instance method 'pureStatus()' cannot implement C++ function 'pureStatus' because it is a pure virtual method; a pure virtual method's vtable slot dispatches to an overriding method, never to a definition of 'pureStatus' itself}}
  @cxx @implementation
  public func pureStatus() -> Int32 { return 0 }
}
