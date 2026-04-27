// Tests that conformance operators found via Clang overload resolution (ADL
// free functions, hidden friends, template instantiations, and synthesized
// forwarding wrappers) can be re-resolved when deserializing a module that
// references them from inlinable function bodies.
//
// RUN: %empty-directory(%t)
// RUN: %target-swiftxx-frontend -emit-module %s -module-name TestADL -I %S/Inputs -o %t/test-part.swiftmodule
// RUN: %target-swiftxx-frontend -merge-modules -emit-module %t/test-part.swiftmodule -module-name TestADL -I %S/Inputs -o %t/TestADL.swiftmodule -sil-verify-none
// RUN: %target-typecheck-verify-swift -I %S/Inputs -cxx-interoperability-mode=default -I %t

import CustomIteratorADL

@inlinable
public func testNamespacedEqualEqual() -> Bool {
    let it = ns1.NamespacedInputIterator(0)
    return it == it
}

@inlinable
public func testHiddenFriendEqualEqual() -> Bool {
    let it = HiddenFriendInputIterator(0)
    return it == it
}

@inlinable
public func testTemplatedMinusPlusEqual() -> Bool {
    var it = ns3.NamespacedTemplatedRACIteratorInt(0)
    it += 1
    return (it - it) == 0
}

@inlinable
public func testInheritedOpsWrappers() -> Bool {
    var it = ns5.InheritedOpsRACIterator(0)
    it += 1
    return (it - it) == 0 && it == it
}
