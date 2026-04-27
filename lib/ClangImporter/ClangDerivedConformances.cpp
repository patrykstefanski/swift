//===--- ClangDerivedConformances.cpp -------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2022 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "ClangDerivedConformances.h"
#include "ClangSynthesizedDecls.h"
#include "ImporterImpl.h"
#include "SwiftLookupTable.h"
#include "swift/AST/ConformanceLookup.h"
#include "swift/AST/Identifier.h"
#include "swift/AST/KnownProtocols.h"
#include "swift/AST/LayoutConstraint.h"
#include "swift/AST/ParameterList.h"
#include "swift/AST/PrettyStackTrace.h"
#include "swift/AST/ProtocolConformance.h"
#include "swift/Basic/Assertions.h"
#include "swift/ClangImporter/ClangImporter.h"
#include "swift/ClangImporter/ClangImporterRequests.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Sema/DelayedDiagnostic.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Overload.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"

using namespace swift;
using namespace swift::importer;

/// Known C++ stdlib types, for which we can assume conformance to the standard
/// (e.g., std::map has template parameters for key and value types, and has
/// members like key_type, size_type, and operator[]).
enum class CxxStdType {
  uncategorized,
  optional,
  set,
  unordered_set,
  multiset,
  pair,
  map,
  unordered_map,
  multimap,
  vector,
  span,
};

static CxxStdType identifyCxxStdTypeByName(StringRef name) {
#define CaseStd(name) Case(#name, CxxStdType::name)
  return llvm::StringSwitch<CxxStdType>(name)
      .CaseStd(optional)
      .CaseStd(set)
      .CaseStd(unordered_set)
      .CaseStd(multiset)
      .CaseStd(pair)
      .CaseStd(map)
      .CaseStd(unordered_map)
      .CaseStd(multimap)
      .CaseStd(vector)
      .CaseStd(span)
      .Default(CxxStdType::uncategorized);
#undef CxxStdCase
}

static const clang::TypeDecl *
lookupCxxTypeMember(clang::Sema &Sema, const clang::CXXRecordDecl *Rec,
                    StringRef name, bool mustBeComplete = false) {
  auto R = clang::LookupResult(Sema, &Sema.PP.getIdentifierTable().get(name),
                               clang::SourceLocation(),
                               clang::Sema::LookupMemberName);
  R.suppressDiagnostics();
  Sema.LookupQualifiedName(R, const_cast<clang::CXXRecordDecl *>(Rec));

  if (!R.isSingleResult())
    return nullptr; // Result was absent or ambiguous

  auto it = R.begin();
  if (it->getAccess() != clang::AS_public)
    return nullptr; // Not accessible from outside of the class

  auto *td = dyn_cast<clang::TypeDecl>(it.getDecl());
  if (!td)
    return nullptr; // Was not a clang::TypeDecl

  if (mustBeComplete &&
      !Sema.isCompleteType({}, td->getASTContext().getTypeDeclType(td)))
    return nullptr;

  return td;
}

static std::pair<clang::CXXMethodDecl *, clang::CXXMethodDecl *>
lookupCxxZeroArityMethod(clang::Sema &Sema, const clang::CXXRecordDecl *Rec,
                         StringRef name) {
  auto R = clang::LookupResult(Sema, &Sema.PP.getIdentifierTable().get(name),
                               clang::SourceLocation(),
                               clang::Sema::LookupMemberName);
  R.suppressDiagnostics();
  Sema.LookupQualifiedName(R, const_cast<clang::CXXRecordDecl *>(Rec));

  if (!R.isSingleResult() && !R.isOverloadedResult())
    return {nullptr, nullptr};

  clang::CXXMethodDecl *constMethod = nullptr, *mutMethod = nullptr;
  for (auto it = R.begin(); it != R.end(); ++it) {
    if (it.getAccess() != clang::AS_public)
      continue; // not public

    auto *cmd = dyn_cast<clang::CXXMethodDecl>(it.getDecl());
    if (!cmd)
      continue; // not a method

    if (cmd->getMinRequiredArguments() != 0)
      continue; // not 0-arity

    if (cmd->isVolatile() ||
        cmd->getRefQualifier() == clang::RefQualifierKind::RQ_RValue)
      continue; // volatile and r-value ref overload (not supported)

    if (cmd->isConst()) {
      if (constMethod)
        return {nullptr, nullptr};
      constMethod = cmd;
    } else {
      if (mutMethod)
        return {nullptr, nullptr};
      mutMethod = cmd;
    }
  }
  return {constMethod, mutMethod};
}

/// Alternative to `NominalTypeDecl::lookupDirect`.
/// This function does not attempt to load extensions of the nominal decl.
static TinyPtrVector<ValueDecl *>
lookupDirectWithoutExtensions(NominalTypeDecl *decl, Identifier id) {
  ASTContext &ctx = decl->getASTContext();
  auto *importer = static_cast<ClangImporter *>(ctx.getClangModuleLoader());

  TinyPtrVector<ValueDecl *> result;

  if (id.isOperator()) {
    auto underlyingId = getOperatorName(ctx, id);
    TinyPtrVector<ValueDecl *> underlyingFuncs = evaluateOrDefault(
        ctx.evaluator, ClangRecordMemberLookup({decl, underlyingId}), {});
    for (auto it : underlyingFuncs) {
      if (auto synthesizedFunc =
              importer->getCXXSynthesizedOperatorFunc(cast<FuncDecl>(it)))
        result.push_back(synthesizedFunc);
    }
  } else {
    // See if there is a Clang decl with the given name.
    result = evaluateOrDefault(ctx.evaluator,
                               ClangRecordMemberLookup({decl, id}), {});
  }

  // Check if there are any synthesized Swift members that match the name.
  for (auto member : decl->getCurrentMembersWithoutLoading()) {
    if (auto namedMember = dyn_cast<ValueDecl>(member)) {
      if (namedMember->hasName() && !namedMember->getName().isSpecial() &&
          namedMember->getName().getBaseIdentifier().is(id.str()) &&
          // Make sure we don't add duplicate entries, as that would wrongly
          // imply that lookup is ambiguous.
          !llvm::is_contained(result, namedMember)) {
        result.push_back(namedMember);
      }
    }
  }
  return result;
}

template <typename Decl>
static Decl *lookupDirectSingleWithoutExtensions(NominalTypeDecl *decl,
                                                 Identifier id) {
  auto results = lookupDirectWithoutExtensions(decl, id);
  if (results.size() != 1)
    return nullptr;
  return dyn_cast<Decl>(results.front());
}

static bool isValidEqualEqualOp(const NominalTypeDecl *decl,
                                const ValueDecl *imported) {
  auto equalEqual = dyn_cast<FuncDecl>(imported);
  if (!equalEqual)
    return false;
  auto params = equalEqual->getParameters();
  if (params->size() != 2)
    return false;
  auto lhs = params->get(0);
  auto rhs = params->get(1);
  if (lhs->isInOut() || rhs->isInOut())
    return false;
  auto lhsTy = lhs->getTypeInContext();
  auto rhsTy = rhs->getTypeInContext();
  if (!lhsTy || !rhsTy)
    return false;
  auto lhsNominal = lhsTy->getAnyNominal();
  auto rhsNominal = rhsTy->getAnyNominal();
  return lhsNominal == rhsNominal && lhsNominal == decl;
}

static bool isValidMinusOp(const NominalTypeDecl *decl,
                           const ValueDecl *imported) {
  auto minus = dyn_cast<FuncDecl>(imported);
  if (!minus)
    return false;
  auto params = minus->getParameters();
  if (params->size() != 2)
    return false;
  auto lhs = params->get(0);
  auto rhs = params->get(1);
  if (lhs->isInOut() || rhs->isInOut())
    return false;
  auto lhsTy = lhs->getTypeInContext();
  auto rhsTy = rhs->getTypeInContext();
  if (!lhsTy || !rhsTy)
    return false;
  auto lhsNominal = lhsTy->getAnyNominal();
  auto rhsNominal = rhsTy->getAnyNominal();
  if (lhsNominal != rhsNominal || lhsNominal != decl)
    return false;
  auto returnTy = minus->getResultInterfaceType();
  auto binaryIntegerProto =
      decl->getASTContext().getProtocol(KnownProtocolKind::BinaryInteger);
  return static_cast<bool>(checkConformance(returnTy, binaryIntegerProto));
}

