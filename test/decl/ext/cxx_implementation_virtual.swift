// Verifies `@cxx @implementation` on C++ *virtual* methods. Phase 2: virtual
// methods of classes with multiple or virtual inheritance are accepted (clang
// emits the `this`-adjusting thunks and VTT). A standalone polymorphic class is
// also fine, as are abstract bases (pure-virtual classes), classes with a
// pure-virtual slot alongside a Swift-implemented key function, and deep
// non-virtual inheritance chains. (Covariant returns remain unsupported --
// rejected by C++ representability, since they return a pointer/reference to a
// C++ class. A receiver class with an *indirect* virtual base -- a virtual base
// reached through a non-virtual base, e.g. the classic diamond -- is rejected
// with a clean diagnostic, because Swift cannot lower its value-type layout.)


// RUN: %target-swift-frontend -typecheck -verify %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_virtual.h

// REQUIRES: swift_feature_CxxImplementation

// Standalone polymorphic class.
extension Animal {
  @cxx @implementation
  func sound() -> Int32 { return id * 10 }
}

// Multiple inheritance: supported. `Derived2::f` overrides a non-const method,
// so the Swift method is `mutating`.
extension Derived2 {
  @cxx @implementation
  mutating func f() {}
}

// Virtual inheritance: supported. `VDerived::g` is const.
extension VDerived {
  @cxx @implementation
  func g() -> Int32 { return 0 }
}

// Abstract base: Swift implements the derived class's key function `Square::area`.
extension Square {
  @cxx @implementation
  func area() -> Int32 { return id * id }
}

// Swift implements the key function `Partial::impl`; `Partial` also has a
// pure-virtual slot (handled by the Swift-emitted vtable).
extension Partial {
  @cxx @implementation
  func impl() -> Int32 { return id * 7 }
}

// Deep non-virtual chain: Swift implements the leaf override `L2::f`.
extension L2 {
  @cxx @implementation
  func f() -> Int32 { return a * 2 }
}

// Indirect virtual base (single path): unsupported -- Swift cannot represent
// the value-type layout, so it is rejected rather than crashing IRGen.
extension IndirectVB {
  @cxx @implementation
  // expected-error@+1 {{'@cxx @implementation' does not support a C++ method of a class with an indirect virtual base}}
  func vf() -> Int32 { return a }
}

// Classic virtual diamond (shared indirect virtual base): also rejected.
extension Diamond {
  @cxx @implementation
  // expected-error@+1 {{'@cxx @implementation' does not support a C++ method of a class with an indirect virtual base}}
  func vf() -> Int32 { return a + d }
}

