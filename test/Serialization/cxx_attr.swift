// RUN: %empty-directory(%t)

// Ensure .swift -> .ll
// RUN: %target-swift-frontend \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -emit-ir %s | %FileCheck %s

// Ensure .swift -> .sib -> .ll
// RUN: %target-swift-frontend \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -emit-sib %s -o %t/cxx_attr.sib
// RUN: %target-swift-frontend \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -emit-sil %t/cxx_attr.sib | %FileCheck --check-prefix=SIL %s

// REQUIRES: swift_feature_CxxImplementation

// CHECK: define hidden {{.*}} @foo

// SIL: sil hidden [asmname "foo"]

@cxx
func foo(x: Int32) -> Int32 { return x }
