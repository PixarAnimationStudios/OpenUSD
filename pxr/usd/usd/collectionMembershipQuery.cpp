//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usd/collectionMembershipQuery.h"
#include "pxr/usd/usd/collectionPredicateLibrary.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdCollectionMembershipQueryTokens,
                        USD_COLLECTION_MEMBERSHIP_QUERY_TOKENS);

namespace {

/* static */
void
_ComputeIncludedImpl(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred,
    std::set<UsdObject> *includedObjects,
    SdfPathSet *includedPaths)
{
    using ExpressionEvaluator =
        UsdCollectionMembershipQuery::ExpressionEvaluator;
    
    if (!((bool)includedObjects ^ (bool)includedPaths)) {
        TF_CODING_ERROR("Either includedObjects or includedPaths must be"
                        " valid, but not both");
    }

    std::set<UsdObject> result;

    const UsdCollectionMembershipQuery::PathExpansionRuleMap& pathExpRuleMap =
        query.GetAsPathExpansionRuleMap();
    const bool hasExcludes = query.HasExcludes();

    // Helper function to get the UsdProperty object associated with a given
    // property path.
    auto GetPropertyAtPath = [stage](const SdfPath &path) {
        if (const UsdPrim p = stage->GetPrimAtPath(path.GetPrimPath())) {
            return p.GetProperty(path.GetNameToken());
        }
        return UsdProperty();
    };

    // Returns true if a property is excluded in the PathExpansionRuleMap.
    auto IsPropertyExplicitlyExcluded = [hasExcludes,pathExpRuleMap](
            const SdfPath &propPath) {
        if (!hasExcludes) {
            return false;
        }
        auto it = pathExpRuleMap.find(propPath);
        if (it != pathExpRuleMap.end()) {
            return it->second == UsdTokens->exclude;
        }
        return false;
    };

    auto AppendIncludedObject = [includedObjects, includedPaths](
            const UsdObject &obj) {
        if (includedObjects) {
            includedObjects->insert(obj);
        } else if (includedPaths) {
            includedPaths->insert(obj.GetPath());
        }
    };

    // Iterate through all the entries in the PathExpansionRuleMap.
    for (const auto &pathAndExpansionRule : pathExpRuleMap) {
        const TfToken &expansionRule = pathAndExpansionRule.second;

        // Skip excluded paths.
        if (expansionRule == UsdTokens->exclude) {
            continue;
        }

        const SdfPath &path = pathAndExpansionRule.first;

        if (expansionRule == UsdTokens->explicitOnly) {
            if (path.IsPrimPath()) {
                UsdPrim p = stage->GetPrimAtPath(path);
                if (p && pred(p)) {
                    AppendIncludedObject(p);
                }
            } else if (path.IsPropertyPath()) {
                if (UsdProperty property = GetPropertyAtPath(path)) {
                    AppendIncludedObject(property.As<UsdObject>());
                }
            } else {
                TF_CODING_ERROR("Unknown path type in membership-map.");
            }
        }

        else if (expansionRule == UsdTokens->expandPrims ||
                 expansionRule == UsdTokens->expandPrimsAndProperties)
        {
            if (path.IsPropertyPath()) {
                if (UsdProperty property = GetPropertyAtPath(path)) {
                    AppendIncludedObject(property.As<UsdObject>());
                }
            } else if (UsdPrim prim = stage->GetPrimAtPath(path)) {

                UsdPrimRange range(prim, pred);
                auto iter = range.begin();
                for (; iter != range.end() ; ++iter) {
                    const UsdPrim &descendantPrim = *iter;

                    // Skip the descendant prim and its subtree
                    // if it's excluded.
                    // If an object below the excluded object is included,
                    // it will have a separate entry in the
                    // path<->expansionRule map.
                    if (hasExcludes && !query.IsPathIncluded(
                            descendantPrim.GetPath())) {
                        iter.PruneChildren();
                        continue;
                    }

                    AppendIncludedObject(descendantPrim.As<UsdObject>());

                    if (expansionRule != UsdTokens->expandPrimsAndProperties) {
                        continue;
                    }

                    // Call GetProperties() on the prim (which is known to be
                    // slow), only when the client is interested in property
                    // objects.
                    //
                    // Call GetPropertyNames() otherwise.
                    if (includedObjects) {
                        std::vector<UsdProperty> properties =
                            descendantPrim.GetProperties();
                        for (const auto &property : properties) {
                            // Add the property to the result only if it's
                            // not explicitly excluded.
                            if (!IsPropertyExplicitlyExcluded(
                                    property.GetPath())) {
                                AppendIncludedObject(property.As<UsdObject>());
                            }
                        }
                    } else {
                        for (const auto &propertyName :
                                descendantPrim.GetPropertyNames()) {
                            SdfPath propertyPath =
                                descendantPrim.GetPath().AppendProperty(
                                    propertyName);
                            if (!IsPropertyExplicitlyExcluded(propertyPath)) {
                                // Can't call IncludeObject here since we're
                                // avoiding creation of the object.
                                includedPaths->insert(propertyPath);
                            }
                        }
                    }
                }
            }
        }
    }

    // Walk everything according to \p pred, and do incremental search.
    TfToken expansionRule = query.GetTopExpansionRule();
    if (query.HasExpression() &&
        (expansionRule == UsdTokens->expandPrims ||
         expansionRule == UsdTokens->expandPrimsAndProperties)) {

        bool searchProperties =
            (expansionRule == UsdTokens->expandPrimsAndProperties);

        ExpressionEvaluator::IncrementalSearcher
            searcher = query.GetExpressionEvaluator().MakeIncrementalSearcher();
        
        UsdPrimRange range = stage->Traverse(pred);
        
        for (auto iter = range.begin(),
                 end = range.end(); iter != end; ++iter) {

            SdfPredicateFunctionResult r = searcher.Next(iter->GetPath());
            bool didProps = false;
            if (r) {
                // With a positive result that's constant over descendants, we
                // can copy everything until the next sibling.
                if (r.IsConstant()) {
                    auto subtreeIter = iter;
                    auto subtreeEnd = subtreeIter;
                    subtreeEnd.PruneChildren();
                    ++subtreeEnd;
                    for (; subtreeIter != subtreeEnd; ++subtreeIter) {
                        AppendIncludedObject(*subtreeIter);
                        if (searchProperties) {
                            didProps = true;
                            for (UsdProperty const &prop:
                                     subtreeIter->GetProperties()) {
                                AppendIncludedObject(prop);
                            }
                        }
                    }
                }
                // We have a positive result on this object, but we have to keep
                // searching descendants, since results may vary.
                else {
                    AppendIncludedObject(*iter);
                }
            }

            // If we're searching properties and didn't already do properties
            // above, do them here.  Constancy doesn't matter here since we
            // don't search descendants of properties.
            if (searchProperties && !didProps && !(r && !r.IsConstant())) {
                for (UsdProperty const &prop: iter->GetProperties()) {
                    if (searcher.Next(prop.GetPath())) {
                        AppendIncludedObject(prop);
                    }
                }
            }

            // If we have a constant result (either positive or negative), we
            // can skip the subtree.
            if (r.IsConstant()) {
                iter.PruneChildren();
            }
        }
    }
}

} // anonymous

