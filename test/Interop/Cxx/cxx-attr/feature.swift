// Test that @cxx requires both C++ interop and CxxImplementation feature

// @cxx with both C++ interop and CxxImplementation feature should work
// RUN: %target-swift-frontend \
// RUN:   -typecheck %s \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation

// @cxx without C++ interop should fail
// RUN: not %target-swift-frontend \
// RUN:   -typecheck %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   2>&1 | %FileCheck --check-prefix=NO-CXX-INTEROP %s

// @cxx without CxxImplementation feature should fail
// RUN: not %target-swift-frontend \
// RUN:   -typecheck %s \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   2>&1 | %FileCheck --check-prefix=NO-FEATURE %s

@cxx
func foo() {}

// NO-CXX-INTEROP: error: 'cxx' requires C++ interoperability
// NO-FEATURE: error: 'cxx' attribute is only valid when experimental feature CxxImplementation is enabled
