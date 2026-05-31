#ifndef TEST_INTEROP_CXX_IMPLEMENTATION_METHODS_H
#define TEST_INTEROP_CXX_IMPLEMENTATION_METHODS_H

struct Vec {
  int x;
  static Vec make(int v);   // static method, implemented in Swift
  int dot(Vec o) const;     // const instance method, implemented in Swift
  void scale(int k);        // non-const instance method, implemented in Swift
};

#endif
