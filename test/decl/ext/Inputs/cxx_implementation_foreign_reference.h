#define SWIFT_SHARED_REFERENCE(_retain, _release)                              \
  __attribute__((swift_attr("import_reference")))                              \
  __attribute__((swift_attr("retain:" #_retain)))                             \
  __attribute__((swift_attr("release:" #_release)))

struct Widget;
void retainWidget(Widget *);
void releaseWidget(Widget *);

// Foreign-reference type (imports as a Swift *class*, reference semantics).
struct Widget {
  int id;
  Widget(int i) : id(i) {}
  virtual int describe() const;   // virtual instance method
  int tag() const;                // non-virtual instance method
  int count(int n) const;         // const/non-const overload pair (same params)
  int count(int n);
} SWIFT_SHARED_REFERENCE(retainWidget, releaseWidget);

// Plain value-type C++ record (imports as a struct) -- the contrast case.
struct Value {
  int x;
  Value(int v) : x(v) {}
  int get() const;
};
