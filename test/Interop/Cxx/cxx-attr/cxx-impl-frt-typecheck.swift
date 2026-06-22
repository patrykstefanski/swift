// Phase 1 of foreign-reference-type (FRT) support for `@cxx @implementation`:
// FRTs are usable as parameters (borrowed) and as `+1` (SWIFT_RETURNS_RETAINED)
// results. A `+0` (SWIFT_RETURNS_UNRETAINED, or unannotated) FRT result is not
// yet supported and is diagnosed (the Swift body produces +1, which would leak
// against a +0 contract).

// RUN: %target-swift-frontend -typecheck -verify %s \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -disable-availability-checking \
// RUN:   -import-objc-header %S/Inputs/cxx-impl-frt.h

// REQUIRES: swift_feature_CxxImplementation

// FRT as a parameter (borrowed): supported.
@cxx @implementation
func valueOf(_ n: Node) -> Int32 { return n.value }

// FRT result, +1 (SWIFT_RETURNS_RETAINED): supported.
@cxx @implementation
func dup(_ n: Node) -> Node { return n }

// FRT result, +0 (SWIFT_RETURNS_UNRETAINED): not yet supported.
@cxx @implementation // expected-error {{'@cxx @implementation' for a function returning a foreign-reference type requires a '+1' result; annotate the C++ declaration 'SWIFT_RETURNS_RETAINED' ('SWIFT_RETURNS_UNRETAINED' / unannotated results are not yet supported)}}
func keep(_ n: Node) -> Node { return n }

// FRT result, unannotated (defaults to +0): also not yet supported.
@cxx @implementation // expected-error {{'@cxx @implementation' for a function returning a foreign-reference type requires a '+1' result; annotate the C++ declaration 'SWIFT_RETURNS_RETAINED' ('SWIFT_RETURNS_UNRETAINED' / unannotated results are not yet supported)}}
func unann(_ n: Node) -> Node { return n }
