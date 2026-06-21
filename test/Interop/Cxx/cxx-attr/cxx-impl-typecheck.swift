// RUN: %target-typecheck-verify-swift \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-stable-abi-triple \
// RUN:   -import-bridging-header %S/Inputs/cxx-impl.h \
// RUN:   -disable-objc-interop

@cxx @implementation
func CxxImplFunc1(_: Int32) -> Int32 {
  return 0
}

@implementation(BadCategory) @cxx
func CxxImplFunc_BadCategory(_: Int32) -> Int32 {
  // expected-error@-2 {{global function 'CxxImplFunc_BadCategory' does not belong to an Objective-C category; remove the category name from this attribute}} {{16-29=}}
  // expected-error@-3 {{could not find imported function 'CxxImplFunc_BadCategory' matching global function 'CxxImplFunc_BadCategory'; make sure you import the module or header that declares it}}
  return 0
}

@cxx @implementation
func CxxImplFuncMissing(_: Int32) -> Int32 {
  // expected-error@-2 {{could not find imported function 'CxxImplFuncMissing' matching global function 'CxxImplFuncMissing'; make sure you import the module or header that declares it}}
  return 0
}

@cxx @implementation
func CxxImplFuncMismatch1(_: Float) -> Int32 {
  // expected-error@-1 {{global function 'CxxImplFuncMismatch1' of type '(Float) -> Int32' does not match type '(Int32) -> Int32' declared by the header}}
  return 0
}

@cxx @implementation
func CxxImplFuncMismatch2(_: Int32) -> Float {
  // expected-error@-1 {{global function 'CxxImplFuncMismatch2' of type '(Int32) -> Float' does not match type '(Int32) -> Int32' declared by the header}}
  return 0
}

// Same-arity C++ overloads are disambiguated by the Swift signature's parameter
// types: each implementation binds to the overload whose parameters match.
@cxx @implementation
func sameArityOverload(_: Int32) -> Int32 { return 0 }    // -> int sameArityOverload(int)

@cxx @implementation
func sameArityOverload(_: Double) -> Double { return 0 }  // -> double sameArityOverload(double)

// `@cxx(name:)` lets the Swift function be named differently from the C++
// function it implements; the importer looks up the supplied C++ name. Matches
// `int CxxImplFuncNameMismatch1(int)`.
@cxx(name: "CxxImplFuncNameMismatch1") @implementation
func swiftSideName1(_: Int32) -> Int32 {
  return 0
}

// The interface is found via the explicit name, so a signature mismatch is
// still diagnosed against the *matched* C++ declaration.
@cxx(name: "CxxImplFuncNameMismatch2") @implementation
func swiftSideName2(_: Float) -> Int32 {
  // expected-error@-1 {{global function 'swiftSideName2' of type '(Float) -> Int32' does not match type '(Int32) -> Int32' declared by the header}}
  return 0
}

// An explicit name that matches no imported C++ function fails, and the
// diagnostic names the searched-for C++ name (not the Swift name).
@cxx(name: "DoesNotExist") @implementation
func swiftSideName3(_: Int32) -> Int32 {
  // expected-error@-2 {{could not find imported function 'DoesNotExist' matching global function 'swiftSideName3'; make sure you import the module or header that declares it}}
  return 0
}