static bool isValidPlusEqualOp(const NominalTypeDecl *decl, Type distanceTy,
                               const ValueDecl *imported) {
  auto plusEqual = dyn_cast<FuncDecl>(imported);
  if (!plusEqual)
    return false;
  auto params = plusEqual->getParameters();
  if (params->size() != 2)
    return false;
  auto lhs = params->get(0);
  auto rhs = params->get(1);
  if (rhs->isInOut())
    return false;
  auto lhsTy = lhs->getTypeInContext();
  auto rhsTy = rhs->getTypeInContext();
  if (!lhsTy || !rhsTy)
    return false;
  if (rhsTy->getCanonicalType() != distanceTy->getCanonicalType())
    return false;
  auto lhsNominal = lhsTy->getAnyNominal();
  if (lhsNominal != decl)
    return false;
  auto returnTy = plusEqual->getResultInterfaceType();
  return returnTy->isVoid();
}

namespace {
/// Result of finding or synthesizing a C++ binary operator for a record.
struct ResolvedCxxOperator {
  /// The operator as a Swift decl in the free-operator shape, or null if no
  /// usable operator was found.
  ValueDecl *swiftDecl = nullptr;
  /// The Clang function whose return type is the operator's return type
  /// (the resolved callee, or the forwarding wrapper if one was used).
  const clang::FunctionDecl *clangFunction = nullptr;

  explicit operator bool() const { return swiftDecl != nullptr; }
};
} // namespace

/// Find a C++ binary operator for \p classDecl and import it as a Swift
/// ValueDecl. Runs Clang overload resolution once, covering member and
/// non-member operators (free functions, hidden friends, and template
/// instantiations). If the resolved operator doesn't pass \p isValid (e.g. it
/// is defined on a base class), the resolved call is wrapped in a synthesized
/// forwarding operator with the given operand types and a return type from
/// \p makeReturnTy. Successful resolutions are cached by (operator, operand
/// types); repeated queries return the same declaration, which
/// deserialization relies on when re-resolving cross-referenced operators.
static ResolvedCxxOperator findOrSynthesizeCxxOperator(
    ClangImporter::Implementation &impl,
    const clang::CXXRecordDecl *classDecl,
    clang::BinaryOperatorKind operatorKind, clang::QualType lhsTy,
    clang::QualType rhsTy,
    function_ref<clang::QualType(const clang::FunctionDecl *op)> makeReturnTy,
    function_ref<bool(ValueDecl *)> isValid) {
  clang::ASTContext &clangCtx = classDecl->getASTContext();
  clang::Sema &clangSema = impl.getClangSema();

  auto overloadedOpKind =
      clang::BinaryOperator::getOverloadedOperator(operatorKind);
  const char *opSpelling = clang::getOperatorSpelling(overloadedOpKind);
  auto opName = clang::DeclarationName(&clangCtx.Idents.get(opSpelling));

  // 1. Check the cache. Synthesized forwarding wrappers live in no Clang
  // DeclContext, so neither member lookup nor ADL could rediscover them; the
  // cache is what makes repeated queries (including deserialization's
  // re-resolution) converge on the one declaration created for this key.
  std::tuple<unsigned, const clang::Type *, const clang::Type *> cacheKey{
      static_cast<unsigned>(operatorKind),
      clangCtx.getCanonicalType(lhsTy).getTypePtr(),
      clangCtx.getCanonicalType(rhsTy).getTypePtr()};
  auto cached = impl.resolvedCxxOperators.find(cacheKey);
  if (cached != impl.resolvedCxxOperators.end())
    return {cached->second.first, cached->second.second};

  auto cacheAndReturn =
      [&](ValueDecl *swiftDecl,
          const clang::FunctionDecl *fn) -> ResolvedCxxOperator {
    impl.resolvedCxxOperators.try_emplace(cacheKey, swiftDecl, fn);
    return {swiftDecl, fn};
  };

  // 2. Run Clang overload resolution once to find the best viable operator.
  // This covers member and non-member operators (free functions, hidden
  // friends, member operators, template instantiations). The operands are
  // references to freshly created parameter declarations, so the resulting
  // call expression can double as the body of a forwarding wrapper if one
  // turns out to be needed below. Both operands are modifiable lvalues: the
  // left operand must be one so mutating member operators (e.g.
  // `T &operator+=(difference_type)`) remain viable, and both match what a
  // wrapper actually forwards. A compound-assignment wrapper takes its left
  // operand by reference so it mutates the caller's value rather than a copy
  // (and imports into Swift with an `inout` parameter).
  if (clang::BinaryOperator::isCompoundAssignmentOp(operatorKind))
    lhsTy = clangCtx.getLValueReferenceType(lhsTy);

  auto *tu = clangCtx.getTranslationUnitDecl();
  auto *lhsParam = createClangParmVarDecl(clangCtx, tu, nullptr, lhsTy);
  auto *rhsParam = createClangParmVarDecl(clangCtx, tu, nullptr, rhsTy);
  auto *lhsRef = createClangDeclRefExpr(
      clangCtx, lhsParam, lhsTy.getNonReferenceType(), clang::VK_LValue);
  auto *rhsRef = createClangDeclRefExpr(
      clangCtx, rhsParam, rhsTy.getNonReferenceType(), clang::VK_LValue);

  // Resolution failures (no viable candidate, ambiguity, deleted function)
  // emit hard errors; trap them so they only mean "no operator found".
  // Availability and deprecation diagnostics for the winning candidate are
  // delayed; capture them without emitting. Rewritten C++20 candidates
  // (e.g. reversed `operator==`) are not considered, so the callee is always
  // the operator as spelled.
  clang::UnresolvedSet<1> noFns;
  clang::Sema::SFINAETrap trap(clangSema);
  clang::sema::DelayedDiagnosticPool diagPool{
      clangSema.DelayedDiagnostics.getCurrentPool()};
  auto diagState = clangSema.DelayedDiagnostics.push(diagPool);
  clang::ExprResult callResult = clangSema.CreateOverloadedBinOp(
      classDecl->getLocation(), operatorKind, noFns, lhsRef, rhsRef,
      /*RequiresADL=*/true, /*AllowRewrittenCandidates=*/false);
  clangSema.DelayedDiagnostics.popWithoutEmitting(diagState);

  if (trap.hasErrorOccurred() || !callResult.isUsable())
    return {};

  // If a builtin operator was selected (the result is a plain BinaryOperator),
  // there is no function to import.
  auto *call =
      dyn_cast<clang::CXXOperatorCallExpr>(callResult.get()->IgnoreImplicit());
  if (!call)
    return {};
  clang::FunctionDecl *clangCallee = call->getDirectCallee();
  if (!clangCallee)
    return {};
  impl.registerSynthesizedClangDecl(clangCallee, classDecl);
  if (impl.isUnavailableInSwift(clangCallee))
    return {};

  if (auto *imported = dyn_cast_or_null<ValueDecl>(
          impl.importDecl(clangCallee, impl.CurrentVersion))) {
    // A non-member operator imports as a free Swift operator that matches the
    // shape the validators expect (and retains its Clang decl, which callers
    // inspect -- e.g. to read a difference_type off `operator-`). Prefer it.
    if (isValid(imported))
      return cacheAndReturn(imported, clangCallee);
    // A *member* operator instead imports as an `__operator...` method, where
    // the left operand is `self` rather than an explicit parameter. That fails
    // validation which expects the free-operator shape (e.g. `isValidPlusEqualOp`
    // wants two parameters). Swift attaches the matching free operator as a
    // synthesized alternate; use that.
    if (auto *fd = dyn_cast<FuncDecl>(imported)) {
      auto *importer = static_cast<ClangImporter *>(
          impl.SwiftContext.getClangModuleLoader());
      if (auto *synthesizedOperator =
              importer->getCXXSynthesizedOperatorFunc(fd))
        if (isValid(synthesizedOperator))
          return cacheAndReturn(synthesizedOperator, clangCallee);
    }
  }

  // 3. Resolution found a viable operator but isValid rejected it (e.g. it's
  // defined on a base class). Wrap the already-built call in a forwarding
  // operator with the derived type. Don't synthesize a wrapper if building
  // the call produced delayed diagnostics (e.g. the underlying operator is
  // unavailable for the current target).
  if (!diagPool.empty())
    return {};

  // The wrapper's decl context is the translation unit; otherwise, the
  // operator might get imported as a static member function of a different
  // type (e.g. an operator declared inside of a C++ namespace would get
  // imported as a member function of a Swift enum), which would make the
  // operator un-discoverable to Swift name lookup.
  //
  // A compound-assignment wrapper returns void, discarding the reference the
  // underlying operator returns: its Swift form must return Void (that is the
  // shape `isValidPlusEqualOp` and the iterator protocols require).
  bool isCompoundAssign =
      clang::BinaryOperator::isCompoundAssignmentOp(operatorKind);
  clang::QualType returnTy =
      isCompoundAssign ? clangCtx.VoidTy : makeReturnTy(clangCallee);
  auto wrapperTy = clangCtx.getFunctionType(
      returnTy, {lhsTy, rhsTy}, clang::FunctionProtoType::ExtProtoInfo());
  auto *wrapper = createClangFunctionDecl(clangCtx, tu, opName, wrapperTy);
  wrapper->setAccess(clang::AccessSpecifier::AS_public);
  // The parameters were created ahead of the wrapper (their references form
  // the body); reparent them, as Sema::ActOnFunctionDeclarator does.
  lhsParam->setDeclContext(wrapper);
  rhsParam->setDeclContext(wrapper);
  wrapper->setParams({lhsParam, rhsParam});
  clang::Stmt *body;
  if (isCompoundAssign)
    body = clang::CompoundStmt::Create(
        clangCtx, {callResult.get()}, clang::FPOptionsOverride(),
        clang::SourceLocation(), clang::SourceLocation());
  else
    body = createClangReturnStmt(clangCtx, callResult.get());
  wrapper->setBody(body);
  impl.registerSynthesizedClangDecl(wrapper, classDecl);

  if (auto *imported = dyn_cast_or_null<ValueDecl>(
          impl.importDecl(wrapper, impl.CurrentVersion)))
    if (isValid(imported))
      return cacheAndReturn(imported, wrapper);

  return {};
}

