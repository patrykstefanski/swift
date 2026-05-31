#ifndef TEST_CXX_IMPL_VIRTUAL_H
#define TEST_CXX_IMPL_VIRTUAL_H
struct Animal {
  int id;
  Animal(int i) : id(i) {}
  virtual int sound() const;   // implemented in Swift
};
#endif
