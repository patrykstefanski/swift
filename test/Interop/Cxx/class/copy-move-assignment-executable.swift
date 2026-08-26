// RUN: %target-run-simple-swift(-I %S/Inputs/ -Xfrontend -enable-experimental-cxx-interop)
//
// Re-test with optimizations:
// RUN: %target-run-simple-swift(-I %S/Inputs/ -Xfrontend -enable-experimental-cxx-interop -O)
//
// REQUIRES: executable_test

import StdlibUnittest
import CopyMoveAssignment

var CxxCopyMoveAssignTestSuite = TestSuite("CxxCopyMoveAssignTestSuite")

@inline(never)
func takeValue<T>(_ x: T) {
    let _ = x
}

CxxCopyMoveAssignTestSuite.test("NonTrivialCopyAssign") {
    do {
        var instance = NonTrivialCopyAssign()
        expectEqual(0, instance.copyAssignCounter)
        let instance2 = NonTrivialCopyAssign()
        instance = instance2
        // `operator=` isn't called.
        expectEqual(0, instance.copyAssignCounter)
        takeValue(instance2)
    }
    // The number of constructors and destructors called for `NonTrivialCopyAssign` must be balanced.
    expectEqual(0, InstanceBalanceCounter.getCounterValue())
}

CxxCopyMoveAssignTestSuite.test("NonTrivialMoveAssign") {
    do {
        var instance = NonTrivialMoveAssign()
        expectEqual(0, instance.moveAssignCounter)
        instance = NonTrivialMoveAssign()
        // `operator=` isn't called.
        expectEqual(0, instance.moveAssignCounter)
    }
    // The number of constructors and destructors called for `NonTrivialCopyAssign` must be balanced.
    expectEqual(0, InstanceBalanceCounter.getCounterValue())
}

CxxCopyMoveAssignTestSuite.test("NonTrivialCopyAndCopyMoveAssign") {
    do {
        var instance = NonTrivialCopyAndCopyMoveAssign()
        expectEqual(0, instance.assignCounter)
        let instance2 = NonTrivialCopyAndCopyMoveAssign()
        instance = instance2
        // `operator=` isn't called.
        expectEqual(0, instance.assignCounter)
        takeValue(instance2)
    }
    // The number of constructors and destructors called for `NonTrivialCopyAndCopyMoveAssign` must be balanced.
    expectEqual(0, InstanceBalanceCounter.getCounterValue())
}

@inline(never)
func assign(_ dst: UnsafeMutablePointer<NonTrivialSelfAssign>,
            from src: UnsafePointer<NonTrivialSelfAssign>) {
    dst.pointee = src.pointee
}

@inline(never)
func assign(_ buffer: UnsafeMutableBufferPointer<NonTrivialSelfAssign>,
            _ i: Int, from j: Int) {
    buffer[i] = buffer[j]
}

// Not specialized, so the assignment goes through the value witness.
@_semantics("optimize.sil.specialize.generic.never")
@inline(never)
func assignGeneric<T>(_ dst: UnsafeMutablePointer<T>, from src: UnsafePointer<T>) {
    dst.pointee = src.pointee
}

// Assigning a value to itself in place must leave it intact: the temporary
// copy of the source is eliminated (for the buffer subscript already at
// -Onone), so the witness sees the same address as source and destination.
func checkSelfAssign(_ body: (UnsafeMutablePointer<NonTrivialSelfAssign>) -> Void) {
    do {
        let instance = UnsafeMutablePointer<NonTrivialSelfAssign>.allocate(capacity: 1)
        instance.initialize(to: NonTrivialSelfAssign(42))
        body(instance)
        expectEqual(42, instance.pointee.value)
        instance.deinitialize(count: 1)
        instance.deallocate()
    }
    expectEqual(0, InstanceBalanceCounter.getCounterValue())
}

CxxCopyMoveAssignTestSuite.test("NonTrivialSelfAssign/pointee") {
    checkSelfAssign { assign($0, from: UnsafePointer($0)) }
}

CxxCopyMoveAssignTestSuite.test("NonTrivialSelfAssign/bufferSubscript") {
    checkSelfAssign { assign(UnsafeMutableBufferPointer(start: $0, count: 1), 0, from: 0) }
}

CxxCopyMoveAssignTestSuite.test("NonTrivialSelfAssign/valueWitness") {
    checkSelfAssign { assignGeneric($0, from: UnsafePointer($0)) }
}

runAllTests()