UsdObjectCollectionExpressionEvaluator
::UsdObjectCollectionExpressionEvaluator(UsdStageWeakPtr const &stage,
                                         SdfPathExpression const &expr)
    : _stage(stage)
    , _evaluator(SdfMakePathExpressionEval(
                     expr, UsdGetCollectionPredicateLibrary()))
{
}

SdfPredicateFunctionResult
UsdObjectCollectionExpressionEvaluator
::Match(SdfPath const &path) const
{
    if (_stage) {
        if (UsdObject obj = _stage->GetObjectAtPath(path)) {
            return _evaluator.Match(path, PathToObj { _stage });
        }
    }
    return SdfPredicateFunctionResult::MakeConstant(false);
}

SdfPredicateFunctionResult
UsdObjectCollectionExpressionEvaluator
::Match(UsdObject const &obj) const
{
    if (_stage) {
        return _evaluator.Match(obj.GetPath(), PathToObj { _stage });
    }
    return SdfPredicateFunctionResult::MakeConstant(false);
}

UsdObjectCollectionExpressionEvaluator::IncrementalSearcher
UsdObjectCollectionExpressionEvaluator
::MakeIncrementalSearcher() const
{
    if (_stage) {
        return _evaluator.MakeIncrementalSearcher(PathToObj { _stage });
    }
    return {};
}

