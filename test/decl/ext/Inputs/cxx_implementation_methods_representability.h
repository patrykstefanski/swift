// Types for test/decl/ext/cxx_implementation_methods_representability.swift.

struct TrivialPair { int a; int b; };

// Non-trivial C++ class (non-trivial copy constructor + destructor). Using it as
// a by-value parameter or result of a `@cxx @implementation` method is out of
// scope, just as for free functions.
class NonTrivial {
public:
  NonTrivial() {}
  NonTrivial(const NonTrivial &other) : value(other.value) {}
  ~NonTrivial() {}
  int value;
};

struct Host {
  int x;
  int takesNonTrivial(NonTrivial n) const;  // non-trivial by-value param.
  int okTrivial(TrivialPair p) const;       // trivial: representable.
};

// A non-trivial record used as a *receiver*: this is allowed, because
// `self`/`this` is always passed by pointer (never copied at the boundary).
class NonTrivialReceiver {
public:
  NonTrivialReceiver() {}
  NonTrivialReceiver(const NonTrivialReceiver &o) : value(o.value) {}
  ~NonTrivialReceiver() {}
  int value;
  int read() const;
};
