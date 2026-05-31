// RUN: %target-swift-frontend \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_virtual.h -emit-ir \
// RUN:   -target %target-future-triple %s > %t.ir
// RUN: %FileCheck --input-file %t.ir %s

// REQUIRES: swift_feature_CxxImplementation

// Implement the virtual key function `Animal::sound()` in Swift. The body lowers
// like a const instance method (`this` first), and because it is the class's key
// function, Swift drives clang to emit the class vtable + RTTI here. The same is
// checked for an abstract base's derived key function (`Square::area`, whose
// RTTI references the abstract base's RTTI) and for a class with a pure-virtual
// slot (`Partial`, whose Swift-emitted vtable holds __cxa_pure_virtual).


extension Animal {
  @cxx @implementation
  public func sound() -> Int32 { return id * 10 }
}

/// The method body: const instance method, `this` (ptr) first.
// CHECK-DAG: define{{.*}} i32 @_ZNK6Animal5soundEv(ptr

/// The vtable is emitted here (a definition, not `external`), with the slot
/// pointing at the Swift-provided method body.
// CHECK-DAG: @_ZTV6Animal = {{(dso_local )?}}{{(unnamed_addr )?}}constant {{.*}}@_ZNK6Animal5soundEv

/// RTTI is emitted too (the vtable embeds a pointer to it).
// CHECK-DAG: @_ZTI6Animal =
// CHECK-DAG: @_ZTS6Animal =

// Abstract base: Swift implements the derived key function `Square::area`. Swift
// emits Square's vtable, and Square's RTTI references the abstract base's RTTI
// (`_ZTI5Shape`) -- the base-class RTTI chain is wired correctly.
extension Square {
  @cxx @implementation
  public func area() -> Int32 { return id * id }
}
// CHECK-DAG: define{{.*}} i32 @_ZNK6Square4areaEv(ptr
// CHECK-DAG: @_ZTV6Square = {{(dso_local )?}}{{(unnamed_addr )?}}constant {{.*}}@_ZNK6Square4areaEv
// CHECK-DAG: @_ZTI6Square = {{.*}}@_ZTI5Shape

// Swift implements `Partial::impl` (the key function); `Partial` also has a
// pure-virtual slot `extra()`, which the Swift-emitted vtable fills with
// __cxa_pure_virtual.
extension Partial {
  @cxx @implementation
  public func impl() -> Int32 { return id * 7 }
}
// CHECK-DAG: define{{.*}} i32 @_ZNK7Partial4implEv(ptr
// CHECK-DAG: @_ZTV7Partial = {{.*}}@_ZNK7Partial4implEv{{.*}}@__cxa_pure_virtual

