// SWIFT_PRIVATE_FILEID gives `@cxx @implementation` bodies access to non-public
// members, as for any extension. The header names "main/blessed.swift".
//
// RUN: %empty-directory(%t)
// RUN: split-file %s %t
//
// blessed.swift has access. No -verify-ignore-unrelated: the header must be
// clean too.
//
// RUN: %target-swift-frontend -typecheck -verify \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -I %t%{fs-sep}Inputs -module-name main %t%{fs-sep}blessed.swift
// RUN: %target-swift-frontend -typecheck -verify \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -I %t%{fs-sep}Inputs -module-name main %t%{fs-sep}blessed.swift \
// RUN:   -Xcc -DTEST_PRIVATE=protected
//
// cursed.swift isn't named, so it has no access.
//
// RUN: %target-swift-frontend -typecheck -verify -verify-ignore-unrelated \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -I %t%{fs-sep}Inputs -module-name main %t%{fs-sep}cursed.swift
// RUN: %target-swift-frontend -typecheck -verify -verify-ignore-unrelated \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -I %t%{fs-sep}Inputs -module-name main %t%{fs-sep}cursed.swift \
// RUN:   -Xcc -DTEST_PRIVATE=protected
//
// unannotated.swift implements a class without the annotation.
//
// RUN: %target-swift-frontend -typecheck -verify -verify-ignore-unrelated \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -I %t%{fs-sep}Inputs -module-name main %t%{fs-sep}unannotated.swift
// RUN: %target-swift-frontend -typecheck -verify -verify-ignore-unrelated \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -target %target-swift-5.8-abi-triple \
// RUN:   -I %t%{fs-sep}Inputs -module-name main %t%{fs-sep}unannotated.swift \
// RUN:   -Xcc -DTEST_PRIVATE=protected

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

#define BLESS __attribute__((__swift_attr__("private_fileid:main/blessed.swift")))

#define IMMORTAL                                                               \
  __attribute__((swift_attr("import_reference")))                              \
  __attribute__((swift_attr("retain:immortal")))                               \
  __attribute__((swift_attr("release:immortal")))

// A value type

struct BLESS Counter {
TEST_PRIVATE:
  int count;
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
  int secret;
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
  int base;
  int scaled(int by) const { return base * by; }
  int halved() const;
};

class BLESS IMMORTAL Derived : public ProtectedBase {
public:
  int readBase() const;
  int scaledBase(int by) const;
  int halvedBase() const;
};

// A value type without the annotation

struct Unannotated {
TEST_PRIVATE:
  int count;
  int bump(int by) { return count += by; }
  int twice() const;

public:
  int get() const;
  void add(int by);
  int doubled() const;
};

#endif // TEST_INTEROP_CXX_CXX_IMPL_PRIVATE_FILEID_H

//--- blessed.swift
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

//--- cursed.swift
import PrivateFileID

// Protected members are diagnosed as private too.

extension Counter {
  @cxx @implementation
  public func get() -> Int32 { return count } // expected-error {{'count' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public mutating func set(_ v: Int32) { count = v } // expected-error {{'count' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public mutating func add(_ by: Int32) { _ = bump(by) } // expected-error {{'bump' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public func doubled() -> Int32 { return twice() } // expected-error {{'twice' is inaccessible due to 'private' protection level}}
}

extension Service {
  @cxx @implementation
  public func reveal() -> Int32 { return secret } // expected-error {{'secret' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public func rotate(_ by: Int32) { secret += by } // expected-error {{'secret' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public func add(_ by: Int32) { _ = bump(by) } // expected-error {{'bump' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public func doubled() -> Int32 { return twice() } // expected-error {{'twice' is inaccessible due to 'private' protection level}}
}

extension Derived {
  @cxx @implementation
  public func readBase() -> Int32 { return base } // expected-error {{'base' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public func scaledBase(_ by: Int32) -> Int32 { return scaled(by) } // expected-error {{'scaled' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public func halvedBase() -> Int32 { return halved() } // expected-error {{'halved' is inaccessible due to 'private' protection level}}
}

//--- unannotated.swift
import PrivateFileID

// Non-public fields are imported but inaccessible; methods aren't imported.

extension Unannotated {
  @cxx @implementation
  public func get() -> Int32 { return count } // expected-error {{'count' is inaccessible due to 'private' protection level}}

  @cxx @implementation
  public mutating func add(_ by: Int32) { _ = bump(by) } // expected-error {{cannot find 'bump' in scope}}

  @cxx @implementation
  public func doubled() -> Int32 { return twice() } // expected-error {{cannot find 'twice' in scope}}
}
