struct Animal {                 // single, standalone polymorphic class (in scope)
  int id;
  Animal(int i) : id(i) {}
  virtual int sound() const;    // key function, implemented in Swift
};

struct Base { virtual void f(); };
struct Derived2 : Base, Animal { void f() override; };  // multiple inheritance (out of scope)

struct VBase { virtual int h() const; };
struct VDerived : virtual VBase { virtual int g() const; };  // virtual base (out of scope)

// Abstract base (pure virtual): Swift implements the derived class's key
// function. Dispatch through the abstract-base pointer reaches the Swift body.
struct Shape {
  int id;
  Shape(int i) : id(i) {}
  virtual int area() const = 0; // pure virtual -> Shape is abstract (no key fn)
};
struct Square : Shape {
  Square(int i) : Shape(i) {}
  int area() const override;    // Square's key function -> implemented in Swift
};

// Swift implements the key function of a class that ALSO has a pure-virtual
// slot (the Swift-emitted vtable must place __cxa_pure_virtual there).
struct Partial {
  int id;
  Partial(int i) : id(i) {}
  virtual int impl() const;     // key function -> implemented in Swift
  virtual int extra() const = 0;
};

// Deep non-virtual single-inheritance chain: Swift implements the leaf
// override (the leaf's key function).
struct L0 { int a; L0(int x) : a(x) {} virtual int f() const { return a; } };
struct L1 : L0 { L1(int x) : L0(x) {} };
struct L2 : L1 { L2(int x) : L1(x) {} int f() const override; };

// Indirect virtual base (out of scope: Swift cannot lower the value-type
// layout). `MidV` derives a virtual base; classes that derive `MidV`
// non-virtually therefore have `VBaseA` as an *indirect* virtual base.
struct VBaseA { int a; VBaseA(int x) : a(x) {} virtual int vf() const { return a; } };
struct MidV : virtual VBaseA { int m; MidV(int x) : VBaseA(x), m(x) {} };
// Single indirect path to the virtual base.
struct IndirectVB : MidV {
  int z;
  IndirectVB(int x) : VBaseA(x), MidV(x), z(x) {}
  int vf() const override;
};
// Classic virtual diamond: `VBaseA` shared via two non-virtual bases.
struct MidV2 : virtual VBaseA { int n; MidV2(int x) : VBaseA(x), n(x) {} };
struct Diamond : MidV, MidV2 {
  int d;
  Diamond(int x) : VBaseA(x), MidV(x), MidV2(x), d(x) {}
  int vf() const override;
};