std::set<UsdObject>
UsdComputeIncludedObjectsFromCollection(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred)
{
    std::set<UsdObject> result;
    _ComputeIncludedImpl(query, stage, pred, &result, nullptr);
    return result;
}

SdfPathSet
UsdComputeIncludedPathsFromCollection(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred)
{
    SdfPathSet result;
    _ComputeIncludedImpl(query, stage, pred, nullptr, &result);
    return result;
}

Usd_CollectionMembershipQueryBase::Usd_CollectionMembershipQueryBase(
    const PathExpansionRuleMap& pathExpansionRuleMap,
    const SdfPathSet& includedCollections)
    : Usd_CollectionMembershipQueryBase(
        PathExpansionRuleMap {pathExpansionRuleMap},
        SdfPathSet {includedCollections}, {})
{
}

Usd_CollectionMembershipQueryBase::Usd_CollectionMembershipQueryBase(
    PathExpansionRuleMap&& pathExpansionRuleMap,
    SdfPathSet&& includedCollections)
    : Usd_CollectionMembershipQueryBase(
        std::move(pathExpansionRuleMap),
        std::move(includedCollections), {})
{
}

Usd_CollectionMembershipQueryBase::Usd_CollectionMembershipQueryBase(
    const PathExpansionRuleMap& pathExpansionRuleMap,
    const SdfPathSet& includedCollections,
    const TfToken &topExpansionRule)
    : Usd_CollectionMembershipQueryBase(
        PathExpansionRuleMap {pathExpansionRuleMap},
        SdfPathSet {includedCollections},
        topExpansionRule)
{
}

Usd_CollectionMembershipQueryBase::Usd_CollectionMembershipQueryBase(
    PathExpansionRuleMap&& pathExpansionRuleMap,
    SdfPathSet&& includedCollections,
    TfToken const &topExpansionRule)
    : _topExpansionRule(topExpansionRule)
    , _pathExpansionRuleMap(std::move(pathExpansionRuleMap))
    , _includedCollections(std::move(includedCollections))
{
    for (const auto &pathAndExpansionRule : _pathExpansionRuleMap) {
        if (pathAndExpansionRule.second == UsdTokens->exclude) {
            _hasExcludes = true;
            break;
        }
    }
}

