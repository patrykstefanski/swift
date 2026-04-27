// RUN: %target-swift-ide-test -print-module -module-to-print=CustomIteratorADL -source-filename=x -I %S/Inputs -cxx-interoperability-mode=default | %FileCheck %s

// Case 1: Namespaced input iterator with non-member operator== found via ADL.
// CHECK: struct NamespacedInputIterator : UnsafeCxxInputIterator {
// CHECK:   func successor() -> ns1.NamespacedInputIterator
// CHECK:   typealias Pointee = CInt
// CHECK:   var pointee: CInt { get }
// CHECK: }

// Case 2: Hidden friend operator== found via ADL.
// CHECK: struct HiddenFriendInputIterator : UnsafeCxxInputIterator {
// CHECK:   func successor() -> HiddenFriendInputIterator
// CHECK:   typealias Pointee = CInt
// CHECK:   var pointee: CInt { get }
// CHECK: }

// Case 3: Namespaced template RAC iterator — == and - via ADL, += as member.
// CHECK: struct NamespacedTemplatedRACIterator<CInt> : UnsafeCxxRandomAccessIterator, UnsafeCxxInputIterator {
// CHECK:   func successor() -> ns3.NamespacedTemplatedRACIterator<CInt>
// CHECK:   typealias Pointee = CInt
// CHECK:   typealias Distance = ns3.NamespacedTemplatedRACIterator<CInt>.difference_type
// CHECK:   var pointee: CInt { get }
// CHECK: }

// Case 4: operator== in a different namespace — should NOT get conformance.
// CHECK: struct UnfindableEqIterator {
// CHECK-NOT: UnsafeCxxInputIterator
// CHECK:   func successor() -> ns4_types.UnfindableEqIterator
// CHECK: }

// Case 5: RAC iterator inheriting ==, -, and += from a base class; forwarding
// wrappers with the derived type are synthesized around the base operators.
// CHECK: struct InheritedOpsRACIterator : UnsafeCxxRandomAccessIterator, UnsafeCxxInputIterator {
// CHECK:   func successor() -> ns5.InheritedOpsRACIterator
// CHECK:   typealias Pointee = CInt
// CHECK:   typealias Distance = CInt
// CHECK:   var pointee: CInt { get }
// CHECK: }
