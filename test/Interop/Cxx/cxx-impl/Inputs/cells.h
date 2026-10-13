#ifndef TEST_INTEROP_CXX_CXX_IMPL_CELLS_H
#define TEST_INTEROP_CXX_CXX_IMPL_CELLS_H

#include <swift/bridging>

// A `T &` parameter is implemented by a `borrowing Cell<T>` and a `const T &`
// parameter by a `borrowing ConstCell<T>`. Their values may change under the
// borrow, so an implementation spelling every reference this way needs no
// `@unsafe`, unless it also has a `self` or a parameter of foreign reference
// type: the optimizer assumes no reference points into those.

// C++ callers may pass aliasing references, or references to storage the
// implementation reaches otherwise.

int observe(int &a, int &b);
int readTwice(const int &a, int &b);

extern int global;
int observeGlobal(int &x);
int readGlobal(const int &x);

extern int *_Nullable bumpTarget;
void bump();
int readAroundCallback(const int &x);
int writeAroundCallback(int &x);

// Non-trivial and move-only referents.

struct Tracked {
  int value;
  Tracked(int v);
  Tracked(const Tracked &other);
  ~Tracked();
};

int readTracked(const Tracked &t);
void bumpTracked(Tracked &t);
void assignTracked(Tracked &dst, const Tracked &src);

struct MoveOnly {
  int value;
  MoveOnly(int v);
  MoveOnly(MoveOnly &&other);
  MoveOnly(const MoveOnly &) = delete;
  ~MoveOnly();
};

int readMoveOnly(const MoveOnly &m);
void bumpMoveOnly(MoveOnly &m);
void replaceMoveOnly(MoveOnly &m, int v);

// Pointer slots.

struct SWIFT_IMMORTAL_REFERENCE Node {
  int value;
};

void reseatPtr(int *_Nullable &p, int *_Nullable to);
int readThroughPtr(int *_Nonnull const &p);
void reseatNode(Node *_Nonnull &p, Node *_Nonnull to);

// Methods, a namespace member and operators.

struct Counter {
  int count;
  static void reset(int &x);
  void addTo(int &x) const;
  void absorb(const int &x);
};

namespace ns {
void increment(int &x);
} // namespace ns

struct Vector {
  int x;
  int y;
  Vector &operator+=(const Vector &rhs);
};

bool operator==(const Vector &a, const Vector &b);

// A `Cell` or a `ConstCell` picks the reference among overloads.

int pick(int &x);
int pick(int *_Nonnull x);

// expected-note@+1{{found this candidate}}
int project(int x);
// expected-note@+1{{found this candidate}}
int project(const int &x);

// A lifetimebound reference: the `ConstCell` is the C++ referent.

struct SWIFT_NONESCAPABLE IntView {
  const int *_Nullable value;
};

IntView viewInt(const int &x [[clang::lifetimebound]]);

// Rejected: a `Cell` or `ConstCell` that is not `borrowing`.

void consumingCell(int &x);
void inoutCell(int &x);
void consumingConstCell(const int &x);

// Rejected: a `Cell` or `ConstCell` that does not spell the parameter.

void cellForConstRef(const int &x);
void constCellForRef(int &x);
void constCellForValue(int x);
void cellForPointer(int *_Nonnull x);
void cellWrongReferent(int &x);
void cellForNodeRef(Node &n);
void constCellForNodeRef(const Node &n);

// Rejected: implementations that are not marked `@unsafe` although they have
// a reference parameter spelled with the projection, a `self`, or a parameter
// of foreign reference type.

// expected-note@+1{{'b' declared as a C++ reference here}}
int mixedSpellings(int &a, int &b);
// expected-note@+1{{'x' declared as a C++ reference here}}
int takesNode(int &x, Node *_Nonnull n);
// expected-note@+2{{'slot' declared as a C++ reference here}}
// expected-note@+1{{'x' declared as a C++ reference here}}
void takesNodeSlot(Node *_Nonnull &slot, int &x);

struct Rejections {
  int count;
  // expected-note@+1{{'x' declared as a C++ reference here}}
  void addTo(int &x) const;
  // expected-note@+1{{'x' declared as a C++ reference here}}
  void absorb(const int &x);
  // expected-note@+1{{'rhs' declared as a C++ reference here}}
  Rejections &operator+=(const Rejections &rhs);
};

#endif
