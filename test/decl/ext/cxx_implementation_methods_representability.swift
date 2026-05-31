// Verifies C++ representability for `@cxx @implementation` *methods*. The
// explicit-parameter/result check is the same one used for free functions
// (covered by cxx_implementation_representability.swift); what is specific to
// methods is that an instance method's *receiver* (self) may be a non-trivial
// C++ record, because `self`/`this` is always passed by pointer and never copied
// at the ABI boundary.

// RUN: %target-typecheck-verify-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_methods_representability.h

// REQUIRES: swift_feature_CxxImplementation

extension Host {
  // Trivial parameter and result: representable, no diagnostics.
  @cxx @implementation
  func okTrivial(_ p: TrivialPair) -> Int32 { return p.a }
}

extension NonTrivialReceiver {
  // The receiver is a non-trivial C++ class, but because `self`/`this` is passed
  // by pointer (never copied at the boundary), implementing its method is
  // allowed — no diagnostic, unlike a non-trivial *parameter* or *result*.
  @cxx @implementation
  func read() -> Int32 { return value }
}
