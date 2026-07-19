// Verifies the attribute-checker half of C++ namespace support for `@cxx`.
//
// A C++ namespace imports into Swift as an enum, and an `extension` of that
// enum is technically a "type context". The attribute checker used to reject
// ALL type contexts with "@cxx can only be applied to global functions", which
// made the documented namespace case (see docs/CxxImplementationDesign.md)
// unreachable even in principle. `visitCxxDeclAttr` now allows type contexts
// that are clang namespaces (via `importer::isClangNamespace`), while still
// rejecting genuine types (classes/structs) — see test/attr/attr_cxx.swift for
// the rejected case.
//
// IMPORTANT (known limitation): the *matching* machinery
// (`findFunctionInterfaceAndImplementation` / `lookupRelatedFuncs`) does not yet
// resolve a function declared inside an imported C++ namespace, so the
// end-to-end `@cxx @implementation` for a namespaced function currently fails
// with "could not find imported function". The point of THIS test is to lock in
// that the attribute is no longer rejected for the wrong reason ("global
// functions only"); wiring up namespace matching is follow-up work. When that
// lands, the negative expectation below should be removed.

// RUN: %target-typecheck-verify-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_namespace.h

// REQUIRES: swift_feature_CxxImplementation

extension mathz {
  // No "@cxx can only be applied to global functions" error here (that is the
  // attribute-checker fix). The remaining matching-layer gap surfaces as the
  // "could not find imported function" diagnostic below.
  @cxx @implementation
  static func nsAdd(_ a: Int32, _ b: Int32) -> Int32 { return a + b }
  // expected-error @-2 {{could not find imported function 'nsAdd' matching static method 'nsAdd'}}
}

