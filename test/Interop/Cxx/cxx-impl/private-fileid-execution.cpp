// A C++ program calls `@cxx @implementation` methods that use non-public
// members, opened with SWIFT_PRIVATE_FILEID to this test's Swift file.

// RUN: %empty-directory(%t)
// RUN: split-file %s %t

// RUN: %target-interop-build-clangxx \
// RUN:   -c %t/main.cpp \
// RUN:   -I %t/Inputs \
// RUN:   -o %t/private-fileid-execution-main.o
// RUN: %target-interop-build-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -module-name PrivateFileIDExecutionMain \
// RUN:   -parse-as-library \
// RUN:   -I %t/Inputs \
// RUN:   -Xlinker %t/private-fileid-execution-main.o \
// RUN:   %t/private-fileid-execution.swift \
// RUN:   -o %t/private-fileid-execution
// RUN: %target-codesign %t/private-fileid-execution
// RUN: %target-run %t/private-fileid-execution | %FileCheck %s

// The same, with protected members.

// RUN: %target-interop-build-clangxx \
// RUN:   -c %t/main.cpp \
// RUN:   -I %t/Inputs \
// RUN:   -DTEST_PRIVATE=protected \
// RUN:   -o %t/private-fileid-execution-main-protected.o
// RUN: %target-interop-build-swift \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -module-name PrivateFileIDExecutionMain \
// RUN:   -parse-as-library \
// RUN:   -I %t/Inputs \
// RUN:   -Xcc -DTEST_PRIVATE=protected \
// RUN:   -Xlinker %t/private-fileid-execution-main-protected.o \
// RUN:   %t/private-fileid-execution.swift \
// RUN:   -o %t/private-fileid-execution-protected
// RUN: %target-codesign %t/private-fileid-execution-protected
// RUN: %target-run %t/private-fileid-execution-protected | %FileCheck %s

// REQUIRES: executable_test
// REQUIRES: swift_feature_CxxImplementation

//--- Inputs/module.modulemap
module PrivateFileID {
  header "private-fileid.h"
  requires cplusplus
}

//--- Inputs/private-fileid.h
#ifndef TEST_INTEROP_CXX_CXX_IMPL_PRIVATE_FILEID_H
#define TEST_INTEROP_CXX_CXX_IMPL_PRIVATE_FILEID_H

// Override this to test protected
#ifndef TEST_PRIVATE
#define TEST_PRIVATE private
#endif

#define BLESS                                                                  \
  __attribute__((__swift_attr__(                                               \
      "private_fileid:PrivateFileIDExecutionMain/private-fileid-execution.swift")))

#define IMMORTAL                                                               \
  __attribute__((swift_attr("import_reference")))                              \
  __attribute__((swift_attr("retain:immortal")))                               \
  __attribute__((swift_attr("release:immortal")))

// A value type

struct BLESS Counter {
TEST_PRIVATE:
  int count = 0;
  int bump(int by) { return count += by; }
  int twice() const;

public:
  int get() const;
  void set(int v);
  void add(int by);
  int doubled() const;
};

// A foreign reference type

class BLESS IMMORTAL Service {
TEST_PRIVATE:
  int secret = 42;
  int bump(int by) { return secret += by; }
  int twice() const;

public:
  int reveal() const;
  void rotate(int by);
  void add(int by);
  int doubled() const;
};

// A foreign reference type, annotated unlike its base

class ProtectedBase {
protected:
  int base = 100;
  int scaled(int by) const { return base * by; }
  int halved() const;
};

class BLESS IMMORTAL Derived : public ProtectedBase {
public:
  int readBase() const;
  int scaledBase(int by) const;
  int halvedBase() const;
};

#endif // TEST_INTEROP_CXX_CXX_IMPL_PRIVATE_FILEID_H

//--- private-fileid-execution.swift
import PrivateFileID

extension Counter {
  @cxx @implementation
  public func get() -> Int32 { return count }

  @cxx @implementation
  public mutating func set(_ v: Int32) { count = v }

  @cxx @implementation
  public mutating func add(_ by: Int32) { _ = bump(by) }

  @cxx @implementation
  public func doubled() -> Int32 { return twice() }
}

extension Service {
  @cxx @implementation
  public func reveal() -> Int32 { return secret }

  @cxx @implementation
  public func rotate(_ by: Int32) { secret += by }

  @cxx @implementation
  public func add(_ by: Int32) { _ = bump(by) }

  @cxx @implementation
  public func doubled() -> Int32 { return twice() }
}

// Inherited fields of a foreign reference type are read-only, even public ones.

extension Derived {
  @cxx @implementation
  public func readBase() -> Int32 { return base }

  @cxx @implementation
  public func scaledBase(_ by: Int32) -> Int32 { return scaled(by) }

  @cxx @implementation
  public func halvedBase() -> Int32 { return halved() }
}

//--- main.cpp
#include <stdio.h>

#include "private-fileid.h"

// Non-public methods called from Swift
int Counter::twice() const { return count * 2; }
int Service::twice() const { return secret * 2; }
int ProtectedBase::halved() const { return base / 2; }

int main() {
  // A value type
  Counter counter;
  printf("Counter get=%d\n", counter.get());
  // CHECK: Counter get=0

  counter.set(40);
  printf("Counter set=%d\n", counter.get());
  // CHECK: Counter set=40

  counter.add(2);
  printf("Counter add=%d\n", counter.get());
  // CHECK: Counter add=42

  printf("Counter doubled=%d\n", counter.doubled());
  // CHECK: Counter doubled=84

  // A foreign reference type
  Service service;
  printf("Service reveal=%d\n", service.reveal());
  // CHECK: Service reveal=42

  service.rotate(8);
  printf("Service rotate=%d\n", service.reveal());
  // CHECK: Service rotate=50

  service.add(5);
  printf("Service add=%d\n", service.reveal());
  // CHECK: Service add=55

  printf("Service doubled=%d\n", service.doubled());
  // CHECK: Service doubled=110

  // Protected members of an unannotated base
  Derived derived;
  printf("Derived readBase=%d\n", derived.readBase());
  // CHECK: Derived readBase=100

  printf("Derived scaledBase=%d\n", derived.scaledBase(3));
  // CHECK: Derived scaledBase=300

  printf("Derived halvedBase=%d\n", derived.halvedBase());
  // CHECK: Derived halvedBase=50

  return 0;
}
