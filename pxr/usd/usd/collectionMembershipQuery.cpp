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
                // If this prim is the stage's pseudo-root, increment the
                // range's begin iterator to skip it.  This happens when the
                // collection has 'includeRoot' set to include '/'.  This fixup
                // is necessary since the below test of query.IsPathIncluded()
                // will return false for '/' since only prims and properties can
                // be included, but that will thwart the remainder of the
                // iteration descendant to '/'.
                if (prim == stage->GetPseudoRoot()) {
                    range.increment_begin();
                }
                
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
                }
                // If there is a property path directly in the map, then it is
                // considered included even if the rule is expandPrims.
                else if (p.IsPropertyPath() ||
                         (i->second == UsdTokens->expandPrimsAndProperties) ||
                         (i->second == UsdTokens->explicitOnly && p == path)) {
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


SdfPathExpression
UsdComputePathExpressionFromCollectionMembershipQueryRuleMap(
    Usd_CollectionMembershipQueryBase::PathExpansionRuleMap const &ruleMap)
{
    // Shorthand to increase readability.
    using Expr = SdfPathExpression;

    // If there are no rules, we don't match anything.
    if (ruleMap.empty()) {
        return Expr::Nothing();
    }

    // Use an enum instead of tokens to avoid TfToken refcounting locally.
    enum Rule { ExpandPrims, ExpandPrimsAndProps, Exclude, ExplicitOnly };
    auto tokenToRule = [](TfToken const &token) {
        if (token == UsdTokens->expandPrims) { return ExpandPrims; }
        if (token == UsdTokens->
            expandPrimsAndProperties) { return ExpandPrimsAndProps; }
        if (token == UsdTokens->exclude) { return Exclude; }
        return ExplicitOnly;
    };

    // Build a lexicographically ordered list of entries to process.  This
    // ensures that we will see ancestor paths prior to descendant paths.
    //
    // During this process we also collect any explictOnly prim paths, and all
    // property paths separately.  These can all be treated as ExplicitOnly
    // entries, either included or excluded.  We combine these with the final
    // expression at the end, in such a way that they are evaluated first, since
    // they are the fastest to evaluate.
    using Entry = std::pair<SdfPath, Rule>;
    std::vector<Entry> entries;
    entries.reserve(ruleMap.size());

    // These subexpressions track explicitly included or excluded single-path
    // rules.  They are both just union chains, like '/foo /bar /baz' etc.
    Expr explicitIncludes, explicitExcludes;
    auto addUnion = [](Expr &expr, SdfPath const &path) {
        expr = Expr::MakeOp(Expr::ImpliedUnion,
                            std::move(expr), Expr::MakeAtom(path));
    };

    for (auto const &[path, ruleToken]: ruleMap) {
        Rule rule = tokenToRule(ruleToken);
        // Collect explicit-only rules separately.
        if (rule == ExplicitOnly) {
            addUnion(explicitIncludes, path);
        }
        // Property paths can always be treated as explicit since they don't
        // have descendant prims/properties.
        else if (!path.IsAbsoluteRootOrPrimPath()) {
            addUnion(
                rule == Exclude ? explicitExcludes : explicitIncludes, path);
        }
        // Other rules for prim-like paths are handled by the shift/reduce
        // builder.
        else {
            TF_AXIOM(path.IsAbsoluteRootOrPrimPath());
            entries.emplace_back(path, rule);
        }
    }
    std::sort(entries.begin(), entries.end());

    // Build the expression shift-reduce style, merging the most descendant
    // expressions into ancestral expressions.
    
    // Each stack entry contains the path for this rule as 'self', its
    // accumulated subexpression thus far (incorporating descendant rules), and
    // finally the rule for this path.
    struct StackEntry {
        StackEntry(SdfPath const &self_, Rule rule_)
            : StackEntry(SdfPath(self_), rule_) {}
        StackEntry(SdfPath &&self_, Rule rule_)
            : self(std::move(self_)), rule(rule_) {
            // Note that this cannot be a property path.  Those are handled in
            // explicitIncludes/Excludes.
            TF_AXIOM(self.IsAbsoluteRootOrPrimPath());

            // If this is an exclude of the absolute root path, our expression
            // is just Nothing.
            if (self.IsAbsoluteRootPath() && rule == Exclude) {
                return;
            }
            // Otherwise build the initial expression for this path's rule,
            // starting with the path itself.
            Expr::PathPattern pattern(self);
            pattern.AppendChild({}); // tack on '//'
            expr = Expr::MakeAtom(std::move(pattern));
            // If the rule is ExpandPrims (not properties) subtract properties.
            if (rule == ExpandPrims) {
                static Expr const &allPropsExpr = *(new Expr("//*.*"));
                expr = Expr::MakeOp(Expr::Difference,
                                    std::move(expr), Expr{allPropsExpr});
            }
            // Finally, to get correct precedence, we insert a weaker reference
            // '%_' on the left hand side joined with the operator for our rule.
            // So for an include it's like '%_ /self//' and for an exclude it's
            // like '%_ - /self//'.  The parent will compose this expression
            // over its own as the weaker reference to insert in the correct
            // spot.  But if this rule is for the absolute root, we do not
            // include a weaker reference since it's the final rule.
            if (!self.IsAbsoluteRootPath()) {
                expr = Expr::MakeOp(
                    rule == Exclude ? Expr::Difference : Expr::Union,
                    Expr::WeakerRef(), std::move(expr));
            }
        }
        SdfPath self;
        Expr expr;
        Rule rule;
    };
    std::vector<StackEntry> stack;

    // Helper function to reduce the stack top (back()) into the next
    // (ancestral) stack entry by merging its accumulated expression into the
    // ancestor, reducing the stack size by one. If the stack is not empty,
    // return the empty expression.  If the stack has only one element, return
    // the final expression and leave the stack empty.
    auto reduce = [](std::vector<StackEntry> &stack) {
        Expr topExpr = std::move(stack.back().expr);
        stack.pop_back();
        // If the stack is empty, return the final expression.
        if (stack.empty()) {
            return topExpr;
        }
        // Otherwise combine into the next entry by composing the oldTop's
        // expression over the newTop's -- replacing the oldTop's weaker
        // reference '%_' with the newTop's expression.
        StackEntry &newTop = stack.back();
        newTop.expr = topExpr.ComposeOver(newTop.expr);
        // Return empty since the expression is not yet complete.
        topExpr = {};
        return topExpr;
    };
    
    // For uniformity we want the stack top to always be '/'.  If the first
    // entry in 'entries' is not '/', it means that '/' is implicitly excluded,
    // so in that case we push an exclude of '/' manually.  Otherwise the loop
    // over entries below will push the '/' on its first iteration.  If entries
    // is empty here, it means that all paths were explicit and there are no
    // other rules.
    if (!entries.empty() && !entries.front().first.IsAbsoluteRootPath()) {
        stack.emplace_back(SdfPath::AbsoluteRootPath(), Exclude);
    }

    // Process each entry in lexicographical order, which ensures we process
    // ancestors prior to descendants.
    for (Entry &curEntry: entries) {
        // Reduce the stack until we find an ancestor of 'curEntry'.  Note that
        // this loop will never reduce stack to empty() -- it is only possibly
        // empty the first time through.
        while (!stack.empty() && !curEntry.first.HasPrefix(stack.back().self)) {
            reduce(stack);
        }
        // Push/shift this new descendant, unless it is redundant.  The rule map
        // computation sometimes produces redundant entries, like
        // excludes=[/foo, /foo/bar].  So skip this entry if it is subsumed by
        // the stack top.  This is true when top=exclude and cur=exclude, or
        // when top=expandPrims and cur=expandPrims, or when
        // top=expandPrimsAndProps and cur!=exclude.
        if (!stack.empty()) {
            Rule topRule = stack.back().rule, curRule = curEntry.second;
            if ((topRule == Exclude && curRule == Exclude) ||
                (topRule == ExpandPrims && curRule == ExpandPrims) ||
                (topRule == ExpandPrimsAndProps && curRule != Exclude)) {
                // This entry is redundant with the stack top, so skip it.  Any
                // descendant entries of this curEntry will still be processed.
                continue;
            }
        }
        // Push new descendant.
        stack.emplace_back(std::move(curEntry.first), curEntry.second);
    }
    
    // Reduce the remainder of the stack to complete building the expression.
    Expr result;
    while (!stack.empty()) {
        result = reduce(stack);
    }

    // Put it all together.  The overall final form of the expression is:
    //
    // (incl_1 + ... + incl_N) + (~(excl_1 + ... + excl_N) & result)
    //  ~~~~~~~~~~~~~~~~~~~~~       ~~~~~~~~~~~~~~~~~~~~~
    //  ^ explicitIncludes ^        ^ explicitExcludes ^
    //
    // Where 'result' is the previously shift/reduce-computed expression dealing
    // with hierarchical include/exclude rules, and 'incl/excl_1..N' are the
    // explicit includes & excludes.

    return Expr::MakeOp(Expr::ImpliedUnion,
                        std::move(explicitIncludes),
                        Expr::MakeOp(Expr::Intersection,
                                     Expr::MakeComplement(
                                         std::move(explicitExcludes)),
                                     std::move(result)));
}

PXR_NAMESPACE_CLOSE_SCOPE