bool
Usd_CollectionMembershipQueryBase::_IsPathIncludedByRuleMap(
    const SdfPath &path,
    TfToken *expansionRule) const
{
    // Coding Error if one passes in a relative path to _IsPathIncludedByRuleMap
    // Passing one causes a infinite loop because of how `GetParentPath` works.
    if (!path.IsAbsolutePath()) {
        TF_CODING_ERROR("Relative paths are not allowed");
        return false;
    }

    // Only prims and properties can belong to a collection.
    if (!path.IsPrimPath() && !path.IsPropertyPath())
        return false;

    // Have separate code paths for prim and property paths as we'd like this 
    // method to be as fast as possible.
    if (path.IsPrimPath()) {
        for (SdfPath p = path; p != SdfPath::EmptyPath();  p = p.GetParentPath())     
        {
            const auto i = _pathExpansionRuleMap.find(p);
            if (i != _pathExpansionRuleMap.end()) {
                if (i->second == UsdTokens->exclude) {
                    if (expansionRule) {
                        *expansionRule = UsdTokens->exclude;
                    }
                    return false;
                } else if (i->second != UsdTokens->explicitOnly || 
                           p == path) {
                    if (expansionRule) {
                        *expansionRule = i->second;
                    }
                    return true;
                }
            }
        }
    } else {
        for (SdfPath p = path; p != SdfPath::EmptyPath();  p = p.GetParentPath()) 
        {
            const auto i = _pathExpansionRuleMap.find(p);
            if (i != _pathExpansionRuleMap.end()) {
                if (i->second == UsdTokens->exclude) {
                    if (expansionRule) {
                        *expansionRule = UsdTokens->exclude;
                    }
                    return false;
                } else if ((i->second == UsdTokens->expandPrimsAndProperties) ||
                           (i->second == UsdTokens->explicitOnly && p == path)){
                    if (expansionRule) {
                        *expansionRule = i->second;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

bool 
Usd_CollectionMembershipQueryBase::_IsPathIncludedByRuleMap(
    const SdfPath &path,
    const TfToken &parentExpansionRule,
    TfToken *expansionRule) const
{
    // Coding Error if one passes in a relative path to _IsPathIncludedByRuleMap
    if (!path.IsAbsolutePath()) {
        TF_CODING_ERROR("Relative paths are not allowed");
        return false;
    }

    // Only prims and properties can belong to a collection.
    if (!path.IsPrimPath() && !path.IsPropertyPath())
        return false;

    // Check if there's a direct entry in the path-expansionRule map.
    const auto i = _pathExpansionRuleMap.find(path);
    if (i != _pathExpansionRuleMap.end()) {
        if (expansionRule) {
            *expansionRule = i->second;
        }
        return i->second != UsdTokens->exclude;
    }

    // There's no direct-entry, so decide based on the parent path's 
    // expansion-rule.
    if (path.IsPrimPath()) {
        bool parentIsExcludedOrExplicitlyIncluded = 
            (parentExpansionRule == UsdTokens->exclude ||
             parentExpansionRule == UsdTokens->explicitOnly);

        if (expansionRule) {
            *expansionRule = parentIsExcludedOrExplicitlyIncluded ? 
                UsdTokens->exclude : parentExpansionRule;
        }

        return !parentIsExcludedOrExplicitlyIncluded;

    } else {
        // If it's a property path, then the path is excluded unless its 
        // parent-path's expansionRule is "expandPrimsAndProperties".
        if (expansionRule) {
            *expansionRule = 
                (parentExpansionRule == UsdTokens->expandPrimsAndProperties) ?
                UsdTokens->expandPrimsAndProperties : UsdTokens->exclude;
        }
        if (parentExpansionRule == UsdTokens->expandPrimsAndProperties) {
            return true;
        }
    }
    return false;
}

bool
Usd_CollectionMembershipQueryBase::_HasEmptyRuleMap() const
{
    return _pathExpansionRuleMap.empty();
}

size_t
Usd_CollectionMembershipQueryBase::_Hash::operator()(
    Usd_CollectionMembershipQueryBase const& q) const
{
    TRACE_FUNCTION();

    // Hashing unordered maps is costly because two maps holding the
    // same (key,value) pairs may store them in a different layout,
    // due to population history.  We must use a history-independent
    // order to compute a consistent hash value.
    //
    // If the runtime cost becomes problematic, we should consider
    // computing the hash once and storing it in the MembershipQuery,
    // as a finalization step in _ComputeMembershipQueryImpl().
    typedef std::pair<SdfPath, TfToken> _Entry;
    std::vector<_Entry> entries(q._pathExpansionRuleMap.begin(),
                                q._pathExpansionRuleMap.end());
    std::sort(entries.begin(), entries.end());

    // Don't hash _hasExcludes because it is derived from
    // the contents of _pathExpansionRuleMap.
    return TfHash::Combine(entries, q._topExpansionRule);
}

PXR_NAMESPACE_CLOSE_SCOPE
