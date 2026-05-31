// Header for `@cxx @implementation` C++ *method* tests. A Swift extension of an
// imported C++ record can provide the body for these methods; the body is
// emitted under the method's Itanium-mangled symbol.

struct Vec {
  int x;

  // Static method: scoped free function, no `this`.
  static Vec zero();

  // Const instance method: `this` is `const Vec *`.
  int dot(Vec o) const;

  // Non-const instance method: `this` is `Vec *` (maps to a `mutating` Swift
  // method).
  void scale(int k);

  // Another const method, used to check that a `mutating` Swift method cannot
  // implement a `const` C++ method.
  int probe() const;

  // Another non-const method, used to check that a non-`const` C++ method must
  // be implemented by a `mutating` Swift method.
  void touch();

  // Same-name overloads: matching by name is ambiguous, so these cannot be
  // implemented (matching fails, like overloaded free functions).
  int overloaded() const;
  int overloaded(int y) const;
};

struct Shape {
  int kind;
  // Virtual method: cannot be implemented in Swift (vtable dispatch).
  virtual int area() const;
};
