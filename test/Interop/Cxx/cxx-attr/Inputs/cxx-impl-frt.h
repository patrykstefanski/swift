#ifndef TEST_INTEROP_CXX_IMPL_FRT_H
#define TEST_INTEROP_CXX_IMPL_FRT_H

#define SWIFT_SHARED_REFERENCE(_retain, _release)                              \
  __attribute__((swift_attr("import_reference")))                              \
  __attribute__((swift_attr("retain:" #_retain)))                             \
  __attribute__((swift_attr("release:" #_release)))
#define SWIFT_RETURNS_RETAINED __attribute__((swift_attr("returns_retained")))
#define SWIFT_RETURNS_UNRETAINED __attribute__((swift_attr("returns_unretained")))

struct Node;
void Node_retain(Node *_Nonnull);
void Node_release(Node *_Nonnull);

// Foreign-reference type: imports as a Swift class (reference semantics).
struct Node {
  int value;
} SWIFT_SHARED_REFERENCE(Node_retain, Node_release);

// Foreign-reference type as a parameter (borrowed, +0) and as a result. All
// implemented in Swift via `@cxx @implementation`. Only `+1` results are
// supported in phase 1; `+0` / unannotated results are diagnosed.
int valueOf(Node *_Nonnull n);                                  // +0 borrowed param
Node *_Nonnull dup(Node *_Nonnull n) SWIFT_RETURNS_RETAINED;    // +1 result (supported)
Node *_Nonnull keep(Node *_Nonnull n) SWIFT_RETURNS_UNRETAINED; // +0 result (diagnosed)
Node *_Nonnull unann(Node *_Nonnull n);                         // unannotated (diagnosed)

#endif
