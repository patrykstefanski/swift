/// @cxx attribute
/// This test shouldn't require the objc runtime.

// RUN: %target-swift-frontend(mock-sdk: %clang-importer-sdk) -typecheck -verify %s \
// RUN:    -enable-experimental-feature CxxImplementation \
// RUN:    -enable-experimental-cxx-interop \
// RUN:    -disable-objc-interop

// `@cxx` takes no argument: the emitted C++ symbol is derived from the function
// itself (its base identifier for a bare `@cxx`, or the matched C++
// declaration's mangled name for a `@cxx @implementation`). Unlike `@c`, there
// is no explicit-name form.
@cxx func foo(x: Int32) -> Int32 { return x }

@cxx func defaultName() {}

// The parenthesized name form `@cxx(name)` is not valid syntax.
@cxx(name) func hasArgument() {}
// expected-error @-1 {{expected declaration}}

// @cxx and @c/@_cdecl pick different foreign languages for the same decl and
// would be resolved inconsistently across compiler phases, so the combination
// is rejected.
@c @cxx func conflictWithC() {}
// expected-error @-1 {{cannot apply both '@cxx' and @c to global function}}

// A @cxx function whose name collides with a reserved Swift runtime symbol is
// diagnosed (a warning, mirroring @c/@_cdecl).
@cxx func swift_retain() {}
// expected-warning @-1 {{symbol name 'swift_retain' is reserved for the Swift runtime}}

class Foo {
  @cxx // expected-error{{@cxx can only be applied to global functions}}
  func foo(x: Int32) -> Int32 { return x }

  @cxx // expected-error{{@cxx can only be applied to global functions}}
  static func bar(x: Int32) -> Int32 { return x }
}
