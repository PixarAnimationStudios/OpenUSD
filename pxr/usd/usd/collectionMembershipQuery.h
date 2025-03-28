//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_COLLECTION_MEMBERSHIP_QUERY_H
#define PXR_USD_USD_COLLECTION_MEMBERSHIP_QUERY_H

/// \file usd/collectionMembershipQuery.h

#include "pxr/pxr.h"

#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/primFlags.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/pathExpressionEval.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

#define USD_COLLECTION_MEMBERSHIP_QUERY_TOKENS \
    (IncludedByMembershipExpression)           \
    (ExcludedByMembershipExpression)

TF_DECLARE_PUBLIC_TOKENS(UsdCollectionMembershipQueryTokens,
                         USD_API, USD_COLLECTION_MEMBERSHIP_QUERY_TOKENS);


class Usd_CollectionMembershipQueryBase
{
public:
    /// Holds an unordered map describing membership of paths in this collection
    /// and the associated expansionRule for how the paths are to be expanded.
    /// Valid expansionRules are UsdTokens->explicitOnly,
    /// UsdTokens->expandPrims, and UsdTokens->expandPrimsAndProperties.  For
    /// more information on the expansion rules, please see the expansionRule
    /// attribute on UsdCollectionAPI.
    /// If a collection includes another collection, the included collection's
    /// PathExpansionRuleMap is merged into this one. If a path is excluded,
    /// its expansion rule is set to UsdTokens->exclude.
    using PathExpansionRuleMap = std::unordered_map<SdfPath,
          TfToken, SdfPath::Hash>;

    /// Default Constructor, creates an empty Usd_CollectionMembershipQueryBase
    /// object
    Usd_CollectionMembershipQueryBase() = default;

    /// Constructor that takes a path expansion rule map.  The map is scanned
    /// for 'excludes' when the Usd_CollectionMembershipQueryBase object is
    /// constructed.
    Usd_CollectionMembershipQueryBase(
        const PathExpansionRuleMap& pathExpansionRuleMap,
        const SdfPathSet& includedCollections);

    /// Constructor that takes a path expansion rule map as an rvalue reference
    Usd_CollectionMembershipQueryBase(
        PathExpansionRuleMap&& pathExpansionRuleMap,
        SdfPathSet&& includedCollections);

    /// Constructor that additionally takes an additional expression evaluator
    /// and a top-level expansion rule.
    Usd_CollectionMembershipQueryBase(
        const PathExpansionRuleMap& pathExpansionRuleMap,
        const SdfPathSet& includedCollections,
        const TfToken &topExpansionRule);

    /// Constructor that additionally takes an additional expression evaluator
    /// as an rvalue reference and a top-level expansion rule.
    Usd_CollectionMembershipQueryBase(
        PathExpansionRuleMap&& pathExpansionRuleMap,
        SdfPathSet&& includedCollections,
        TfToken const &topExpansionRule);
    
    /// Returns true if the collection excludes one or more paths below an
    /// included path via the excludes relationship (see
    /// UsdCollectionAPI::GetExcludesRel()).
    bool HasExcludes() const {
        return _hasExcludes;
    }
    

    /// Returns a raw map of the paths included or excluded in the
    /// collection along with the expansion rules for the included
    /// paths.
    const PathExpansionRuleMap& GetAsPathExpansionRuleMap() const {
        return _pathExpansionRuleMap;
    }

    /// Returns a set of paths for all collections that were included in the
    /// collection from which this Usd_CollectionMembershipQueryBase object was
    /// computed. This set is recursive, so collections that were included by
    /// other collections will be part of this set. The collection from which
    /// this Usd_CollectionMembershipQueryBase object was computed is *not* part
    /// of this set.
    const SdfPathSet& GetIncludedCollections() const {
        return _includedCollections;
    }

