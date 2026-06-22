// `@cxx @implementation` on an instance method of a C++ foreign-reference type
// (`SWIFT_SHARED_REFERENCE`, imported as a Swift class). **Non-virtual** methods
// are supported (phase 2): the imported `self` is a direct class reference whose
// value *is* the `this` pointer, bound directly in the C++-method entry-point
// lowering. A **virtual** method is still rejected, because the importer surfaces
// a synthesized dynamic-dispatch thunk (`__synthesizedVirtualCall_<name>`) under
// the method name, so the matcher finds no interface.

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

  // Non-virtual non-const instance method: supported. An FRT is a class, so the
  // Swift method is non-mutating (the const/mutating rule is relaxed for FRT
  // receivers) and mutates through the reference.
  @cxx @implementation
  func bump(_ by: Int32) { id += by }

  // Virtual instance method: not yet supported.
  @cxx @implementation // expected-error {{'@cxx @implementation' supports only non-virtual instance methods of a foreign-reference type (a 'SWIFT_SHARED_REFERENCE' class); a virtual method is dispatched through a thunk that is not yet matched}}
  func describe() -> Int32 { return id }
}

// Contrast: an instance method of a value-type C++ record still works.
extension Value {
  @cxx @implementation
  func get() -> Int32 { return x }
}
