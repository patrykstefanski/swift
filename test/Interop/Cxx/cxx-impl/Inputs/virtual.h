#ifndef TEST_INTEROP_CXX_CXX_IMPL_VIRTUAL_H
#define TEST_INTEROP_CXX_CXX_IMPL_VIRTUAL_H

// A polymorphic class whose key function is implemented in Swift, so the Swift
// module emits the vtable and RTTI.

struct Shape {
  int sides;

  // The key function.
  virtual int area() const;
  virtual void scale(int factor);
  // The vtable names an inline virtual method, so the module emitting the
  // vtable emits the method's body too.
  virtual int perimeter() const { return 4 * sides; }
};

// A class whose key function stays in C++: Swift emits no vtable for it. An
// override along single, non-virtual inheritance with an unchanged return
// type needs no adjusting thunk.

struct SimpleBase {
  int stored;

  // The key function; its body stays in C++ (the execution test's main file).
  virtual void sbAnchor();
  virtual int simple() const;
};
struct SimpleDerived : SimpleBase {
  // The key function; its body stays in C++ (the execution test's main file).
  virtual void sdAnchor();
  int simple() const override;
};

// A pure virtual method's vtable slot dispatches to an overriding method,
// never to a definition of the method itself. The class's key function is
// implementable like any other.

struct Abstract {
  // The key function.
  virtual int anchor() const;
  virtual int pureMethod() const = 0;
};

// A covariant return type crossing to a base at a nonzero offset makes the
// overridden slot name a return-adjusting thunk.

struct RetA {
  int a;
};
struct RetB {
  int b;
};
struct RetC : RetA, RetB {};

RetC *_Nonnull sharedRetC();

struct CloneBase {
  virtual RetB *_Nonnull clone();
};
struct CloneDerived : CloneBase {
  // The key function; its body stays in C++ (the execution test's main file).
  virtual void cloneAnchor();
  RetC *_Nonnull clone() override;
};

// Multiple inheritance: an override of a method of the non-primary base makes
// that base's secondary vtable name a this-adjusting thunk. The key function
// is implemented in Swift, so the Swift module emits the vtable group, the
// thunk, and the derived class's implicit destructor, which the vtable names.

extern int destroyedMIBaseA;

struct MIBaseA {
  int a;

  virtual ~MIBaseA() { ++destroyedMIBaseA; }
  virtual void firstA();
};
struct MIBaseB {
  int b;

  virtual int fromB() const;
};
struct MIDerived : MIBaseA, MIBaseB {
  // The key function.
  virtual void miAnchor();
  void firstA() override;
  int fromB() const override;
};

// A virtual base: an override of a method of the virtual base makes the base's
// vtable name a this-adjusting thunk with a virtual (vcall-offset) adjustment.
// The Swift module emits the vtable, VTT, and thunk.

struct VBase {
  int vb;

  virtual int vbMethod() const;
};
struct VDerived : virtual VBase {
  int vd;

  // The key function.
  virtual void vAnchor();
  int vbMethod() const override;
};

// A foreign reference type: Swift calls dispatch dynamically through the
// importer's synthesized thunk, while the Swift implementation provides the
// body the vtable slot names. The key function is implemented in Swift.

struct Engine;
void retainEngine(Engine *_Nonnull);
void releaseEngine(Engine *_Nonnull);

struct __attribute__((swift_attr("import_reference")))
__attribute__((swift_attr("retain:retainEngine")))
__attribute__((swift_attr("release:releaseEngine"))) Engine {
  int rpm;

  // The key function.
  virtual int status() const;
  virtual void boost(int amount);
};

// A pure virtual method of a foreign reference type.

struct AbstractEngine;
void retainAbstractEngine(AbstractEngine *_Nonnull);
void releaseAbstractEngine(AbstractEngine *_Nonnull);

struct __attribute__((swift_attr("import_reference")))
__attribute__((swift_attr("retain:retainAbstractEngine")))
__attribute__((swift_attr("release:releaseAbstractEngine"))) AbstractEngine {
  // The key function.
  virtual int aeAnchor() const;
  virtual int pureStatus() const = 0;
};

#endif // !TEST_INTEROP_CXX_CXX_IMPL_VIRTUAL_H
