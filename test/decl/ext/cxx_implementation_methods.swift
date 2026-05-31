// Verifies `@cxx @implementation` for C++ *methods* (static and instance)
// declared in an imported C++ record `Vec`, implemented in a Swift extension.
//
// This exercises:
//  * `visitCxxDeclAttr` allowing `@cxx` in an imported C++ record type context
//    (via `importer::isClangCxxRecord`), not only namespaces;
//  * the interface/implementation matcher pairing the Swift method with the
//    imported C++ method (`lookupRelatedFuncs` includes records); and
//  * rejection of a C++ virtual method.

// RUN: %target-swift-frontend -typecheck -verify %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_methods.h

// REQUIRES: swift_feature_CxxImplementation

extension Vec {
  // Static method: scoped free function, no `this`.
  @cxx @implementation
  static func zero() -> Vec { return Vec(x: 0) }

  // Const instance method: `this` is `const Vec *`.
  @cxx @implementation
  func dot(_ o: Vec) -> Int32 { return x * o.x }

  // Non-const instance method: `this` is `Vec *`, implemented as `mutating`.
  @cxx @implementation
  mutating func scale(_ k: Int32) { x = x * k }

  // Overloads are disambiguated by Swift argument labels: the zero-argument
  // `overloaded()` matches `int Vec::overloaded() const` (the `int` overload is
  // a distinct Swift name), so this resolves with no diagnostic.
  @cxx @implementation
  func overloaded() -> Int32 { return x }
}

// A method that is not declared in the header: matching fails.
extension Vec {
  @cxx @implementation
  func notInHeader() -> Int32 {
    // expected-error@-2 {{could not find imported function 'notInHeader' matching instance method 'notInHeader()'; make sure you import the module or header that declares it}}
    return 0
  }
}

// A `mutating` Swift method cannot implement a C++ `const` method (it would
// emit a receiver-mutating body under the `const` mangling).
extension Vec {
  @cxx @implementation
  mutating func probe() -> Int32 {
    // expected-error@-1 {{'mutating' method cannot implement a C++ 'const' method with '@cxx @implementation'; declare the method non-mutating}}
    x += 1
    return x
  }
}

// Conversely, a non-`const` C++ method must be implemented by a `mutating` Swift
// method; a non-mutating one would pass `self` by value, which cannot form the
// C++ `this` pointer ABI. (This used to crash in IRGen.)
extension Vec {
  @cxx @implementation
  func touch() {
    // expected-error@-1 {{a non-'const' C++ method must be implemented by a 'mutating' method with '@cxx @implementation'}}
  }
}

// A C++ virtual method cannot be implemented in Swift.
extension Shape {
  @cxx @implementation
  func area() -> Int32 {
    // expected-error@-1 {{'@cxx @implementation' cannot implement a C++ virtual method}}
    return 0
  }
}
