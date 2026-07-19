// RUN: %target-swift-emit-ir \
// RUN:   -I %S/Inputs \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   %s | %FileCheck %s

// REQUIRES: swift_feature_CxxImplementation

import GlobalFunctions

// int identity(int x)
// CHECK: define i32 @_Z8identityi(i32 %0)
@cxx @implementation
public func identity(_ x: Int32) -> Int32 { x }

// int add(int x, int y)
// CHECK: define i32 @_Z3addii(i32 %0, i32 %1)
@cxx @implementation
public func add(_ x: Int32, _ y: Int32) -> Int32 { x + y }

// int load(const int *p)
// CHECK: define i32 @_Z4loadPKi(ptr %0)
@cxx @implementation
public func load(_ p: UnsafePointer<Int32>?) -> Int32 { p!.pointee }

// void swap(int *p, int *q)
// CHECK: define void @_Z4swapPiS_(ptr %0, ptr %1)
@cxx @implementation
public func swap(_ p: UnsafeMutablePointer<Int32>?, _ q: UnsafeMutablePointer<Int32>?) {
    let tmp = p!.pointee
    p!.pointee = q!.pointee
    q!.pointee = tmp
}