/// Find or synthesize `operator==` comparing two values of \p clangDecl's
/// type, returning bool.
static ResolvedCxxOperator
findCxxEqualEqualOperator(ClangImporter::Implementation &impl,
                          NominalTypeDecl *decl,
                          const clang::CXXRecordDecl *clangDecl) {
  clang::ASTContext &clangCtx = clangDecl->getASTContext();
  clang::QualType recordTy = clangCtx.getRecordType(clangDecl);
  auto returnTy = [boolTy = clangCtx.BoolTy](const clang::FunctionDecl *) {
    return boolTy;
  };
  auto isValid = [decl](ValueDecl *imported) {
    return isValidEqualEqualOp(decl, imported);
  };
  return findOrSynthesizeCxxOperator(impl, clangDecl,
                                     clang::BinaryOperatorKind::BO_EQ,
                                     recordTy, recordTy, returnTy, isValid);
}

/// Find or synthesize `operator-` subtracting two values of \p clangDecl's
/// type. Its return type is the iterator's difference_type; the result's
/// clangFunction carries it for `findCxxPlusEqualOperator`.
static ResolvedCxxOperator
findCxxMinusOperator(ClangImporter::Implementation &impl, NominalTypeDecl *decl,
                     const clang::CXXRecordDecl *clangDecl) {
  clang::QualType recordTy =
      clangDecl->getASTContext().getRecordType(clangDecl);
  auto returnTy = [](const clang::FunctionDecl *fd) {
    return fd->getReturnType();
  };
  auto isValid = [decl](ValueDecl *imported) {
    return isValidMinusOp(decl, imported);
  };
  return findOrSynthesizeCxxOperator(impl, clangDecl,
                                     clang::BinaryOperatorKind::BO_Sub,
                                     recordTy, recordTy, returnTy, isValid);
}

/// Find or synthesize `operator+=` advancing a value of \p clangDecl's type
/// by the iterator's difference_type. Requires the already-found `operator-`
/// (\p minusOp): its Clang return type is the right operand's type, and
/// \p distanceTy (its imported Swift result type) is what the right operand
/// must match.
static ResolvedCxxOperator
findCxxPlusEqualOperator(ClangImporter::Implementation &impl,
                         NominalTypeDecl *decl,
                         const clang::CXXRecordDecl *clangDecl,
                         const ResolvedCxxOperator &minusOp, Type distanceTy) {
  clang::QualType recordTy =
      clangDecl->getASTContext().getRecordType(clangDecl);
  // The right operand of `operator+=` is the iterator's difference_type, i.e.
  // the result type of `operator-`. Read it from the Clang `operator-` rather
  // than from the imported Swift decl: when `operator-` is a member operator,
  // the Swift decl is a synthesized alternate with no usable Clang decl, and
  // the iterator's difference_type may come from an iterator_traits
  // specialization rather than a member typedef.
  clang::QualType rhsTy = minusOp.clangFunction->getReturnType();
  auto returnTy = [](const clang::FunctionDecl *fd) {
    return fd->getReturnType();
  };
  auto isValid = [decl, distanceTy](ValueDecl *imported) {
    return isValidPlusEqualOp(decl, distanceTy, imported);
  };
  return findOrSynthesizeCxxOperator(impl, clangDecl,
                                     clang::BinaryOperatorKind::BO_AddAssign,
                                     recordTy, rhsTy, returnTy, isValid);
}

void swift::simple_display(llvm::raw_ostream &out,
                           const CxxRecordDeclDescriptor &desc) {
  out << "Inferring C++ iterator info for '";
  if (desc.decl->getIdentifier())
    out << desc.decl->getName();
  else if (desc.decl->isAnonymousStructOrUnion())
    out << "(anonymous record)";
  else
    out << "(unnamed record)";
  out << "'\n";
}

SourceLoc
swift::extractNearestSourceLoc(const CxxRecordDeclDescriptor &desc) {
  return SourceLoc();
}

static std::optional<CxxIteratorCategory>
categorizeIteratorFromTypeTag(const clang::TypeDecl *tyDecl) {

  const clang::CXXRecordDecl *underlyingDecl = nullptr;
  if (auto typedefDecl = dyn_cast<clang::TypedefNameDecl>(tyDecl))
    // Typical case: `using iterator_tag = some_tag;`
    underlyingDecl = typedefDecl->getUnderlyingType()
                         .getCanonicalType()
                         ->getAsCXXRecordDecl();
  else
    // Less common: `struct iterator_tag : some_tag {};`
    underlyingDecl = dyn_cast<clang::CXXRecordDecl>(tyDecl);

  if (underlyingDecl)
    underlyingDecl = underlyingDecl->getDefinition();
  if (!underlyingDecl)
    return {};

  std::optional<CxxIteratorCategory> category = std::nullopt;

  // In the easiest case, the underlyingDecl is the iterator category tag from
  // the std namespace, but it might also be some record that inherits from one
  // of the std iterator tags. Look through all of its (public) bases and find
  // the strongest iterator category.

  llvm::SmallVector<const clang::CXXRecordDecl *, 2> queue;
  queue.push_back(underlyingDecl);

  while (!queue.empty()) {
    auto *decl = queue.back();
    queue.pop_back();

    auto matchedTag = false;
    if (decl->isInStdNamespace() && decl->getIdentifier()) {
      auto declCategory =
          llvm::StringSwitch<std::optional<CxxIteratorCategory>>(
              decl->getName())
              .Case("input_iterator_tag", CxxIteratorCategory::Input)
              .Case("random_access_iterator_tag",
                    CxxIteratorCategory::RandomAccess)
              .Case("contiguous_iterator_tag", CxxIteratorCategory::Contiguous)
              .Default(std::nullopt);

      if (declCategory.has_value()) {
        matchedTag = true;

        if (!category.has_value() || category.value() < declCategory.value())
          // This is one of the std category tags, and is stronger than what
          // we've previously seen
          category = declCategory;
      }
    }

    if (!matchedTag) {
      // We only need to explore bases if this isn't a std iterator category tag
      for (auto base : decl->bases()) {
        if (base.getAccessSpecifier() != clang::AS_public)
          // Simply skip over any non-public base
          continue;

        auto *ty = base.getType()->getAs<clang::RecordType>();
        if (!ty)
          // Bail if we encounter some kind of base that isn't a record type
          return {};

        auto *baseDecl = dyn_cast_or_null<clang::CXXRecordDecl>(
            ty->getDecl()->getDefinition());
        if (!baseDecl || (baseDecl->isDependentContext() &&
                          !baseDecl->isCurrentInstantiation(decl)))
          // Bail if we encounter a base that isn't a defined C++ record
          return {};

        // Look at this base later
        queue.push_back(baseDecl);
      }
    }
  }
  return category;
}