    /// Return the top expansion rule for this query object.  This rule is the
    /// expansion rule from the UsdCollectionAPI instance that was used to build
    /// this query object.  The top expansion rule is used when evaluating the
    /// expression associated with this query, to determine whether it should
    /// match prims only, or both prims and properties.
    TfToken GetTopExpansionRule() const {
        return _topExpansionRule;
    }

protected:

    /// Hash functor
    struct _Hash {
        USD_API
        size_t operator()(Usd_CollectionMembershipQueryBase const& query) const;
    };

    /// Hash function
    inline size_t _GetHash() const {
        return _Hash()(*this);
    }

    USD_API
    bool _IsPathIncludedByRuleMap(const SdfPath &path,
                                  TfToken *expansionRule=nullptr) const;

    USD_API
    bool _IsPathIncludedByRuleMap(const SdfPath &path,
                                  const TfToken &parentExpansionRule,
                                  TfToken *expansionRule=nullptr) const;

    // Return true if the _pathExpansionRuleMap is empty, meaning that we would
    // use an expression in the derived class, for example.
    USD_API
    bool _HasEmptyRuleMap() const;

    TfToken _topExpansionRule;
    
    PathExpansionRuleMap _pathExpansionRuleMap;

    SdfPathSet _includedCollections;

    // A cached flag indicating whether _pathExpansionRuleMap contains
    // any exclude rules.
    bool _hasExcludes=false;
};

/// Compute an SdfPathExpression that matches the same paths as \p ruleMap.  The
/// resulting SdfPathExpression is always complete (see
/// SdfPathExpression::IsComplete()) and never contains predicates.
USD_API
SdfPathExpression
UsdComputePathExpressionFromCollectionMembershipQueryRuleMap(
    Usd_CollectionMembershipQueryBase::PathExpansionRuleMap const &ruleMap);

// -------------------------------------------------------------------------- //
// UsdCollectionMembershipQuery                                               //
// -------------------------------------------------------------------------- //
/// \class UsdCollectionMembershipQuery
///
/// \brief Represents a flattened view of a collection.  For more information
/// about collections, please see UsdCollectionAPI as a way to encode and
/// retrieve a collection from scene description.  A
/// UsdCollectionMembershipQuery object can be used to answer queries about
/// membership of paths in the collection efficiently.
template <class ExprEval>
class Usd_CollectionMembershipQuery : public Usd_CollectionMembershipQueryBase
{
public:
    using ExpressionEvaluator = ExprEval;
    
    using Usd_CollectionMembershipQueryBase::Usd_CollectionMembershipQueryBase;

    /// \overload
    /// Returns whether the given path is included in the collection from which
    /// this Usd_CollectionMembershipQueryBase object was computed. This is the
    /// API that clients should use for determining if a given object is a
    /// member of the collection. To enumerate all the members of a collection,
    /// use \ref UsdComputeIncludedObjectsFromCollection or \ref
    /// UsdComputeIncludedPathsFromCollection.
    ///
    /// If \p expansionRule is not nullptr, it is set to the expansion- rule
    /// value that caused the path to be included in or excluded from the
    /// collection. If \p path is not included in the collection, \p
    /// expansionRule is set to UsdTokens->exclude.  If this query is not using
    /// an expansion rule map and is instead using a pattern-based membership
    /// expression, then expansionRule is set to one of the special
    /// UsdCollectionMembershipQueryTokens values,
    /// IncludedByMembershipExpression or ExcludedByMembershipExpression as
    /// appropriate.
    ///
    /// It is useful to specify this parameter and use this overload of
    /// IsPathIncluded(), when you're interested in traversing a subtree
    /// and want to know whether the root of the subtree is included in a
    /// collection. For evaluating membership of descendants of the root,
    /// please use the other overload of IsPathIncluded(), that takes both
    /// a path and the parent expansionRule.
    ///
    /// The python version of this method only returns the boolean result.  It
    /// does not return an SdfPredicateFunctionResult or set \p expansionRule.
    SdfPredicateFunctionResult
    IsPathIncluded(const SdfPath &path,
                   TfToken *expansionRule=nullptr) const {
        // If we have a rule map, go that way.  Otherwise try the expression.
        if (UsesPathExpansionRuleMap()) {
            return SdfPredicateFunctionResult::MakeVarying(
                _IsPathIncludedByRuleMap(path, expansionRule));
        }
        const SdfPredicateFunctionResult
            res = GetExpressionEvaluator().Match(path);
        if (expansionRule) {
            *expansionRule = res ?
                UsdCollectionMembershipQueryTokens->
                    IncludedByMembershipExpression : 
                UsdCollectionMembershipQueryTokens->
                    ExcludedByMembershipExpression;
        }
        return res;
    }

