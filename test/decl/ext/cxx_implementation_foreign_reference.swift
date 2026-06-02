// Verifies that `@cxx @implementation` on an *instance* method of a C++
// foreign-reference type (`SWIFT_SHARED_REFERENCE`) is rejected with a clean
// diagnostic rather than crashing IRGen or reporting a confusing
// "could not find imported function".
//
// Such a method is not yet supported. The imported `self` is a class reference
// (passed directly), but the C++-method entry-point lowering expects an
// indirect `this`, so a *non-virtual* one would crash IRGen
// (`emitEntryPointArgumentsCOrObjC`); a *virtual* one additionally fails to
// match, because for a reference type the importer surfaces a synthesized
// dynamic-dispatch thunk (`__synthesizedVirtualCall_<name>`) under the method's
// name. A method of a value-type C++ record is unaffected.

// RUN: %target-swift-frontend -typecheck -verify %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -disable-availability-checking \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_foreign_reference.h

// REQUIRES: swift_feature_CxxImplementation

extension Widget {
  // Virtual instance method of a foreign-reference type: rejected.
  @cxx @implementation // expected-error {{'@cxx @implementation' does not support implementing a C++ instance method of a foreign-reference type}}
  func describe() -> Int32 { return id }

  // Non-virtual instance method of a foreign-reference type: also rejected (it
  // would otherwise crash IRGen).
  @cxx @implementation // expected-error {{'@cxx @implementation' does not support implementing a C++ instance method of a foreign-reference type}}
  func tag() -> Int32 { return id }
}

// Contrast: an instance method of a value-type C++ record still works.
extension Value {
  @cxx @implementation
  func get() -> Int32 { return x }
}