std::optional<CxxIteratorCategory>
CxxIteratorInfoRequest::evaluate(Evaluator &evaluator,
                                 CxxRecordDeclDescriptor desc) const {
  auto *decl = desc.decl;
  auto &sema = desc.sema;
  auto &ctx = decl->getASTContext();

  // Look for a non-primary specialization of std::iterator_traits<decl>.
  //
  // If one exists, use its iterator tag, but don't try to instantiate
  // a specialization if there isn't already one.
  if (auto *stdNS = sema.getStdNamespace()) {
    auto iteratorTraitsId =
        ctx.DeclarationNames.getIdentifier(&ctx.Idents.get("iterator_traits"));
    if (auto *iterator_traits = stdNS->lookup(iteratorTraitsId)
                                    .find_first<clang::ClassTemplateDecl>()) {
      void *insertPos = nullptr; // unused
      if (auto *traitSpecialization = iterator_traits->findSpecialization(
              {clang::TemplateArgument(ctx.getTypeDeclType(decl))}, insertPos);
          traitSpecialization && traitSpecialization->hasDefinition()) {
        if (traitSpecialization->isExplicitSpecialization()) {
          // Determine info from definition of iterator_traits specialization,
          // but only if it is a non-primary specialization.
          //
          // N.B. decl is ITER_TRAITS from [iterator.concepts.general]
          decl = traitSpecialization->getDefinition();
        }
      }
    }
  }

  if (sema.getLangOpts().CPlusPlus20) {
    // Only look for iterator_concept if we are using C++20 or above.
    auto *conceptDecl = lookupCxxTypeMember(sema, decl, "iterator_concept");
    if (conceptDecl)
      return categorizeIteratorFromTypeTag(conceptDecl);

    // iterator_concept should take precedence, but if it is absent, fallback
    // to iterator_category.
  }

  if (auto *categoryDecl = lookupCxxTypeMember(sema, decl, "iterator_category")) {
    auto info = categorizeIteratorFromTypeTag(categoryDecl);
    if (info == CxxIteratorCategory::Contiguous)
      info = CxxIteratorCategory::RandomAccess;
    return info;
  }

  return {};
}

bool swift::hasIteratorCategory(const clang::CXXRecordDecl *clangDecl) {
  clang::IdentifierInfo *name =
      &clangDecl->getASTContext().Idents.get("iterator_category");
  auto members = clangDecl->lookup(name);
  if (members.empty())
    return false;
  // NOTE: If this is a templated typedef, Clang might have instantiated
  // several equivalent typedef decls, so members.isSingleResult() may
  // return false here. But if they aren't equivalent, Clang should have
  // already complained about this. Let's assume that they are equivalent.
  // (see filterNonConflictingPreviousTypedefDecls in clang/Sema/SemaDecl.cpp)
  return isa<clang::TypeDecl>(members.front());
}

ValueDecl *ClangImporter::getCxxSynthesizedConformanceOperator(
    const DeclBaseName &name, NominalTypeDecl *selfType) {
  assert(name.isOperator());

  auto *clangDecl =
      dyn_cast_or_null<clang::CXXRecordDecl>(selfType->getClangDecl());
  if (!clangDecl)
    return nullptr;

  // Operators are found or synthesized via Clang overload resolution and may
  // not be serialized as members, so they must be re-resolved when
  // deserialized. Resolution is deterministic (and cached), so this
  // reproduces the operator the serialized module referenced.
  //
  // The operators the Cxx iterator conformances synthesize (==, -, +=) are
  // re-resolved with the exact recipes live derivation uses, so a
  // deserialized reference binds the same declaration the conformance
  // produced.

  if (name.getIdentifier() == selfType->getASTContext().Id_EqualsOperator)
    return findCxxEqualEqualOperator(Impl, selfType, clangDecl).swiftDecl;

  if (name.getIdentifier() == selfType->getASTContext().getIdentifier("-"))
    return findCxxMinusOperator(Impl, selfType, clangDecl).swiftDecl;

  if (name.getIdentifier() == selfType->getASTContext().getIdentifier("+=")) {
    auto minusOp = findCxxMinusOperator(Impl, selfType, clangDecl);
    auto *minus = dyn_cast_or_null<FuncDecl>(minusOp.swiftDecl);
    if (!minus)
      return nullptr;
    return findCxxPlusEqualOperator(Impl, selfType, clangDecl, minusOp,
                                    minus->getResultInterfaceType())
        .swiftDecl;
  }

  return nullptr;
}

static void
conformToCxxIteratorIfNeeded(ClangImporter::Implementation &impl,
                             NominalTypeDecl *decl,
                             const clang::CXXRecordDecl *clangDecl) {
  static_assert(
      CxxIteratorCategory::Input < CxxIteratorCategory::RandomAccess &&
      CxxIteratorCategory::RandomAccess < CxxIteratorCategory::Contiguous);

  PrettyStackTraceDecl trace("trying to conform to UnsafeCxxInputIterator", decl);
  ASTContext &ctx = decl->getASTContext();
  clang::Sema &clangSema = impl.getClangSema();

  if (!ctx.getProtocol(KnownProtocolKind::UnsafeCxxInputIterator) ||
      !ctx.getProtocol(KnownProtocolKind::UnsafeCxxMutableInputIterator))
    return; // Don't even bother if we don't have protocols for input iterators

  // FIXME: hasIteratorCategory() is more conservative than it should be because
  // it doesn't consider an inherited iterator_category, nor iterator_concept.
  // For now, checking this maintains existing behavior and ensures consistency
  // across ClangImporter, where clang::Sema isn't always readily available.

  auto iterInfo = evaluateOrDefault(
      ctx.evaluator, CxxIteratorInfoRequest({clangDecl, clangSema}), {});

  if (!iterInfo.has_value())
    return;

  auto category = iterInfo.value();
  ASSERT(category >= CxxIteratorCategory::Input);

  // Input iterators require:
  //   - operator*()  / .pointee
  //   - operator++() / .successor()
  //   - operator==() / func ==(lhs:rhs)

  auto [pointee, operatorStar, _] =
      impl.lookupAndImportPointeeAndOperatorStar(decl);
  if (!pointee || pointee->isGetterMutating() ||
      pointee->getTypeInContext()->hasError())
    return;

  auto *successor = impl.lookupAndImportSuccessor(decl);
  if (!successor || successor->isMutating())
    return;
  auto successorTy = successor->getResultInterfaceType();
  if (!successorTy || successorTy->getAnyNominal() != decl)
    return;

  // Check if present: `func ==`
  auto equalEqual = findCxxEqualEqualOperator(impl, decl, clangDecl);
  if (!equalEqual)
    return;

  Type pointeeTy = pointee->getTypeInContext();

  // Look for __operatorStar(), which must be non-mutating and return a
  // reference. This makes sure we use the const operator* overload.
  Type dereferenceResultTy = pointeeTy;
  if (operatorStar && !operatorStar->isMutating()) {
    auto operatorStarReturnTy = operatorStar->getResultInterfaceType();
    assert(operatorStarReturnTy &&
           "__operatorStar doesn't have a return type?");

    if (operatorStarReturnTy &&
        operatorStarReturnTy->getAnyPointerElementType() &&
        (operatorStarReturnTy->getAnyPointerElementType()->getCanonicalType() ==
         pointeeTy->getCanonicalType()))
      dereferenceResultTy = operatorStar->getResultInterfaceType();
  }
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Pointee"), pointeeTy);
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("DereferenceResult"),
                               dereferenceResultTy);

  bool mutableIterator = pointee->isSettable(nullptr);

  if (mutableIterator)
    impl.addSynthesizedProtocolAttrs(
        decl, {KnownProtocolKind::UnsafeCxxMutableInputIterator});
  else
    impl.addSynthesizedProtocolAttrs(
        decl, {KnownProtocolKind::UnsafeCxxInputIterator});

  if (category < CxxIteratorCategory::RandomAccess ||
      !ctx.getProtocol(KnownProtocolKind::UnsafeCxxRandomAccessIterator) ||
      !ctx.getProtocol(KnownProtocolKind::UnsafeCxxMutableRandomAccessIterator))
    return;

  // Random access iterators additionally require:
  //   - operator-()  / func -(lhs:rhs)
  //   - operator+=() / func +=(lhs:rhs)

  // Check if present: `func -`
  auto minusOp = findCxxMinusOperator(impl, decl, clangDecl);
  auto minus = dyn_cast_or_null<FuncDecl>(minusOp.swiftDecl);
  if (!minus)
    return;

  // distanceTy conforms to BinaryInteger, this is ensured by isValidMinusOp.
  auto distanceTy = minus->getResultInterfaceType();

  // Check if present: `func +=`
  auto plusEqualOp =
      findCxxPlusEqualOperator(impl, decl, clangDecl, minusOp, distanceTy);
  auto plusEqual = dyn_cast_or_null<FuncDecl>(plusEqualOp.swiftDecl);
  if (!plusEqual)
    return;

  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Distance"), distanceTy);
  if (mutableIterator)
    impl.addSynthesizedProtocolAttrs(
        decl, {KnownProtocolKind::UnsafeCxxMutableRandomAccessIterator});
  else
    impl.addSynthesizedProtocolAttrs(
        decl, {KnownProtocolKind::UnsafeCxxRandomAccessIterator});

  if (category < CxxIteratorCategory::Contiguous ||
      !ctx.getProtocol(KnownProtocolKind::UnsafeCxxContiguousIterator) ||
      !ctx.getProtocol(KnownProtocolKind::UnsafeCxxMutableContiguousIterator))
    return;

  // Contiguous iterators do not have any additional requirements

  if (mutableIterator)
    impl.addSynthesizedProtocolAttrs(
        decl, {KnownProtocolKind::UnsafeCxxMutableContiguousIterator});
  else
    impl.addSynthesizedProtocolAttrs(
        decl, {KnownProtocolKind::UnsafeCxxContiguousIterator});
}

