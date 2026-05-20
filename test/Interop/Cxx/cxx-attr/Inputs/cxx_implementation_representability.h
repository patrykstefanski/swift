// Types used by test/decl/ext/cxx_implementation_representability.swift.

// A trivial C++ struct: representable in both C and C++.
struct TrivialStruct {
  int x;
};

// A C++ class with a non-trivial copy constructor and destructor. Implementing
// a C++ function that passes/returns such a type by value is out of scope for
// the initial @cxx feature (lifetime management at the ABI boundary), so @cxx
// signatures using it must be rejected.
class NonTrivialClass {
public:
  NonTrivialClass() {}
  NonTrivialClass(const NonTrivialClass &other) : value(other.value) {}
  ~NonTrivialClass() {}
  int value;
};
