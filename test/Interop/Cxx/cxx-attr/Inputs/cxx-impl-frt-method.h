#ifndef TEST_INTEROP_CXX_IMPL_FRT_METHOD_H
#define TEST_INTEROP_CXX_IMPL_FRT_METHOD_H

#define SWIFT_SHARED_REFERENCE(_retain, _release)                              \
  __attribute__((swift_attr("import_reference")))                              \
  __attribute__((swift_attr("retain:" #_retain)))                             \
  __attribute__((swift_attr("release:" #_release)))

struct Box;
void Box_retain(Box *_Nonnull);
void Box_release(Box *_Nonnull);

// Foreign-reference type with non-virtual instance methods implemented in Swift.
struct Box {
  int value;
  int get() const;     // non-virtual const   -> _ZNK3Box3getEv
  void add(int n);     // non-virtual non-const -> _ZN3Box3addEi
} SWIFT_SHARED_REFERENCE(Box_retain, Box_release);

Box *_Nonnull makeBox(int v);  // factory, defined in the C++ main

#endif
