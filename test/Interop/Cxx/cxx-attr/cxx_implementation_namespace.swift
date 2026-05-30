// Verifies `@cxx @implementation` for a function declared in an imported C++
// *namespace*. The namespace imports into Swift as an enum (`mathz`) and the
// function is implemented in a Swift extension of that enum.
//
// This exercises two fixes working together:
//  * `visitCxxDeclAttr` allows `@cxx` in a clang-namespace type context (it used
//    to reject every type context with "@cxx can only be applied to global
//    functions"); and
//  * `lookupRelatedFuncs` (in ClangImporter) includes the decl being matched in
//    the candidate set, so the interface/implementation matcher can pair the
//    Swift implementation with the imported C++ declaration. This is needed
//    because `lookupQualified` on a namespace enum returns only the namespace's
//    Clang members, not the members added by Swift extensions.

// RUN: %target-swift-frontend -typecheck -verify %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_namespace.h
//
// RUN: %target-swift-frontend -emit-ir %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_namespace.h \
// RUN:   | %FileCheck %s

// REQUIRES: swift_feature_CxxImplementation

// Implements the namespaced C++ function `mathz::nsAdd`. No diagnostics: neither
// the old "@cxx can only be applied to global functions" error (attribute
// checker fix) nor "could not find imported function" (matcher fix).
extension mathz {
  @cxx @implementation
  static func nsAdd(_ a: Int32, _ b: Int32) -> Int32 { return a + b }
}

// The body must be emitted under the Itanium-mangled name of
// `mathz::nsAdd(int, int)` so the C++ caller links against it.
// CHECK: define {{.*}}@_ZN5mathz5nsAddEii