    /// \overload
    /// Returns whether the given path, \p path is included in the
    /// collection from which this Usd_CollectionMembershipQueryBase object was
    /// computed, given the parent-path's inherited expansion rule,
    /// \p parentExpansionRule.
    ///
    /// If \p expansionRule is not nullptr, it is set to the expansion- rule
    /// value that caused the path to be included in or excluded from the
    /// collection. If \p path is not included in the collection, \p
    /// expansionRule is set to UsdTokens->exclude.  If this query is not using
    /// an expansion rule map and is instead using a pattern-based membership
    /// expression, then expansionRule is set to one of the special
    /// UsdCollectionMembershipQueryTokens values,
    /// IncludedByMembershipExpression or ExcludedByMembershipExpression as
    /// appropriate.
    ///
    /// The python version of this method only returns the boolean result.
    /// It does not return \p expansionRule.
    SdfPredicateFunctionResult
    IsPathIncluded(const SdfPath &path,
                   const TfToken &parentExpansionRule,
                   TfToken *expansionRule=nullptr) const {
        // If we have a rule map, go that way.  Otherwise try the expression.
        if (UsesPathExpansionRuleMap()) {
            return SdfPredicateFunctionResult::MakeVarying(
                _IsPathIncludedByRuleMap(
                    path, parentExpansionRule, expansionRule));
        }
        const SdfPredicateFunctionResult
            res = GetExpressionEvaluator().Match(path);
        if (expansionRule) {
            *expansionRule = res ?
                UsdCollectionMembershipQueryTokens->
                    IncludedByMembershipExpression : 
                UsdCollectionMembershipQueryTokens->
                    ExcludedByMembershipExpression;
        }
        return res;
    }

    /// Return true if this query uses the explicit path-expansion rule method
    /// to determine collection membership.  Otherwise, return false if it uses
    /// the pattern-based membership expression to determine membership.
    bool UsesPathExpansionRuleMap() const {
        return !_HasEmptyRuleMap();
    }

    void
    SetExpressionEvaluator(ExpressionEvaluator &&exprEval) {
        _exprEval = std::move(exprEval);
    }

    void
    SetExpressionEvaluator(ExpressionEvaluator const &exprEval) {
        SetExpressionEvaluator(ExpressionEvaluator { exprEval } );
    }
    
    /// Return the expression evaluator associated with this query object.  This
    /// may be an empty evaluator.  See HasExpression().
    ExpressionEvaluator const &
    GetExpressionEvaluator() const {
        return _exprEval;
    }

    /// Return true if the expression evaluator associated with this query
    /// object is not empty.  See GetExpressionEvaluator().
    bool HasExpression() const {
        return !_exprEval.IsEmpty();
    }

    /// Equality operator
    bool operator==(Usd_CollectionMembershipQuery const& rhs) const {
        // Note that MembershipQuery objects that have non-empty _exprEval never
        // compare equal to each other.  This is because the evaluator objects
        // run code, and there's no good way to determine equivalence.
        return _topExpansionRule == rhs._topExpansionRule &&
            _hasExcludes == rhs._hasExcludes &&
            _pathExpansionRuleMap == rhs._pathExpansionRuleMap &&
            _includedCollections == rhs._includedCollections &&
            _exprEval.IsEmpty() == rhs._exprEval.IsEmpty();
        ;
    }

