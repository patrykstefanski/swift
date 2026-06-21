// RUN: %target-swift-emit-ir \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -import-objc-header %S/Inputs/cxx-impl-irgen.h \
// RUN:   %s | %FileCheck %s

// REQUIRES: swift_feature_CxxImplementation

// int cxxFreeFunc(int param);
// CHECK-LABEL: define{{.*}} i32 @_Z11cxxFreeFunci
@cxx @implementation
public func cxxFreeFunc(_ param: Int32) -> Int32 { return param }

// int anotherCxxFunc(int x, int y);
// CHECK-LABEL: define{{.*}} i32 @_Z14anotherCxxFuncii
@cxx @implementation
public func anotherCxxFunc(_ x: Int32, _ y: Int32) -> Int32 { return x + y }

// void voidCxxFunc();
// CHECK-LABEL: define{{.*}} void @_Z11voidCxxFuncv
@cxx @implementation
public func voidCxxFunc() {}

// int cxxFuncWithPointer(int *ptr);
// CHECK-LABEL: define{{.*}} i32 @_Z18cxxFuncWithPointerPi
@cxx @implementation
public func cxxFuncWithPointer(_ ptr: UnsafeMutablePointer<Int32>?) -> Int32 { return ptr!.pointee }

// int overloadedByArity(int x);
// CHECK-LABEL: define{{.*}} i32 @_Z17overloadedByArityi
@cxx @implementation
public func overloadedByArity(_ x: Int32) -> Int32 { return x }

// int overloadedByArity(int x, int y);
// CHECK-LABEL: define{{.*}} i32 @_Z17overloadedByArityii
@cxx @implementation
public func overloadedByArity(_ x: Int32, _ y: Int32) -> Int32 { return x + y }

// void primitiveTypes(long l, char c, float f, double d, bool b);
// CHECK-LABEL: define{{.*}} void @_Z14primitiveTypeslcfdb
@cxx @implementation
public func primitiveTypes(_ l: CLong, _ c: CChar, _ f: Float, _ d: Double, _ b: Bool) {}

// int64_t int64Func(int64_t x);
// CHECK-LABEL: define{{.*}} i64 @_Z9int64Funcx
@cxx @implementation
public func int64Func(_ x: Int64) -> Int64 { return x }

// void constPointerFunc(const int *ptr);
// CHECK-LABEL: define{{.*}} void @_Z16constPointerFuncPKi
@cxx @implementation
public func constPointerFunc(_ ptr: UnsafePointer<Int32>?) {}

// SimpleStruct returnStruct(int x, int y);
// CHECK-LABEL: define{{.*}} @_Z12returnStructii
@cxx @implementation
public func returnStruct(_ x: Int32, _ y: Int32) -> SimpleStruct {
  return SimpleStruct(x: x, y: y)
}

// int acceptStruct(SimpleStruct s);
// CHECK-LABEL: define{{.*}} i32 @_Z12acceptStruct12SimpleStruct
@cxx @implementation
public func acceptStruct(_ s: SimpleStruct) -> Int32 { return s.x }

// `@cxx(name:)` matches `int renamedTarget(int)` by that C++ name even though
// the Swift function is named differently; the body is still emitted under the
// matched declaration's Itanium-mangled symbol, proving `name:` drives matching
// rather than verbatim symbol emission.
// CHECK-LABEL: define{{.*}} i32 @_Z13renamedTargeti
@cxx(name: "renamedTarget") @implementation
public func swiftRenamedFunc(_ param: Int32) -> Int32 { return param }

// Same-arity overloads disambiguated by Swift parameter type; each body is
// emitted under the matched overload's Itanium symbol.
// CHECK-LABEL: define{{.*}} i32 @_Z9sameArityi
@cxx @implementation
public func sameArity(_ x: Int32) -> Int32 { return x }

// CHECK-LABEL: define{{.*}} double @_Z9sameArityd
@cxx @implementation
public func sameArity(_ x: Double) -> Double { return x }


// CHECK-LABEL: define{{.*}} swiftcc void @"$s4main12callCxxFuncsyyF"
// CHECK:   invoke i32 @_Z11cxxFreeFunci
// CHECK:   invoke i32 @_Z14anotherCxxFuncii
// CHECK:   invoke void @_Z11voidCxxFuncv
// CHECK:   invoke i32 @_Z18cxxFuncWithPointerPi
public func callCxxFuncs() {
  _ = cxxFreeFunc(1)
  _ = anotherCxxFunc(2, 3)
  voidCxxFunc()
  var x: Int32 = 42
  _ = cxxFuncWithPointer(&x)
}