static void
conformToCxxConvertibleToBoolIfNeeded(ClangImporter::Implementation &impl,
                                      swift::NominalTypeDecl *decl) {
  PrettyStackTraceDecl trace("trying to conform to CxxConvertibleToBool", decl);
  if (impl.lookupAndImportOperatorBool(decl))
    impl.addSynthesizedProtocolAttrs(decl,
                                     {KnownProtocolKind::CxxConvertibleToBool});
}

static void conformToCxxOptional(ClangImporter::Implementation &impl,
                                 NominalTypeDecl *decl,
                                 const clang::CXXRecordDecl *clangDecl) {
  PrettyStackTraceDecl trace("conforming to CxxOptional", decl);
  ASTContext &ctx = decl->getASTContext();
  clang::ASTContext &clangCtx = impl.getClangASTContext();
  clang::Sema &clangSema = impl.getClangSema();

  auto *value_type = lookupCxxTypeMember(clangSema, clangDecl, "value_type",
                                         /*mustBeComplete=*/true);
  if (!value_type)
    return;

  auto *Wrapped = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(value_type, impl.CurrentVersion));
  if (!Wrapped)
    return;

  auto pointee = impl.lookupAndImportPointee(decl);
  if (!pointee)
    return;

  // `std::optional` has a C++ constructor that takes the wrapped value as a
  // parameter. Unfortunately this constructor has templated parameter type, so
  // it isn't directly usable from Swift. Let's explicitly instantiate a
  // constructor with the wrapped value type, and then import it into Swift.

  auto valueType = clangCtx.getTypeDeclType(value_type);

  auto constRefValueType =
      clangCtx.getLValueReferenceType(valueType.withConst());
  // Create a fake variable with type of the wrapped value.
  auto fakeValueVarDecl = clang::VarDecl::Create(
      clangCtx, /*DC*/ clangCtx.getTranslationUnitDecl(),
      clang::SourceLocation(), clang::SourceLocation(), /*Id*/ nullptr,
      constRefValueType, clangCtx.getTrivialTypeSourceInfo(constRefValueType),
      clang::StorageClass::SC_None);
  auto fakeValueRefExpr = new (clangCtx) clang::DeclRefExpr(
      clangCtx, fakeValueVarDecl, false,
      constRefValueType.getNonReferenceType(), clang::ExprValueKind::VK_LValue,
      clangDecl->getLocation());

  auto clangDeclTyInfo = clangCtx.getTrivialTypeSourceInfo(
      clang::QualType(clangDecl->getTypeForDecl(), 0));
  SmallVector<clang::Expr *, 1> constructExprArgs = {fakeValueRefExpr};

  // Instantiate the templated constructor that would accept this fake variable.
  clang::Sema::SFINAETrap trap(clangSema);
  auto constructExprResult = clangSema.BuildCXXTypeConstructExpr(
      clangDeclTyInfo, clangDecl->getLocation(), constructExprArgs,
      clangDecl->getLocation(), /*ListInitialization*/ false);
  if (!constructExprResult.isUsable() || trap.hasErrorOccurred())
    return;

  auto castExpr = dyn_cast_or_null<clang::CastExpr>(constructExprResult.get());
  if (!castExpr)
    return;

  // The temporary bind expression will only be present for some non-trivial C++
  // types.
  auto bindTempExpr =
      dyn_cast_or_null<clang::CXXBindTemporaryExpr>(castExpr->getSubExpr());

  auto constructExpr = dyn_cast_or_null<clang::CXXConstructExpr>(
      bindTempExpr ? bindTempExpr->getSubExpr() : castExpr->getSubExpr());
  if (!constructExpr)
    return;

  auto constructorDecl = constructExpr->getConstructor();

  auto importedConstructor =
      impl.importDecl(constructorDecl, impl.CurrentVersion);
  if (!importedConstructor)
    return;
  decl->addMember(importedConstructor);

  // Mark `var pointee` as deprecated to direct users towards `var value`, which
  // is provided by the Cxx overlay. It supports mutation and is UB-safe. Only
  // do so if `value` is actually available, i.e. if the conformance to
  // CxxOptional was synthesized.
  auto pointeeDeprecatedAttr = AvailableAttr::createUniversallyDeprecated(
      ctx, "use 'value' instead for instances of 'std.optional'", "value");
  pointee->addAttribute(pointeeDeprecatedAttr);

  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Wrapped"),
                               Wrapped->getUnderlyingType());
  impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxOptional});
}

static void conformToCxxIterableIfNeeded(
    ClangImporter::Implementation &impl, NominalTypeDecl *decl,
    const clang::CXXRecordDecl *clangDecl,
    const ProtocolConformance *rawIteratorConformance) {
  PrettyStackTraceDecl trace("trying to conform to CxxIterable", decl);
  ASTContext &ctx = decl->getASTContext();

  ProtocolDecl *cxxIteratorProto =
      ctx.getProtocol(KnownProtocolKind::UnsafeCxxInputIterator);
  ProtocolDecl *cxxIterableProto =
      ctx.getProtocol(KnownProtocolKind::CxxIterable);
  if (!cxxIteratorProto || !cxxIterableProto)
    return;

  // Take the default definition of `BorrowingIterator` from
  // CxxIterable protocol. This type is currently
  // `CxxBorrowingIterator<Self>`.
  auto borrowingIteratorDecl = cxxIterableProto->getAssociatedType(
      ctx.Id_BorrowingIterator);
  if (!borrowingIteratorDecl)
    return;
  auto borrowingIteratorNominal =
      borrowingIteratorDecl->getDefaultDefinitionType()->getAnyNominal();

  // Substitute generic `Self` parameter.
  auto declSelfTy = decl->getDeclaredInterfaceType();
  auto borrowingIteratorTy =
      BoundGenericType::get(borrowingIteratorNominal, Type(), {declSelfTy});

  auto dereferenceResultDecl = cxxIteratorProto->getAssociatedType(
      ctx.getIdentifier("DereferenceResult"));
  if (!dereferenceResultDecl)
    return;

  auto dereferenceResultTy =
      rawIteratorConformance->getTypeWitness(dereferenceResultDecl);

  if (dereferenceResultTy && dereferenceResultTy->getAnyPointerElementType()) {
    // Only conform to CxxIterable if `__operatorStar` returns
    // `UnsafePointer<Pointee>`. Otherwise, we can't create a span for pointee.
    impl.addSynthesizedTypealias(decl, ctx.Id_BorrowingIterator,
                                 borrowingIteratorTy);
    impl.addSynthesizedProtocolAttrs(decl,
                                     {KnownProtocolKind::CxxIterable});
  }
}

