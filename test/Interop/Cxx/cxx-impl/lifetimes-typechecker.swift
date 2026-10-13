// C++ lifetime annotations on the declarations `@cxx @implementation`
// functions implement.

// RUN: %target-typecheck-verify-swift \
// RUN:   -cxx-interoperability-mode=default \
// RUN:   -enable-experimental-feature CxxImplementation \
// RUN:   -enable-experimental-feature Lifetimes \
// RUN:   -I %S/Inputs

// REQUIRES: swift_feature_CxxImplementation
// REQUIRES: swift_feature_Lifetimes

import Lifetimes


// A lifetimebound `const T &` parameter imports as `borrowing`, so the
// implementation spells it `borrowing` too, and a non-escapable result depends
// on it.

@unsafe @cxx @implementation
@_lifetime(borrow t)
func viewTracked(_ t: borrowing Tracked) -> TrackedView {
  return TrackedView(value: nil)
}

@unsafe @cxx @implementation
@_lifetime(borrow x)
func viewInt(_ x: borrowing CInt) -> IntView {
  return IntView(value: nil)
}

@unsafe @cxx @implementation
@_lifetime(&x)
func viewMutableInt(_ x: inout CInt) -> IntView {
  return IntView(value: nil)
}


// Reference and pointer results. The importer records their dependencies,
// which Swift drops as the results are Escapable, so the implementation
// declares none.

@unsafe @cxx @implementation
func pick(_ a: borrowing CInt, _ b: borrowing CInt) -> UnsafePointer<CInt> {
  fatalError()
}

@unsafe @cxx @implementation
func pickMutable(_ a: inout CInt) -> UnsafeMutablePointer<CInt> {
  fatalError()
}

@unsafe @cxx @implementation
func pointerToInt(_ x: borrowing CInt) -> UnsafePointer<CInt>? {
  fatalError()
}

@unsafe @cxx @implementation
func pointerToTracked(_ t: borrowing Tracked) -> UnsafePointer<Tracked>? {
  fatalError()
}


// A lifetimebound `this`.

extension Owner {
  @cxx @implementation
  func get() -> UnsafePointer<CInt> {
    fatalError()
  }

  @cxx @implementation
  @_lifetime(borrow self)
  func view() -> IntView {
    return IntView(value: nil)
  }

  @unsafe @cxx @implementation
  @_lifetime(borrow x)
  func viewOf(_ x: borrowing CInt) -> IntView {
    return IntView(value: nil)
  }
}


// A foreign reference type.

@cxx @implementation
@_lifetime(borrow n)
func viewNode(_ n: borrowing Node) -> IntView {
  return IntView(value: nil)
}


// Rejections.

extension Rejections {
  // The result depends on `x`, not on `self`.
  // expected-error@+3{{instance method 'viewOfWrongSource' cannot implement C++ function 'viewOfWrongSource' because its lifetime dependencies '@_lifetime(borrow self)' do not match '@_lifetime(borrow x)' declared by the header}}
  @unsafe @cxx @implementation
  @_lifetime(borrow self)
  func viewOfWrongSource(_ x: borrowing CInt) -> IntView {
    return IntView(value: nil)
  }
}

// expected-error@+3{{global function 'viewOfWrongParam' cannot implement C++ function 'viewOfWrongParam' because its lifetime dependencies '@_lifetime(borrow b)' do not match '@_lifetime(borrow a)' declared by the header}}
@unsafe @cxx @implementation
@_lifetime(borrow b)
func viewOfWrongParam(_ a: borrowing CInt, _ b: borrowing CInt) -> IntView {
  return IntView(value: nil)
}

// The import of a lifetimebound `Node &` is `borrowing`.
// expected-error@+3{{global function 'viewNodeByDefault' has a parameter ownership modifier that does not match the declaration in the header}}
@cxx @implementation
@_lifetime(borrow n)
func viewNodeByDefault(_ n: Node) -> IntView {
  return IntView(value: nil)
}
