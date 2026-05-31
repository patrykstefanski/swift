#ifndef TEST_CXX_IMPL_VIRTUAL_ABSTRACT_H
#define TEST_CXX_IMPL_VIRTUAL_ABSTRACT_H
// Abstract base with a pure virtual; Swift implements the derived class's key
// function. Swift emits Square's vtable + RTTI (and references the abstract
// base's RTTI). C++ dispatches through the abstract-base pointer.
struct Shape {
  int id;
  Shape(int i) : id(i) {}
  virtual int area() const = 0;   // pure virtual -> Shape is abstract
};
struct Square : Shape {
  Square(int i) : Shape(i) {}
  int area() const override;      // implemented in Swift (Square's key function)
};
#endif
