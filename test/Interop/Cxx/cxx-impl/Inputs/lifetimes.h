#ifndef TEST_INTEROP_CXX_CXX_IMPL_LIFETIMES_H
#define TEST_INTEROP_CXX_CXX_IMPL_LIFETIMES_H

#include <swift/bridging>

// A class with a user-provided copy constructor and destructor.
struct Tracked {
  int value;
  Tracked(int v);
  Tracked(const Tracked &other);
  ~Tracked();
};

// Non-escapable views of an `int` and of a `Tracked`.
struct SWIFT_NONESCAPABLE IntView {
  const int *value;
};

struct SWIFT_NONESCAPABLE TrackedView {
  const int *value;
};

// A lifetimebound `const T &` parameter imports as `borrowing`.

TrackedView viewTracked(const Tracked &t [[clang::lifetimebound]]);
IntView viewInt(const int &x [[clang::lifetimebound]]);
IntView viewMutableInt(int &x [[clang::lifetimebound]]);

// Reference and pointer results are Escapable, so they depend on nothing in
// Swift.

const int &pick(const int &a [[clang::lifetimebound]], const int &b);
int &pickMutable(int &a [[clang::lifetimebound]]);
const int *pointerToInt(const int &x [[clang::lifetimebound]]);
const Tracked *pointerToTracked(const Tracked &t [[clang::lifetimebound]]);

// A lifetimebound `this`.

struct Owner {
  int data;
  const int &get() const [[clang::lifetimebound]];
  IntView view() const [[clang::lifetimebound]];
  IntView viewOf(const int &x [[clang::lifetimebound]]) const;
};

// A lifetimebound reference to a foreign reference type imports as
// `borrowing`, whether or not it is `const`.

struct SWIFT_IMMORTAL_REFERENCE Node {
  int value;
};

IntView viewNode(Node &n [[clang::lifetimebound]]);

// Rejected implementations.

struct Rejections {
  IntView viewOfWrongSource(const int &x [[clang::lifetimebound]]) const;
};

IntView viewOfWrongParam(const int &a [[clang::lifetimebound]], const int &b);
IntView viewNodeByDefault(Node &n [[clang::lifetimebound]]);

#endif
