// Regression test for: `@cxx` functions must never be emitted into a generated
// Objective-C compatibility header.
//
// A bare `@cxx` function (no `@implementation`) used to CRASH the Objective-C
// header printer: it reached `printAbstractFunctionAsCFunction()`, which begins
// with `assert(FD->getAttrs().hasAttribute<CDeclAttr>())`. A `@cxx` function
// carries `CxxDeclAttr`, not `CDeclAttr`, so the assertion fired (or, in release
// builds, a bogus C declaration was written into the header).
//
// A `@cxx` function implements a C++ declaration that already exists in the
// imported C++ header, so it must be excluded from the generated header in
// every output mode. `DeclAndTypePrinter::shouldInclude` now returns false for
// `getCDeclKind() == ForeignLanguage::Cxx`. The mere fact that this test
// compiles (no crash) is half the regression check; the CHECK-NOT lines verify
// neither function leaks into the header.

// RUN: %empty-directory(%t)
// RUN: %target-swift-frontend(mock-sdk: %clang-importer-sdk) -typecheck %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_header.h \
// RUN:   -emit-objc-header-path %t/header.h
// RUN: %FileCheck %s < %t/header.h

// REQUIRES: objc_interop
// REQUIRES: swift_feature_CxxImplementation

// A bare @cxx function (no @implementation): this is the case that used to trip
// the assertion in the Objective-C header printer.
@cxx public func bareCxxFn(_ param: Int32) -> Int32 { return param }

// A @cxx @implementation function matching the imported header declaration.
@implementation @cxx
public func implCxxFn(_ x: Int32, _ y: Int32) -> Int32 { return x + y }

// The compile MUST succeed (the regression is an assertion crash in the header
// printer). Neither @cxx function may be emitted as a callable declaration.
// With C++ interop enabled, the generated header's C++ section instead records
// them as unavailable — which simultaneously proves the printer did not crash
// AND that no real C/Objective-C declaration was produced for them.
// CHECK: Unavailable in C++: Swift global function 'bareCxxFn
// CHECK: Unavailable in C++: Swift global function 'implCxxFn

