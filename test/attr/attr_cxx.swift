/// @cxx attribute
/// This test shouldn't require the objc runtime.

// RUN: %target-swift-frontend(mock-sdk: %clang-importer-sdk) -typecheck -verify %s \
// RUN:    -enable-experimental-feature CxxImplementation \
// RUN:    -enable-experimental-cxx-interop \
// RUN:    -disable-objc-interop

// `@cxx` optionally takes a `name:` argument giving the unqualified C++
// function name to match against (a source name, not a mangled symbol). A bare
// `@cxx` matches/emits under the function's own base identifier.
@cxx func foo(x: Int32) -> Int32 { return x }

@cxx func defaultName() {}

// Explicit C++ function name to match. The Swift function may be named
// differently from the C++ function it implements.
@cxx(name: "cxx_foo") func renamed(x: Int32) -> Int32 { return x }

// Malformed `name:` arguments are rejected.
@cxx(name) func missingValue() {}
// expected-error @-1 {{expected ':' after label 'name'}}

@cxx("cxx_foo") func missingLabel() {}
// expected-error @-1 {{expected 'name:' in 'cxx' attribute}}

@cxx(name: bar) func nonStringValue() {}
// expected-error @-1 {{expected string literal in 'cxx' attribute}}

@cxx() func emptyParens() {}
// expected-error @-1 {{expected 'name:' in 'cxx' attribute}}

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
