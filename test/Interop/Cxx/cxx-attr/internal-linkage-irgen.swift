// A `@cxx @implementation` function provides the body of a C++ function. Its
// only callers are in C++, which the Swift compiler cannot see. If the Swift
// function is not `public`, it has hidden linkage, and at -O the SIL
// optimizer's dead-function elimination would delete it as unused, unless we
// mark it as referenced from a foreign language.
//
// This test ensures that non-public @cxx @implementation function is not
// removed by dead-function elimination.

// RUN: %target-swift-emit-ir \
// RUN:   -I %S/Inputs \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -O \
// RUN:   %s | %FileCheck %s

// REQUIRES: swift_feature_CxxImplementation

import InternalLinkage

// CHECK: define {{.*}}@_Z3fooi
@cxx @implementation
func foo(_ param: Int32) -> Int32 { return param }