    /// Inequality operator
    bool operator!=(Usd_CollectionMembershipQuery const& rhs) const {
        return !(*this == rhs);
    }

    /// Hash functor
    struct Hash {
        size_t operator()(Usd_CollectionMembershipQuery const& query) const {
            return TfHash::Combine(query._GetHash(), query._exprEval.IsEmpty());
        }
    };

    /// Hash function
    inline size_t GetHash() const {
        return Hash()(*this);
    }
    
private:
    ExpressionEvaluator _exprEval;
};


/// \class UsdObjectCollectionExpressionEvaluator
///
/// \brief Evaluates SdfPathExpressions with objects from a given UsdStage.
class UsdObjectCollectionExpressionEvaluator {
    struct PathToObj {
        UsdObject operator()(SdfPath const &path) const {
            return stage->GetObjectAtPath(path);
        }
        UsdStageWeakPtr stage;
    };
    
public:
    using PathExprEval = SdfPathExpressionEval<UsdObject>;
    using IncrementalSearcher =
        typename PathExprEval::IncrementalSearcher<PathToObj>;
    
    /// Construct an empty evaluator.
    UsdObjectCollectionExpressionEvaluator() = default;
    
    /// Construct an evaluator that evalutates \p expr on objects from \p
    /// stage.  The \p expr must be "complete" (see
    /// SdfPathExpression::IsComplete()).
    ///
    /// Typically these objects are not constructed directly, but instead
    /// are created by UsdCollectionAPI::ComputeMembershipQuery() for
    /// Usd_CollectionMembershipQuery's use.  However it is possible to
    /// construct them directly if one wishes.  Consider calling
    /// UsdCollectionAPI::ResolveCompleteMembershipExpression() to produce
    /// an approprate expression.
    USD_API
    UsdObjectCollectionExpressionEvaluator(UsdStageWeakPtr const &stage,
                                           SdfPathExpression const &expr);

    /// Return true if this evaluator has an invalid stage or an empty
    /// underlying SdfPathExpressionEval object.
    bool IsEmpty() const {
        return !_stage || _evaluator.IsEmpty();
    }

    /// Return the stage this object was constructed with, or nullptr if it
    /// was default constructed.
    UsdStageWeakPtr const &GetStage() const { return _stage; }

    /// Return the result of evaluating the expression against \p path.
    USD_API
    SdfPredicateFunctionResult
    Match(SdfPath const &path) const;

    /// Return the result of evaluating the expression against \p object.
    USD_API
    SdfPredicateFunctionResult
    Match(UsdObject const &object) const;

    /// Create an incremental searcher from this evaluator.  See
    /// SdfPathExpressionEval::IncrementalSearcher for more info and API.
    ///
    /// The returned IncrementalSearcher refers to the evaluator object owned by
    /// this UsdObjectCollectionExpressionEvaluator object.  This means that the
    /// IncrementalSearcher must not be used after this
    /// UsdObjectCollectionExpressionEvaluator object's lifetime ends.
    USD_API
    IncrementalSearcher MakeIncrementalSearcher() const;
    
private:
    UsdStageWeakPtr _stage;
    PathExprEval _evaluator;
};

using UsdCollectionMembershipQuery =
    Usd_CollectionMembershipQuery<UsdObjectCollectionExpressionEvaluator>;

/// Returns all the usd objects that satisfy the predicate, \p pred in the
/// collection represented by the UsdCollectionMembershipQuery object, \p
/// query.
///
/// The results depends on the load state of the UsdStage, \p stage.
USD_API
std::set<UsdObject> UsdComputeIncludedObjectsFromCollection(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred=UsdPrimDefaultPredicate);

/// Returns all the paths that satisfy the predicate, \p pred in the
/// collection represented by the UsdCollectionMembershipQuery object, \p
/// query.
///
/// The result depends on the load state of the UsdStage, \p stage.
USD_API
SdfPathSet UsdComputeIncludedPathsFromCollection(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred=UsdPrimDefaultPredicate);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
