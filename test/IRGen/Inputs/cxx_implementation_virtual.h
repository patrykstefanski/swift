struct Animal {
  int id;
  Animal(int i) : id(i) {}
  virtual int sound() const;   // key function, implemented in Swift
};

// Abstract base (pure virtual): Swift implements the derived key function.
struct Shape {
  int id;
  Shape(int i) : id(i) {}
  virtual int area() const = 0;
};
struct Square : Shape {
  Square(int i) : Shape(i) {}
  int area() const override;   // Square's key function, implemented in Swift
};

// Swift implements the key function of a class that also has a pure-virtual
// slot: the Swift-emitted vtable must place __cxa_pure_virtual in that slot.
struct Partial {
  int id;
  Partial(int i) : id(i) {}
  virtual int impl() const;    // key function, implemented in Swift
  virtual int extra() const = 0;
};

