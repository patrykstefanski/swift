#ifndef TEST_INTEROP_CXX_IMPL_FRT_OVERLOAD_H
#define TEST_INTEROP_CXX_IMPL_FRT_OVERLOAD_H

#define SWIFT_SHARED_REFERENCE(_retain, _release)                              \
  __attribute__((swift_attr("import_reference")))                              \
  __attribute__((swift_attr("retain:" #_retain)))                             \
  __attribute__((swift_attr("release:" #_release)))

struct Calc;
void Calc_retain(Calc *_Nonnull);
void Calc_release(Calc *_Nonnull);

// Foreign-reference type with a *same-arity overload set* whose members differ
// by parameter type (not const-ness): both import and are disambiguated by the
// Swift signature, exactly as for free functions / value-type records. (A
// const/non-const pair with identical parameters, by contrast, cannot be
// disambiguated for an FRT and is diagnosed -- see
// decl/ext/cxx_implementation_foreign_reference.swift.)
struct Calc {
  int base;
  int combine(int x) const;        // -> _ZNK4Calc7combineEi
  double combine(double x) const;  // -> _ZNK4Calc7combineEd
} SWIFT_SHARED_REFERENCE(Calc_retain, Calc_release);

Calc *_Nonnull makeCalc(int v);  // factory, defined in the C++ main

#endif
