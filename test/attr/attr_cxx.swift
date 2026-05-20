/// @cxx attribute
/// This test shouldn't require the objc runtime.

// RUN: %target-swift-frontend(mock-sdk: %clang-importer-sdk) -typecheck -verify %s \
// RUN:    -enable-experimental-feature CxxImplementation \
// RUN:    -enable-experimental-cxx-interop \
// RUN:    -disable-objc-interop

@cxx(cxx_foo) func foo(x: Int32) -> Int32 { return x }

@cxx(not an identifier) func invalidName() {}
// expected-error @-1 {{expected ')' in 'cxx' attribute}}
// expected-error @-2 {{expected declaration}}

@cxx("old string syntax") func stringSyntax() {}
// expected-error @-1 {{expected C identifier in 'cxx' attribute}}
// expected-error @-2 {{expected declaration}}

@cxx() func emptyParen() {}
// expected-error @-1 {{expected C identifier in 'cxx' attribute}}
// expected-error @-2 {{expected declaration}}

@cxx func defaultName() {}

// @cxx and @c/@_cdecl pick different foreign languages for the same decl and
// would be resolved inconsistently across compiler phases, so the combination
// is rejected.
@c @cxx func conflictWithC() {}
// expected-error @-1 {{cannot apply both '@cxx' and @c to global function}}

// A @cxx symbol name that collides with a reserved Swift runtime symbol is
// diagnosed (a warning, mirroring @c/@_cdecl).
@cxx(swift_retain) func reservedName() {}
// expected-warning @-1 {{symbol name 'swift_retain' is reserved for the Swift runtime}}

class Foo {
  @cxx(Foo_foo) // expected-error{{@cxx can only be applied to global functions}}
  func foo(x: Int32) -> Int32 { return x }

  @cxx(Foo_foo_2) // expected-error{{@cxx can only be applied to global functions}}
  static func foo(x: Int32) -> Int32 { return x }
}