static void
conformToCxxSequenceIfNeeded(ClangImporter::Implementation &impl,
                             NominalTypeDecl *decl,
                             const clang::CXXRecordDecl *clangDecl) {
  PrettyStackTraceDecl trace("trying to conform to CxxSequence", decl);
  clang::Sema &clangSema = impl.getClangSema();
  ASTContext &ctx = decl->getASTContext();

  ProtocolDecl *cxxIteratorProto =
      ctx.getProtocol(KnownProtocolKind::UnsafeCxxInputIterator);
  ProtocolDecl *cxxSequenceProto =
      ctx.getProtocol(KnownProtocolKind::CxxSequence);
  // If the Cxx module is missing, or does not include one of the necessary
  // protocols, bail.
  if (!cxxIteratorProto || !cxxSequenceProto)
    return;

  // Look up begin() and end() methods. We only require the const overloads,
  // but will make use of the non-const overloads later, if available, for
  // a possible mutable collection conformance.
  auto [beginConst, beginMut] =
      lookupCxxZeroArityMethod(clangSema, clangDecl, "begin");
  auto [endConst, endMut] =
      lookupCxxZeroArityMethod(clangSema, clangDecl, "end");
  if (!beginConst || !endConst)
    return;

  auto iterTy = beginConst->getReturnType().getCanonicalType();
  if (iterTy != endConst->getReturnType().getCanonicalType())
    // begin() and end() need to have the same return type
    return;

  if (iterTy->isPointerOrReferenceType()) {
    auto pointeeQualType = iterTy->getPointeeType().getCanonicalType();
    if (auto *pointeeRecord = pointeeQualType->getAsCXXRecordDecl()) {
      auto info = evaluateOrDefault(
          ctx.evaluator, ForeignReferenceTypeInfoRequest({pointeeRecord}), {});
      // If begin()/end() return a pointer to an FRT, the pointer will be
      // stripped to the bare FRT class in Swift, which is not a usable iterator
      if (info.isReference())
        return;
    }
  } else {
    // Check if begin() returns an iterator.
    auto *iterDecl = iterTy->getAsCXXRecordDecl();
    if (!iterDecl || !iterDecl->hasDefinition())
      return;
    auto iterInfo = evaluateOrDefault(
        ctx.evaluator, CxxIteratorInfoRequest({iterDecl, clangSema}), {});
    if (!iterInfo.has_value())
      return;
  }

  // import begin() and end()
  auto *begin = dyn_cast_or_null<FuncDecl>(
      impl.importDecl(beginConst, impl.CurrentVersion));
  auto *end = dyn_cast_or_null<FuncDecl>(
      impl.importDecl(endConst, impl.CurrentVersion));
  if (!begin || !end)
    return;

  ASSERT(begin->getBaseName() == "__beginUnsafe" &&
         "begin() should always be __Unsafe");
  ASSERT(end->getBaseName() == "__endUnsafe" &&
         "end() should always be __Unsafe");
  ASSERT(!begin->isMutating() && !end->isMutating() &&
         "begin() and end() should not be mutating");

  auto rawIteratorTy = begin->getResultInterfaceType();
  ASSERT(rawIteratorTy->getCanonicalType() ==
             end->getResultInterfaceType()->getCanonicalType() &&
         "begin() and end() should have the same return type");

  // Check if RawIterator conforms to UnsafeCxxInputIterator.
  auto rawIteratorConformanceRef =
      checkConformance(rawIteratorTy, cxxIteratorProto);
  if (!rawIteratorConformanceRef)
    return;
  auto rawIteratorConformance = rawIteratorConformanceRef.getConcrete();
  auto pointeeDecl =
      cxxIteratorProto->getAssociatedType(ctx.getIdentifier("Pointee"));
  assert(pointeeDecl &&
         "UnsafeCxxInputIterator must have a Pointee associated type");
  auto pointeeTy = rawIteratorConformance->getTypeWitness(pointeeDecl);
  assert(pointeeTy && "valid conformance must have a Pointee witness");

  impl.addSynthesizedTypealias(decl, ctx.Id_Element, pointeeTy);
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("RawIterator"),
                               rawIteratorTy);

  conformToCxxIterableIfNeeded(impl, decl, clangDecl,
                                        rawIteratorConformance);

  // `CxxSequence` and `CxxRandomAccessCollection` protocols require `Element`
  // to be Copyable and Escapable
  if (!pointeeTy->isCopyable() || !pointeeTy->isEscapable())
    return;

  // CxxSequence conformance.
  // Take the default definition of `Iterator` from CxxSequence protocol. This
  // type is currently `CxxIterator<Self>`.
  auto iteratorDecl = cxxSequenceProto->getAssociatedType(ctx.Id_Iterator);
  auto iteratorTy = iteratorDecl->getDefaultDefinitionType();
  // Substitute generic `Self` parameter.
  auto declSelfTy = decl->getDeclaredInterfaceType();
  auto cxxSequenceSelfTy = cxxSequenceProto->getSelfInterfaceType();
  iteratorTy = iteratorTy.subst(
      [&](SubstitutableType *dependentType) {
        if (dependentType->isEqual(cxxSequenceSelfTy))
          return declSelfTy;
        return Type(dependentType);
      },
      LookUpConformanceInModule());
  impl.addSynthesizedTypealias(decl, ctx.Id_Iterator, iteratorTy);

  // Not conforming the type to CxxSequence protocol here:
  // The current implementation of CxxSequence triggers extra copies of the C++
  // collection when creating a CxxIterator instance. It needs a more efficient
  // implementation, which is not possible with the existing Swift features.
  // impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxSequence});

  // Try to conform to CxxRandomAccessCollection if possible.

  auto tryToConformToRandomAccessCollection = [&]() -> bool {
    auto cxxRAIteratorProto =
        ctx.getProtocol(KnownProtocolKind::UnsafeCxxRandomAccessIterator);
    if (!cxxRAIteratorProto ||
        !ctx.getProtocol(KnownProtocolKind::CxxRandomAccessCollection))
      return false;

    // Check if RawIterator conforms to UnsafeCxxRandomAccessIterator.
    if (!checkConformance(rawIteratorTy, cxxRAIteratorProto))
      return false;

    // CxxRandomAccessCollection always uses Int as an Index.
    auto indexTy = ctx.getIntType();

    auto sliceTy = ctx.getSliceType();
    sliceTy = sliceTy.subst(
        [&](SubstitutableType *dependentType) {
          if (dependentType->isEqual(cxxSequenceSelfTy))
            return declSelfTy;
          return Type(dependentType);
        },
        LookUpConformanceInModule());

    auto indicesTy = ctx.getRangeType();
    indicesTy = indicesTy.subst(
        [&](SubstitutableType *dependentType) {
          if (dependentType->isEqual(cxxSequenceSelfTy))
            return indexTy;
          return Type(dependentType);
        },
        LookUpConformanceInModule());

    impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Index"), indexTy);
    impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Indices"), indicesTy);
    impl.addSynthesizedTypealias(decl, ctx.getIdentifier("SubSequence"),
                                 sliceTy);

    auto tryToConformToMutatingRACollection = [&]() -> bool {
      auto rawMutableIteratorProto = ctx.getProtocol(
          KnownProtocolKind::UnsafeCxxMutableRandomAccessIterator);
      if (!rawMutableIteratorProto)
        return false;

      // Check if present: `func __beginMutatingUnsafe() -> RawMutableIterator`
      auto beginMutatingId = ctx.getIdentifier("__beginMutatingUnsafe");
      auto beginMutating =
          lookupDirectSingleWithoutExtensions<FuncDecl>(decl, beginMutatingId);
      if (!beginMutating)
        return false;
      auto rawMutableIteratorTy = beginMutating->getResultInterfaceType();

      // Check if present: `func __endMutatingUnsafe() -> RawMutableIterator`
      auto endMutatingId = ctx.getIdentifier("__endMutatingUnsafe");
      auto endMutating =
          lookupDirectSingleWithoutExtensions<FuncDecl>(decl, endMutatingId);
      if (!endMutating)
        return false;

      if (!checkConformance(rawMutableIteratorTy, rawMutableIteratorProto))
        return false;

      impl.addSynthesizedTypealias(
          decl, ctx.getIdentifier("RawMutableIterator"), rawMutableIteratorTy);
      impl.addSynthesizedProtocolAttrs(
          decl, {KnownProtocolKind::CxxMutableRandomAccessCollection});
      return true;
    };

    bool conformedToMutableRAC = tryToConformToMutatingRACollection();

    if (!conformedToMutableRAC)
      impl.addSynthesizedProtocolAttrs(
          decl, {KnownProtocolKind::CxxRandomAccessCollection});

    return true;
  };

  bool conformedToRAC = tryToConformToRandomAccessCollection();

  // If the collection does not support random access, let's still allow the
  // developer to explicitly convert a C++ sequence to a Swift Array (making a
  // copy of the sequence's elements) by conforming the type to
  // CxxCollectionConvertible. This enables an overload of Array.init declared
  // in the Cxx module.
  ProtocolDecl *cxxConvertibleProto =
      ctx.getProtocol(KnownProtocolKind::CxxConvertibleToCollection);
  if (!conformedToRAC && cxxConvertibleProto) {
    impl.addSynthesizedProtocolAttrs(
        decl, {KnownProtocolKind::CxxConvertibleToCollection});
  }
}

