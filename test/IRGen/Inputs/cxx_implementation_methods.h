// Header for `@cxx @implementation` C++ *method* IRGen tests: verifies the
// emitted symbols use the Itanium method mangling and the C++ method ABI
// (`this` passed first).

struct Pair { long a; long b; long c; };  // 24 bytes: returned indirectly (sret).

struct Vec {
  int x;

  // Static method: no `this`.
  static Vec zero();

  // Const instance method: `this` is `const Vec *` (mangles as `_ZNK...`).
  int dot(Vec o) const;

  // Non-const instance method: `this` is `Vec *` (mangles as `_ZN...`).
  void scale(int k);

  // Struct-returning const instance method: exercises sret + `this` ordering.
  Pair spread(int k) const;
};
