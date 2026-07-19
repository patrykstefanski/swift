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
