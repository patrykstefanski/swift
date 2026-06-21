int CxxImplFunc1(int param);
int CxxImplFuncMismatch1(int param);
int CxxImplFuncMismatch2(int param);
int CxxImplFuncNameMismatch1(int param);
int CxxImplFuncNameMismatch2(int param);

__attribute__((swift_name("CxxImplFuncRenamed_Swift(arg:)")))
int CxxImplFuncRenamed_Cxx(int param);

// Same-arity overloads (ambiguous Swift name)
int sameArityOverload(int x);
double sameArityOverload(double x);

// Two overloads that import to the *same* Swift signature ((Int32) -> Int32):
// distinct in C++ (by value vs const reference) but indistinguishable in Swift.
// Used to exercise the ambiguous-overload diagnostic.
int ambiguousOverload(int x);
int ambiguousOverload(const int &x);

// Value-type record with const/non-const method overloads (same parameters).
// Distinguished by `mutating`: interop maps the const method to a non-mutating
// Swift method and the non-const method to a `mutating` one.
struct Counter {
  int adjust(int x) const;   // const     -> _ZNK7Counter6adjustEi
  int adjust(int x);         // non-const -> _ZN7Counter6adjustEi
};
