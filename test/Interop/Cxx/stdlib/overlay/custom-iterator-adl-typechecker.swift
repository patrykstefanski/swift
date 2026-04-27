// Tests that iterator types whose operators are found via Clang ADL (not
// member lookup or module-wide scan) get the correct protocol conformances.
//
// RUN: %target-typecheck-verify-swift -verify-ignore-unrelated -suppress-notes -I %S/Inputs -cxx-interoperability-mode=default

import CustomIteratorADL

func checkInput<It: UnsafeCxxInputIterator>(_ _: It) {}
func checkRandomAccess<It: UnsafeCxxRandomAccessIterator>(_ _: It) {}

// Case 1: Non-member operator== in same namespace, found via ADL.
func check(it: ns1.NamespacedInputIterator) {
  checkInput(it)
  checkRandomAccess(it) // expected-error {{requires}}
}

// Case 2: Hidden friend operator==, found via ADL.
func check(it: HiddenFriendInputIterator) {
  checkInput(it)
  checkRandomAccess(it) // expected-error {{requires}}
}

// Case 3: Full RAC iterator with template == and - in namespace (ADL),
// member +=.
func check(it: ns3.NamespacedTemplatedRACIteratorInt) {
  checkInput(it)
  checkRandomAccess(it)
}

// Case 4: operator== in a different namespace — ADL won't find it.
// Should NOT conform to UnsafeCxxInputIterator.
func check(it: ns4_types.UnfindableEqIterator) {
  checkInput(it) // expected-error {{requires}}
  checkRandomAccess(it) // expected-error {{requires}}
}

// Case 5: RAC iterator inheriting ==, -, and += from a base class; forwarding
// wrappers are synthesized for the derived type.
func check(it: ns5.InheritedOpsRACIterator) {
  checkInput(it)
  checkRandomAccess(it)
}
