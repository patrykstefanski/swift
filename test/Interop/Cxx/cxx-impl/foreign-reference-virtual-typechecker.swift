// Overriding virtual methods of foreign reference types implemented in Swift
// via `@cxx @implementation`.

// The rejected Swift override draws a note on the imported declaration.
// RUN: %target-typecheck-verify-swift \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -disable-availability-checking \
// RUN:   -verify-ignore-unrelated \
// RUN:   -I %S/Inputs

// REQUIRES: swift_feature_CxxImplementation

import ForeignReferenceVirtual


// An implementation is not a Swift override.

extension Base {
  @cxx @implementation
  public func describe() -> Int32 { return value }
}

extension Derived {
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() * 2 }
}


// Derived does not override scaled().

extension Leaf {
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() + 1 }

  @cxx @implementation
  public func scaled(_ factor: Int32) -> Int32 { return super.scaled(factor) * 3 }
}


// A pure virtual method has no base implementation.

extension Concrete {
  @cxx @implementation
  public func run() -> Int32 { return 1 }

  public func runBase() -> Int32 {
    return super.run() // expected-error {{cannot use 'super' to call C++ pure virtual method 'run()'; it has no base class implementation}}
  }
}


// The override is the key function.

extension KeyDerived {
  @cxx @implementation
  public func describe() -> Int32 { return super.describe() + 100 }
}


// An override of a non-primary base's method.

extension MI {
  @cxx @implementation
  public func side() -> Int32 { return value * 10 }

  @cxx @implementation
  public func describe() -> Int32 { return super.describe() * 3 }
}


// Rejections

extension Rejections {
  // expected-error@+2{{'override' cannot be applied to instance method marked '@cxx'; whether it overrides is declared in C++}}
  @cxx @implementation
  public override func describe() -> Int32 { return 0 }

  // A Swift override that is not an implementation.
  // expected-error@+2{{overriding non-open instance method outside of its defining module}}
  // expected-error@+1{{declared in 'Base' cannot be overridden from extension}}
  public func scaled(_ factor: Int32) -> Int32 { return 0 }
}