bool swift::isUnsafeStdMethod(const clang::CXXMethodDecl *methodDecl) {
  auto *parentDecl =
      dyn_cast<clang::CXXRecordDecl>(methodDecl->getDeclContext());
  if (!parentDecl || !parentDecl->isInStdNamespace() ||
      !parentDecl->getIdentifier())
    return false;

  if (methodDecl->getIdentifier() && methodDecl->getName() == "insert") {
    // Types for which the insert method is considered unsafe,
    // due to potential iterator invalidation.
    return llvm::StringSwitch<bool>(parentDecl->getName())
        .Cases({"set", "unordered_set", "multiset"}, true)
        .Cases({"map", "unordered_map", "multimap"}, true)
        .Default(false);
  }
  return false;
}

/// Look up the `insert(const value_type&)` overload on a C++ container.
///
/// C++ containers like std::set and std::map have multiple `insert` overloads.
/// This finds the overload that takes a single `const value_type&` parameter,
/// which maps most closely to Swift's semantics. The return type of this
/// overload is used as the InsertionResult associated type, since there is no
/// equivalent typedef in C++ we can use directly.
static const clang::CXXMethodDecl *
findInsertMethod(clang::Sema &clangSema, clang::ASTContext &clangCtx,
                 const clang::CXXRecordDecl *clangDecl,
                 const clang::TypeDecl *valueType) {
  auto R = clang::LookupResult(
      clangSema, &clangSema.PP.getIdentifierTable().get("insert"),
      clang::SourceLocation(), clang::Sema::LookupMemberName);
  R.suppressDiagnostics();
  clangSema.LookupQualifiedName(
      R, const_cast<clang::CXXRecordDecl *>(clangDecl));
  switch (R.getResultKind()) {
  case clang::LookupResultKind::Found:
  case clang::LookupResultKind::FoundOverloaded:
    break;
  default:
    return nullptr;
  }

  for (auto *nd : R) {
    if (auto *insertOverload = dyn_cast<clang::CXXMethodDecl>(nd)) {
      if (insertOverload->param_size() != 1)
        continue;
      auto *paramTy = (*insertOverload->param_begin())
                          ->getType()
                          ->getAs<clang::ReferenceType>();
      if (!paramTy)
        continue;
      if (paramTy->getPointeeType()->getCanonicalTypeUnqualified() !=
          clangCtx.getTypeDeclType(valueType)->getCanonicalTypeUnqualified())
        continue;
      if (!paramTy->getPointeeType().isConstQualified())
        continue;
      return insertOverload;
    }
  }
  return nullptr;
}

static void conformToCxxSet(ClangImporter::Implementation &impl,
                            NominalTypeDecl *decl,
                            const clang::CXXRecordDecl *clangDecl,
                            bool isUniqueSet) {
  PrettyStackTraceDecl trace("conforming to CxxSet", decl);
  ASTContext &ctx = decl->getASTContext();
  auto &clangCtx = impl.getClangASTContext();
  auto &clangSema = impl.getClangSema();

  // Look up the type members we need from Clang
  //
  // N.B. we don't actually need const_iterator for multiset, but it should be
  // there. If it's not there for any reason, we should probably bail out.

  auto *size_type = lookupCxxTypeMember(clangSema, clangDecl, "size_type",
                                        /*mustBeComplete=*/true);
  auto *value_type = lookupCxxTypeMember(clangSema, clangDecl, "value_type",
                                         /*mustBeComplete=*/true);
  auto *iterator = lookupCxxTypeMember(clangSema, clangDecl, "iterator",
                                       /*mustBeComplete=*/true);
  auto *const_iterator =
      lookupCxxTypeMember(clangSema, clangDecl, "const_iterator",
                          /*mustBeComplete=*/true);
  if (!size_type || !value_type || !iterator || !const_iterator)
    return;

  auto *insert = findInsertMethod(clangSema, clangCtx, clangDecl, value_type);
  if (!insert)
    return;

  // We've looked up everything we need from Clang for the conformance.
  // Now, use ClangImporter to convert import those types to Swift.
  //
  // NOTE: we're actually importing the typedefs and function members here,
  // but not *adding* them as members to the Swift StructDecl---that is done
  // elsewhere (and could be lazy too, though not at this time of writing).
  // We are just using these imported Swift members for their type fields,
  // because importDecl() needs fewer arguments than importTypeIgnoreIUO().

  auto *Size = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(size_type, impl.CurrentVersion));
  auto *Value = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(value_type, impl.CurrentVersion));
  auto *RawMutableIterator = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(iterator, impl.CurrentVersion));
  auto *RawIterator = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(const_iterator, impl.CurrentVersion));
  auto *Insert =
      dyn_cast_or_null<FuncDecl>(impl.importDecl(insert, impl.CurrentVersion));
  if (!Size || !Value || !RawMutableIterator || !RawIterator || !Insert)
    return;

  // We have our Swift types, synthesize type aliases and conformances

  impl.addSynthesizedTypealias(decl, ctx.Id_Element,
                               Value->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.Id_ArrayLiteralElement,
                               Value->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Size"),
                               Size->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("RawIterator"),
                               RawIterator->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("RawMutableIterator"),
                               RawMutableIterator->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("InsertionResult"),
                               Insert->getResultInterfaceType());

  impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxSet});
  if (isUniqueSet)
    impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxUniqueSet});
}

static void conformToCxxPair(ClangImporter::Implementation &impl,
                             NominalTypeDecl *decl,
                             const clang::CXXRecordDecl *clangDecl) {
  PrettyStackTraceDecl trace("conforming to CxxPair", decl);
  ASTContext &ctx = decl->getASTContext();
  clang::Sema &clangSema = impl.getClangSema();

  auto *first_type = lookupCxxTypeMember(clangSema, clangDecl, "first_type",
                                         /*mustBeComplete=*/true);
  auto *second_type = lookupCxxTypeMember(clangSema, clangDecl, "second_type",
                                          /*mustBeComplete=*/true);
  if (!first_type || !second_type)
    return;

  auto *First = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(first_type, impl.CurrentVersion));
  auto *Second = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(second_type, impl.CurrentVersion));
  if (!First || !Second)
    return;

  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("First"),
                               First->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Second"),
                               Second->getUnderlyingType());
  impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxPair});
}

static void conformToCxxDictionary(ClangImporter::Implementation &impl,
                                   NominalTypeDecl *decl,
                                   const clang::CXXRecordDecl *clangDecl) {
  PrettyStackTraceDecl trace("conforming to CxxDictionary", decl);
  ASTContext &ctx = decl->getASTContext();
  clang::ASTContext &clangCtx = impl.getClangASTContext();
  clang::Sema &clangSema = impl.getClangSema();

  auto *key_type = lookupCxxTypeMember(clangSema, clangDecl, "key_type", true);
  auto *mapped_type =
      lookupCxxTypeMember(clangSema, clangDecl, "mapped_type", true);
  auto *value_type =
      lookupCxxTypeMember(clangSema, clangDecl, "value_type", true);
  auto *size_type =
      lookupCxxTypeMember(clangSema, clangDecl, "size_type", true);
  auto *iterator = lookupCxxTypeMember(clangSema, clangDecl, "iterator", true);
  auto *const_iterator =
      lookupCxxTypeMember(clangSema, clangDecl, "const_iterator", true);
  if (!key_type || !mapped_type || !value_type || !size_type || !iterator ||
      !const_iterator)
    return;

  auto *insert = findInsertMethod(clangSema, clangCtx, clangDecl, value_type);
  if (!insert)
    return;

  auto *Size = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(size_type, impl.CurrentVersion));
  auto *Key = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(key_type, impl.CurrentVersion));
  auto *Value = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(mapped_type, impl.CurrentVersion));
  auto *Element = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(value_type, impl.CurrentVersion));
  auto *RawIterator = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(const_iterator, impl.CurrentVersion));
  auto *RawMutableIterator = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(iterator, impl.CurrentVersion));
  if (!Size || !Key || !Value || !Element || !RawIterator ||
      !RawMutableIterator)
    return;

  auto *Insert =
      dyn_cast_or_null<FuncDecl>(impl.importDecl(insert, impl.CurrentVersion));
  if (!Insert)
    return;

  impl.addSynthesizedTypealias(decl, ctx.Id_Key, Key->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.Id_Value, Value->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.Id_Element,
                               Element->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("RawIterator"),
                               RawIterator->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("RawMutableIterator"),
                               RawMutableIterator->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Size"),
                               Size->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("InsertionResult"),
                               Insert->getResultInterfaceType());
  impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxDictionary});

  // Prevent subscript (returning non-optional) from being synthesized.
  // CxxDictionary adds another subscript that returns an optional value,
  // similarly to Swift.Dictionary.
  (void)impl.lookupAndImportSubscripts(decl, /*noSynthesize=*/true);
}

