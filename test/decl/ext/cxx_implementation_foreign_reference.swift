// `@cxx @implementation` on an instance method of a C++ foreign-reference type
// (`SWIFT_SHARED_REFERENCE`, imported as a Swift class). Both non-virtual and
// **virtual** methods are supported: the imported `self` is a direct class
// reference whose value *is* the `this` pointer, and for a virtual method the
// importer's synthesized dynamic-dispatch thunk (`__synthesizedVirtualCall_<name>`)
// is resolved to the real method (whose vtable Swift emits when it is the key
// function). A method that matches no imported C++ instance method is diagnosed.

// RUN: %target-swift-frontend -typecheck -verify %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -disable-availability-checking \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_foreign_reference.h

// REQUIRES: swift_feature_CxxImplementation

extension Widget {
  // Non-virtual const instance method: supported.
  @cxx @implementation
  func tag() -> Int32 { return id }

  // Non-virtual non-const instance method: supported (mutates through the
  // reference; the const/mutating rule is relaxed for FRT receivers).
  @cxx @implementation
  func bump(_ by: Int32) { id += by }

  // Virtual instance method: supported (the dynamic-dispatch thunk is resolved
  // to the real method, whose vtable Swift emits).
  @cxx @implementation
  func describe() -> Int32 { return id }

  // A method matching no imported C++ instance method is diagnosed.
  @cxx @implementation // expected-error {{'@cxx @implementation' could not match this method to an instance method of the foreign-reference type (a 'SWIFT_SHARED_REFERENCE' class)}}
  func notInHeader() -> Int32 { return id }

  // A const/non-const C++ overload pair on an FRT cannot be disambiguated. An
  // FRT imports as a Swift class, which can't have a `mutating` method, so only
  // one of the pair is imported and a single impl would silently bind it (there
  // is no `mutating` tiebreaker as there is for value-type records). The `@cxx`
  // checker detects the const/non-const sibling and diagnoses it as ambiguous.
  @cxx(name: "g") @implementation // expected-error {{instance method 'gImpl' matches multiple imported C++ overloads with the same Swift signature, so '@cxx @implementation' cannot determine which one it implements}}
  func gImpl(_ x: Int32) -> Int32 { return id + x }
}

// Contrast: an instance method of a value-type C++ record still works.
extension Value {
  @cxx @implementation
  func get() -> Int32 { return x }
}
