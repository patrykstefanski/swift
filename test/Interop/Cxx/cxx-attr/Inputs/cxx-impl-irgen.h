#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int cxxFreeFunc(int param);
int anotherCxxFunc(int x, int y);
void voidCxxFunc();
int cxxFuncWithPointer(int *ptr);

// Target for a `@cxx(name:)` whose Swift function is named differently.
int renamedTarget(int param);

int overloadedByArity(int x);
int overloadedByArity(int x, int y);

void primitiveTypes(long l, char c, float f, double d, bool b);
int64_t int64Func(int64_t x);
void constPointerFunc(const int *ptr);

struct SimpleStruct {
  int x;
  int y;
};

class NonTrivialClass {
public:
  NonTrivialClass() : value(0) {}
  NonTrivialClass(int v) : value(v) {}
  NonTrivialClass(const NonTrivialClass &other) : value(other.value) {}
  ~NonTrivialClass() {}
  int value;
};

SimpleStruct returnStruct(int x, int y);
int acceptStruct(SimpleStruct s);
int acceptClassByRef(const NonTrivialClass &obj);