static void conformToCxxVector(ClangImporter::Implementation &impl,
                               NominalTypeDecl *decl,
                               const clang::CXXRecordDecl *clangDecl) {
  PrettyStackTraceDecl trace("conforming to CxxVector", decl);
  ASTContext &ctx = decl->getASTContext();
  clang::Sema &clangSema = impl.getClangSema();

  auto *value_type = lookupCxxTypeMember(clangSema, clangDecl, "value_type",
                                         /*mustBeComplete=*/true);
  auto *size_type = lookupCxxTypeMember(clangSema, clangDecl, "size_type",
                                        /*mustBeComplete=*/true);
  auto *const_iterator =
      lookupCxxTypeMember(clangSema, clangDecl, "const_iterator",
                          /*mustBeComplete=*/true);

  auto *Element = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(value_type, impl.CurrentVersion));
  auto *Size = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(size_type, impl.CurrentVersion));
  auto *RawIterator = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(const_iterator, impl.CurrentVersion));
  if (!Element || !Size || !RawIterator)
    return;

  impl.addSynthesizedTypealias(decl, ctx.Id_Element,
                               Element->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.Id_ArrayLiteralElement,
                               Element->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Size"),
                               Size->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("RawIterator"),
                               RawIterator->getUnderlyingType());
  impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxVector});
}

static void conformToCxxSpan(ClangImporter::Implementation &impl,
                             NominalTypeDecl *decl,
                             const clang::CXXRecordDecl *clangDecl) {
  PrettyStackTraceDecl trace("conforming to CxxSpan", decl);
  ASTContext &ctx = decl->getASTContext();
  clang::ASTContext &clangCtx = impl.getClangASTContext();
  clang::Sema &clangSema = impl.getClangSema();

  auto *element_type = lookupCxxTypeMember(clangSema, clangDecl, "element_type",
                                           /*mustBeComplete=*/true);
  auto *size_type = lookupCxxTypeMember(clangSema, clangDecl, "size_type",
                                        /*mustBeComplete=*/true);
  auto *pointer = lookupCxxTypeMember(clangSema, clangDecl, "pointer",
                                      /*mustBeComplete=*/true);
  if (!element_type || !size_type || !pointer)
    return;

  auto pointerType = clangCtx.getTypeDeclType(pointer);
  auto sizeType = clangCtx.getTypeDeclType(size_type);

  auto *Element = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(element_type, impl.CurrentVersion));
  auto *Size = dyn_cast_or_null<TypeAliasDecl>(
      impl.importDecl(size_type, impl.CurrentVersion));
  if (!Element || !Size)
    return;

  impl.addSynthesizedTypealias(decl, ctx.Id_Element,
                               Element->getUnderlyingType());
  impl.addSynthesizedTypealias(decl, ctx.getIdentifier("Size"),
                               Size->getUnderlyingType());

  if (pointerType->getPointeeType().isConstQualified())
    impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxSpan});
  else
    impl.addSynthesizedProtocolAttrs(decl, {KnownProtocolKind::CxxMutableSpan});

  // create fake variable for pointer (constructor arg 1)
  auto fakePointerVarDecl = clang::VarDecl::Create(
      clangCtx, /*DC*/ clangCtx.getTranslationUnitDecl(),
      clang::SourceLocation(), clang::SourceLocation(), /*Id*/ nullptr,
      pointerType, clangCtx.getTrivialTypeSourceInfo(pointerType),
      clang::StorageClass::SC_None);

  auto fakePointer = createClangDeclRefExpr(clangCtx, fakePointerVarDecl,
                                            pointerType, clang::VK_LValue);

  // create fake variable for count (constructor arg 2)
  auto fakeCountVarDecl = clang::VarDecl::Create(
      clangCtx, /*DC*/ clangCtx.getTranslationUnitDecl(),
      clang::SourceLocation(), clang::SourceLocation(), /*Id*/ nullptr,
      sizeType, clangCtx.getTrivialTypeSourceInfo(sizeType),
      clang::StorageClass::SC_None);

  auto fakeCount = createClangDeclRefExpr(clangCtx, fakeCountVarDecl, sizeType,
                                         clang::VK_LValue);

  // Use clangSema.BuildCxxTypeConstructExpr to create a CXXTypeConstructExpr,
  // passing constPointer and count
  SmallVector<clang::Expr *, 2> constructExprArgs = {fakePointer, fakeCount};

  auto clangDeclTyInfo = clangCtx.getTrivialTypeSourceInfo(
      clang::QualType(clangDecl->getTypeForDecl(), 0));

  // Instantiate the templated constructor that would accept this fake variable.
  auto constructExprResult = clangSema.BuildCXXTypeConstructExpr(
      clangDeclTyInfo, clangDecl->getLocation(), constructExprArgs,
      clangDecl->getLocation(), /*ListInitialization*/ false);
  if (!constructExprResult.isUsable())
    return;

  auto constructExpr =
      dyn_cast_or_null<clang::CXXConstructExpr>(constructExprResult.get());
  if (!constructExpr)
    return;

  auto constructorDecl = constructExpr->getConstructor();
  auto importedConstructor =
      impl.importDecl(constructorDecl, impl.CurrentVersion);
  if (!importedConstructor)
    return;

  auto attr = AvailableAttr::createUniversallyDeprecated(
      importedConstructor->getASTContext(), "use 'init(_:)' instead.", "");
  importedConstructor->addAttribute(attr);

  decl->addMember(importedConstructor);
}

void swift::deriveAutomaticCxxConformances(
    ClangImporter::Implementation &Impl, NominalTypeDecl *result,
    const clang::CXXRecordDecl *clangDecl) {

  ASSERT(result && clangDecl && "this should not be called with nullptrs");

  // Skip synthesizing conformances if the associated Clang node is from
  // a module that doesn't require cplusplus, to prevent us from accidentally
  // pulling in Cxx/CxxStdlib modules when a client is importing a C library.
  //
  // We will still attempt to synthesize to account for scenarios where the
  // module specification is missing altogether.
  if (auto *clangModule = importer::getClangOwningModule(
          result->getClangNode(), Impl.getClangASTContext());
      clangModule && !requiresCPlusPlus(clangModule))
    return;

  // Automatic conformances: these may be applied to any type that fits the
  // requirements.
  conformToCxxIteratorIfNeeded(Impl, result, clangDecl);
  conformToCxxSequenceIfNeeded(Impl, result, clangDecl);
  conformToCxxConvertibleToBoolIfNeeded(Impl, result);

  // CxxStdlib conformances: these should only apply to known C++ stdlib types,
  // which we determine by name and membership in the std namespace.
  if (!clangDecl->getIdentifier() || !clangDecl->isInStdNamespace())
    return;

  auto ty = identifyCxxStdTypeByName(clangDecl->getName());
  switch (ty) {
  case CxxStdType::uncategorized:
    return;
  case CxxStdType::optional:
    conformToCxxOptional(Impl, result, clangDecl);
    return;
  case CxxStdType::set:
  case CxxStdType::unordered_set:
    conformToCxxSet(Impl, result, clangDecl, /*isUniqueSet=*/true);
    return;
  case CxxStdType::multiset:
    conformToCxxSet(Impl, result, clangDecl, /*isUniqueSet=*/false);
    return;
  case CxxStdType::pair:
    conformToCxxPair(Impl, result, clangDecl);
    return;
  case CxxStdType::map:
  case CxxStdType::unordered_map:
  case CxxStdType::multimap:
    conformToCxxDictionary(Impl, result, clangDecl);
    return;
  case CxxStdType::vector:
    conformToCxxVector(Impl, result, clangDecl);
    return;
  case CxxStdType::span:
    conformToCxxSpan(Impl, result, clangDecl);
    return;
  }
}
