#ifndef TEST_INTEROP_CXX_IMPL_FRT_VIRTUAL_H
#define TEST_INTEROP_CXX_IMPL_FRT_VIRTUAL_H

#define SWIFT_SHARED_REFERENCE(_retain, _release)                              \
  __attribute__((swift_attr("import_reference")))                              \
  __attribute__((swift_attr("retain:" #_retain)))                             \
  __attribute__((swift_attr("release:" #_release)))

struct Animal;
void Animal_retain(Animal *_Nonnull);
void Animal_release(Animal *_Nonnull);

// Foreign-reference type with a virtual method whose body is in Swift. `sound` is
// the key function, so Swift emits Animal's vtable/RTTI.
struct Animal {
  int id;
  virtual int sound() const;   // -> vtable slot points at the Swift body
} SWIFT_SHARED_REFERENCE(Animal_retain, Animal_release);

Animal *_Nonnull makeAnimal(int id);  // defined in the C++ main

#endif
