#ifndef TEST_INTEROP_CXX_CXX_IMPL_FOREIGN_REFERENCE_VIRTUAL_H
#define TEST_INTEROP_CXX_CXX_IMPL_FOREIGN_REFERENCE_VIRTUAL_H

// Foreign reference types with foreign reference bases, which import as Swift
// subclasses. The key functions stay in C++, except KeyDerived's.

struct Base;
void retainBase(Base *_Nonnull);
void releaseBase(Base *_Nonnull);

struct __attribute__((swift_attr("import_reference")))
__attribute__((swift_attr("retain:retainBase")))
__attribute__((swift_attr("release:releaseBase"))) Base {
  int value = 0;

  virtual void baseAnchor();
  virtual int describe() const;
  virtual int scaled(int factor) const;
};

// Overrides describe() and inherits scaled().
struct Derived : Base {
  virtual void derivedAnchor();
  int describe() const override;
};

// Overrides describe() and scaled().
struct Leaf : Derived {
  virtual void leafAnchor();
  int describe() const override;
  int scaled(int factor) const override;
};

// A pure virtual method implemented by a derived class.

struct AbstractBase;
void retainAbstractBase(AbstractBase *_Nonnull);
void releaseAbstractBase(AbstractBase *_Nonnull);

struct __attribute__((swift_attr("import_reference")))
__attribute__((swift_attr("retain:retainAbstractBase")))
__attribute__((swift_attr("release:releaseAbstractBase"))) AbstractBase {
  virtual void abAnchor();
  virtual int run() const = 0;
};

struct Concrete : AbstractBase {
  virtual void concreteAnchor();
  int run() const override;
};

// The override is the key function.
struct KeyDerived : Base {
  int describe() const override;
};

// The override of a non-primary base's method needs a this-adjusting thunk.
struct Unrelated {
  virtual int side() const;
  virtual ~Unrelated();
};

struct MI : Base, Unrelated {
  virtual void miAnchor();
  int side() const override;
  int describe() const override;
};

// Rejections
struct Rejections : Base {
  virtual void rejectionsAnchor();
  int describe() const override;
  int scaled(int factor) const override;
};

#endif
