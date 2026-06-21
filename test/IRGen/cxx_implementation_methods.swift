// RUN: %target-swift-frontend \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -import-objc-header %S/Inputs/cxx_implementation_methods.h -emit-ir \
// RUN:   -target %target-future-triple %s > %t.ir
// RUN: %FileCheck --input-file %t.ir %s

// REQUIRES: swift_feature_CxxImplementation

// Implement C++ methods of the imported record `Vec` in a Swift extension. Each
// body must be emitted under the method's Itanium-mangled symbol, with the C++
// method ABI: instance methods take `this` as their leading pointer argument
// (after any indirect return), const methods mangle as `_ZNK...`.

extension Vec {
  // Static method: no `this`.
  @cxx @implementation
  public static func zero() -> Vec { return Vec(x: 0) }

  // Const instance method.
  @cxx @implementation
  public func dot(_ o: Vec) -> Int32 { return x * o.x }

  // Non-const instance method (maps to a non-const C++ method).
  @cxx @implementation
  public mutating func scale(_ k: Int32) { x = x * k }

  // Struct-returning const instance method: sret comes before `this`.
  @cxx @implementation
  public func spread(_ k: Int32) -> Pair {
    return Pair(a: Int(x), b: Int(k), c: Int(x) + Int(k))
  }

  // const/non-const overloads disambiguated by `mutating` (renamed via
  // `@cxx(name:)` so the two Swift impls don't collide): non-mutating binds the
  // const overload, mutating binds the non-const overload.
  @cxx(name: "probe") @implementation
  public func probeConst(_ k: Int32) -> Int32 { return x + k }

  @cxx(name: "probe") @implementation
  public mutating func probeMutating(_ k: Int32) -> Int32 { x = x + k; return x }
}

/// Static method: emitted under the Itanium symbol.
// CHECK-LABEL: define{{.*}} @_ZN3Vec4zeroEv

/// Const instance method: `this` (ptr) passed first, then `o` (Vec -> i64).
// CHECK-LABEL: define{{.*}} i32 @_ZNK3Vec3dotES_(ptr {{[^,]+}}, i64

/// Non-const instance method: `this` (ptr) passed first, then `k` (i32).
// CHECK-LABEL: define{{.*}} void @_ZN3Vec5scaleEi(ptr {{[^,]+}}, i32

/// Struct-returning const instance method: indirect result (sret) first, then
/// `this` (ptr), then `k` (i32).
// CHECK-LABEL: define{{.*}} void @_ZNK3Vec6spreadEi(ptr {{[^,]*}}sret{{[^,]+}}, ptr {{[^,]+}}, i32

/// const/non-const overloads: the non-mutating impl emits the const symbol, the
/// mutating impl the non-const symbol.
// CHECK-LABEL: define{{.*}} i32 @_ZNK3Vec5probeEi(ptr
// CHECK-LABEL: define{{.*}} i32 @_ZN3Vec5probeEi(ptr
