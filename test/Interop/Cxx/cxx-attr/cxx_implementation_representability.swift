// Verifies that `@cxx` actually type-checks a function's signature for C++
// representability.
//
// Before the fix, `TypeCheckCDeclFunctionRequest` (which runs the
// representability check and rejects throws/async) was only dispatched for
// `CDeclAttr`, never for `CxxDeclAttr` — so `@cxx` signatures were accepted
// unchecked. The dispatch in `TypeCheckDeclPrimary` now also fires for `@cxx`,
// and C++ representability is defined to mirror C (NOT Objective-C): Objective-C
// bridging (String <-> NSString, ...) is stripped, and non-trivial C++ types
// are out of scope.
//
// Note: Objective-C interop is intentionally left ENABLED so that `String`
// conforms to `_ObjectiveCBridgeable`; the point is that C++ still rejects it.

// RUN: %target-typecheck-verify-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-cxx-interop \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_representability.h

// REQUIRES: swift_feature_CxxImplementation

// --- Representable signatures (no diagnostics expected) ---

// Trivial C scalar type.
@cxx func okPrimitive(_ x: Int32) -> Int32 { return x }

// Trivial imported C++ struct, by value.
@cxx func okStruct(_ s: TrivialStruct) -> Int32 { return s.x }

// --- Non-representable signatures (must be diagnosed) ---

// `String` is only representable via Objective-C bridging, which does not exist
// in C++. Must be rejected.
@cxx func takesString(_ s: String) {}
// expected-error @-1 {{cannot be represented in C++}}
// expected-note @-2 {{Swift structs cannot be represented in C++}}

// A non-trivial C++ class is out of scope for @cxx.
@cxx func takesNonTrivial(_ obj: NonTrivialClass) {}
// expected-error @-1 {{cannot be represented in C++}}
// expected-note @-2 {{non-trivial C++ classes cannot be represented in C++}}

// `async` has no representation in a plain C++ function.
@cxx func asyncFn() async {}
// expected-error @-1 {{cannot be asynchronous}}

// Swift error handling has no representation in a plain C++ function.
@cxx func throwingFn() throws {}
// expected-error @-1 {{raising errors from}}
