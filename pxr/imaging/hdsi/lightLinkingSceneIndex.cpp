//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdsi/lightLinkingSceneIndex.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/collectionPredicateLibrary.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

// Schemata
#include "pxr/imaging/hd/categoriesSchema.h"
#include "pxr/imaging/hd/collectionSchema.h"
#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/dependencySchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/instanceCategoriesSchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/lightSchema.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/trace/trace.h"

#include <optional>
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    HDSI_LIGHT_LINK_COLLECTION_CACHE,
    HDSI_LIGHT_LINK_INVALIDATION,
    HDSI_LIGHT_LINK_VERBOSE
);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDSI_LIGHT_LINK_COLLECTION_CACHE,
        "Log cache update operations.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDSI_LIGHT_LINK_INVALIDATION,
        "Log invalidation of prims.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDSI_LIGHT_LINK_VERBOSE,
        "Enable additional logging.");
}

TF_DEFINE_PUBLIC_TOKENS(HdsiLightLinkingSceneIndexTokens,
    HDSI_LIGHT_LINKING_SCENE_INDEX_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((groupPrefix, "group_"))
);

namespace {

// A collection is identified by the prim path and the collection name token.
// Since the prim path is unique, we're guaranteed a unique ID for each
// collection.
using _CollectionId = std::pair<SdfPath, TfToken>;
using _CollectionIdSet =
    std::unordered_set<_CollectionId, TfHash>;

inline _CollectionId
_MakeCollectionId(const SdfPath &primPath, const TfToken &colName)
{
    return {primPath, colName};
}

std::string
_ToStr(const _CollectionId &id)
{
    const auto &[path, token] = id;
    return path.GetString() + "." + token.GetString();
}

} // anon

///////////////////////////////////////////////////////////////////////////////
//
//                HdsiLightLinkingSceneIndex_Impl
//
///////////////////////////////////////////////////////////////////////////////

namespace HdsiLightLinkingSceneIndex_Impl
{

/// Cache of light linking collections discovered on light and light filter
/// prims that tracks the correspondence of collection paths, their membership
/// expressions and the category ID assigned to each unique expression.
///
/// Collections that have the same membership expression are assigned the same
/// category ID. For efficiency, trivial expressions that include
/// all prims in the scene are not tracked by the cache.
///
struct _Cache
{
public:
    using _Expr = SdfPathExpression;
    using _CategoryId = TfToken;
    using _Eval = HdCollectionExpressionEvaluator;

    _Cache(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : _si(inputSceneIndex)
    {}

    /// Updates tables and dirty state for the provided collection and
    /// expression.
    /// 
    void ProcessCollection(
        const SdfPath &primPath,
        const TfToken &collectionName,
        const SdfPathExpression &expr)
    {
        TRACE_FUNCTION();

        const _CollectionId collectionId = 
            _MakeCollectionId(primPath, collectionName);

        const auto colIdEntry = _collectionIdToCategoryId.find(collectionId);
        const bool collectionExists =
            (colIdEntry != _collectionIdToCategoryId.end());

        if (collectionExists) {
            // Yes, we have. Has the expression changed?
            const auto &categoryId = colIdEntry->second;
            if (!TF_VERIFY(!categoryId.IsEmpty())) {
                return;
            }

            const auto idExprEntry = _categoryIdToExpr.find(categoryId);
            if (!TF_VERIFY(idExprEntry != _categoryIdToExpr.end())) {
                return;
            }
            const auto &oldExpr = idExprEntry->second;

            if (oldExpr == expr) {
                TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                    "* ProcessCollection -- Membership expression for %s has "
                    "not changed (%s).\n",
                    _ToStr(collectionId).c_str(), expr.GetText().c_str());

                return;
            }

            // Expression has changed.Remove table entries for the existing 
            // collection and queue invalidation.
            _RemoveCollection(
                collectionId, _InvalidationType::DirtyTargetsAndCollection);

            _dirtyState.push_back({oldExpr, collectionId});

        }

        if (IsTrivial(expr)) {
            TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                "* ProcessCollection -- Expression for %s is trivial.\n",
                _ToStr(collectionId).c_str());

            return;
        }

        // Have we seen this expression before?
        auto & [categoryId, eval] = _exprToCategoryIdAndEval[expr];
        const bool isNewExpr = categoryId.IsEmpty();
        if (isNewExpr) {
            // Nope. Assign a category ID and ...
            categoryId = _GetNewCategoryId();
            _categoryIdToExpr[categoryId] = expr;

            TF_DEBUG(HDSI_LIGHT_LINK_COLLECTION_CACHE).Msg(
                "* ProcessCollection -- Assigned ID %s for collection %s"
                "(expression = %s).\n", categoryId.GetText(),
                 _ToStr(collectionId).c_str(), expr.GetText().c_str());

            // ... create evaluator.
            eval = _MakePathExpressionEvaluator(expr);

        } else {
            TF_DEBUG(HDSI_LIGHT_LINK_COLLECTION_CACHE).Msg(
                "* ProcessCollection -- Using shared ID %s for collection %s."
                "\n", categoryId.GetText(), _ToStr(collectionId).c_str());
        }

        _collectionIdToCategoryId[collectionId] = categoryId;
        _categoryIdToCollectionIds[categoryId].insert(collectionId);

        _dirtyState.push_back({expr, collectionId});
    }

    /// Updates the various tables to remove any reference to \p collectionId.
    /// Updates dirty state to invalidate the targets for the removed
    /// collection.
    ///
    void RemoveCollection(
        const SdfPath &primPath,
        const TfToken &collectionName)
    {
        _RemoveCollection(
            _MakeCollectionId(primPath, collectionName),
            _InvalidationType::DirtyTargets);
    }

    /// Returns the categories that \p primPath belongs to.
    ///
    TfTokenVector
    ComputeCategoriesForPrimPath(
        const SdfPath &primPath) const
    {
        TRACE_FUNCTION();

        // Evaluate all expressions against `primPath`. We expect the number
        // of unique expressions to be small. If this is a hotspot, we may
        // need to cache results, or modify the matching behavior (e.g.
        // compute shallow matches and use a flattening scene index to
        // waterfall results).
        //
        TfTokenVector categories;
        for (const auto & [expr, idAndEval] : _exprToCategoryIdAndEval) {
            const auto &[categoryId, eval] = idAndEval;
            if (eval.Match(primPath)) {
                categories.push_back(categoryId);
            }
        }

        return categories;
    }

    /// Returns true and updates \p categoryId if the cache has an entry for 
    /// the provided collection.
    /// Returns false otherwise (for trivial or untracked collections).
    ///
    bool
    GetCategoryIdForLightLinkingCollection(
        const SdfPath &primPath,
        const TfToken &collectionName,
        TfToken *categoryId)
    {
        const auto entry = _collectionIdToCategoryId.find(
            _MakeCollectionId(primPath, collectionName));

        if (entry != _collectionIdToCategoryId.end()) {
            *categoryId = entry->second;
            return true;
        }

        return false;
    }

    /// Processes the queued dirty state and updates \p dirtiedEntries to
    /// invalidate targeted prims and/or lights.
    ///
    void
    InvalidatePrimsAndClearDirtyState(
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
    {
        if (!dirtiedEntries) {
            TF_CODING_ERROR("Null dirty notice vector provided\n");
            return;
        }

        if (_dirtyState.empty()) {
            return;
        }

        TRACE_FUNCTION();

        // Gather the set of unique expressions and collections to invalidate.
        // XXX For now, we conservatively invalidate the union of all queued 
        //     expressions.
        //     We can consult the tables to skip expressions in certain 
        //     scenarios. 
        //     - The lightLink collection on N lights have the same category
        //       id. We edit the lightLink expression on one of the lights to
        //       the trivial expression '//'.
        //       We don't need to evaluate the expression to invalidate the 
        //       targets in this scenario since their categories are unaffected.
        //
        //     - The lightLink collection on light A shares the same category
        //       id as the shadowLink collection on light B. If we edit the
        //       lightLink expression, we shouldn't need to invalidate the
        //       targets since the category id is still relevant to them.
        //
        //       However, geometry prims in PRMan use a dedicated attribute
        //       "lighting:subset", which needs to be updated in this scenario.
        //       So, it is possibly renderer dependent and should perhaps be
        //       a configurable thing on the scene index at some point.
        //         
        using _ExprSet = std::unordered_set<_Expr, TfHash>;
        _ExprSet exprs;
        _CollectionIdSet collectionIds;

        for (const auto &[expr, optColId] : _dirtyState) {
            exprs.insert(expr);

            if (optColId) {
                collectionIds.insert(*optColId);
            }
        }

        // Evaluating an expression over a scene index can be expensive if
        // several prims need to be traversed.
        // Compute the unioned expression to evaluate (and thus traverse) 
        // just the once.
        //
        SdfPathExpression combinedExpr;
        for (const auto &expr : exprs) {
            combinedExpr = SdfPathExpression::MakeOp(
                SdfPathExpression::Union, combinedExpr, expr);
        }
        TF_DEBUG(HDSI_LIGHT_LINK_INVALIDATION).Msg(
                "Combined expression from %zu dirty expressions: %s\n",
                exprs.size(), combinedExpr.GetText().c_str());

        const auto eval = _MakePathExpressionEvaluator(combinedExpr);
        SdfPathVector targets = _ComputeAllMatches(eval);
        _InvalidateCategoriesOnTargets(targets, dirtiedEntries);
        _InvalidateLights(collectionIds, dirtiedEntries);

        _dirtyState.clear();
    }
    
    /// Returns whether the provided expression is trivial meaning that all
    /// prims in the scene are targeted (illumniated or cast shadows for
    /// light linking).
    ///
    static bool
    IsTrivial(const SdfPathExpression &expr)
    {
        // When using explicit path-based rules with includeRoot = 1, the
        // computed path expression matches all prim paths but not properties.
        //
        static const SdfPathExpression everythingButProperties("~//*.*");

        return expr == SdfPathExpression::Everything() ||
               expr == everythingButProperties;
    }

private:
    enum class _InvalidationType {
        DirtyTargets,
        DirtyTargetsAndCollection
    };

    /// Updates the various tables to remove any reference to \p collectionId.
    /// Updates the dirty state if \p invalidateTargets is true.
    ///
    void _RemoveCollection(
        const _CollectionId &collectionId,
        _InvalidationType invalidationType)
    {
        const auto colIdEntry = _collectionIdToCategoryId.find(collectionId);
        if (colIdEntry == _collectionIdToCategoryId.end()) {
            // Nothing to do. The collection was never added, either because
            // it didn't exist, or because it was trivial.
            return;
        }

        TRACE_FUNCTION();

        TF_DEBUG(HDSI_LIGHT_LINK_COLLECTION_CACHE).Msg(
            "* RemoveCollection %s -- \n"
            "   * Removing cache entries referencing the collection.\n",
            _ToStr(collectionId).c_str());

        TfToken categoryId = colIdEntry->second;
        if (!TF_VERIFY(!categoryId.IsEmpty())) {
            return;
        }
        _collectionIdToCategoryId.erase(colIdEntry);

        auto idPathsEntry = _categoryIdToCollectionIds.find(categoryId);
        if (!TF_VERIFY(idPathsEntry != _categoryIdToCollectionIds.end())) {
            return;
        }
        auto &collectionsUsingId = idPathsEntry->second;
        collectionsUsingId.erase(collectionId);

        // Check if the category ID is being shared by other collections.
        const bool sharingId = !collectionsUsingId.empty();

        const auto idExprEntry = _categoryIdToExpr.find(categoryId);
        if (!TF_VERIFY(idExprEntry != _categoryIdToExpr.end())) {
            return;
        }
        const auto expr = idExprEntry->second;

        const auto exprIdEvalEntry = _exprToCategoryIdAndEval.find(expr);
        if (!TF_VERIFY(exprIdEvalEntry != _exprToCategoryIdAndEval.end())) {
            return;
        }

        if (sharingId) {
            TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                "   * Id (%s) for collection %s is still being used by %zu "
                "other collections.\n", categoryId.GetText(),
                _ToStr(collectionId).c_str(), collectionsUsingId.size());

        } else {
            // Remove reference to the categoryId from tables.
            TF_DEBUG(HDSI_LIGHT_LINK_COLLECTION_CACHE).Msg(
                "   * Removing cache entries referencing Id (%s).\n",
                categoryId.GetText());

            _categoryIdToCollectionIds.erase(idPathsEntry);
            _categoryIdToExpr.erase(idExprEntry);
            _exprToCategoryIdAndEval.erase(exprIdEvalEntry);
        }

        switch (invalidationType) {
        case _InvalidationType::DirtyTargets:
            _dirtyState.push_back({expr, {}});
            break;
        case _InvalidationType::DirtyTargetsAndCollection:
            _dirtyState.push_back({expr, collectionId});
            break;
        // Skip default case to get a compile-time error for unhandled values.
        }
    }

    TfToken
    _GetNewCategoryId()
    {
        std::string strId =
            _tokens->groupPrefix.GetString() + std::to_string(_groupIdx++);
        
        // We expect the number of unique expressions to be in the 100s,
        // not 2^64.
        if (_groupIdx == 0) {
            TF_CODING_ERROR(
                "Overflow detected when computing the category ID.\n");
        }
        return TfToken(strId);
    }

    void
    _InvalidateCategoriesOnTargets(
        const SdfPathVector &targets,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
    {
        TF_DEBUG(HDSI_LIGHT_LINK_INVALIDATION).Msg(
            "   * Invalidating categories on %zu targets ....\n",
            targets.size());

        for (const auto targetPath : targets) {
            dirtiedEntries->push_back(
                HdSceneIndexObserver::DirtiedPrimEntry(
                    targetPath, HdCategoriesSchema::GetDefaultLocator()));
        }
    }

    void
    _InvalidateLights(
        const _CollectionIdSet &collectionIds,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
    {
        TF_DEBUG(HDSI_LIGHT_LINK_INVALIDATION).Msg(
            "   * Invalidating category ID for %zu collections...\n",
            collectionIds.size());

        for (const auto &collectionId : collectionIds) {
            const auto &[primPath, collectionName] = collectionId;

            // XXX Currently, light linking collections are bundled under
            //     HdLightSchema with the collection name as the key
            //     and categoryId as value.
            dirtiedEntries->push_back(
                HdSceneIndexObserver::DirtiedPrimEntry(
                    primPath,
                    HdLightSchema::GetDefaultLocator().Append(collectionName)));

            TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                "       - Invalidating category ID for %s.\n",
                _ToStr(collectionId).c_str());
        }
    }

    static SdfPathVector
    _ComputeAllMatches(const _Eval &eval)
    {
        constexpr auto matchKind =
            HdCollectionExpressionEvaluator::MatchAll;

        SdfPathVector resultVec;
        // XXX This doesn't support instance proxy traversal.
        eval.PopulateMatches(
            SdfPath::AbsoluteRootPath(), matchKind, &resultVec);
        
        return resultVec;
    }

    HdCollectionExpressionEvaluator
    _MakePathExpressionEvaluator(
        const SdfPathExpression &expr) const
    {
        // XXX For now, use the base set of predicates that Hydra ships with.
        //     If this needs to be configured for an application, then the
        //     light linking scene index would need to be registered using the
        //     callback registration mechanism rather than the plugin registry.
        //
        return HdCollectionExpressionEvaluator(
            _si, expr, HdGetCollectionPredicateLibrary());
    }

private:
    const HdSceneIndexBaseRefPtr _si;

    // -------------------------------------------------------------------------
    // Tables
    //
    using _CategoryIdAndEval = std::pair<_CategoryId, _Eval>;

    // Lookup the category ID and evaluator for a given expression.
    //
    using _ExprToCategoryIdAndEvalMap =
        std::unordered_map<_Expr, _CategoryIdAndEval, TfHash>;

    // Reverse lookup to get the expression given the category ID.
    // This may seem redundant, but is useful when processing collection
    // invalidation to check if the expression has changed for a collection
    // tracked by the cache.
    //
    using _CategoryIdToExprMap = std::unordered_map<_CategoryId, _Expr, TfHash>;

    // For each collection path (of the form primPath.collectionName), lookup
    // the assigned category ID.
    //
    using _CollectionIdToCategoryIdMap =
        std::unordered_map<_CollectionId, _CategoryId, TfHash>;

    // If several collections have the same membership expression, we share
    // the assigned category ID amongst them. This map tracks that association.
    //
    using _CategoryIdToCollectionIds
        = std::unordered_map<_CategoryId, _CollectionIdSet, TfHash>;

    _ExprToCategoryIdAndEvalMap _exprToCategoryIdAndEval;
    _CategoryIdToExprMap _categoryIdToExpr;
    _CollectionIdToCategoryIdMap _collectionIdToCategoryId;
    _CategoryIdToCollectionIds _categoryIdToCollectionIds;

    // -------------------------------------------------------------------------
    // Dirty state
    //
    using _OptionalCollectionId = std::optional<_CollectionId>;
    using _DirtyEntry = std::pair<_Expr, _OptionalCollectionId>;
    using _DirtyState = std::vector<_DirtyEntry>;

    _DirtyState _dirtyState;

    // -------------------------------------------------------------------------

    // Suffix used when computing the next group (category) ID.
    size_t _groupIdx = 0;
};

} // namespace HdsiLightLinkingSceneIndex_Impl

using namespace HdsiLightLinkingSceneIndex_Impl;

namespace {

// Instance names of the collection's applied in UsdLux.
const TfTokenVector&
_GetAllLinkingCollectionNames()
{
    static const TfTokenVector names = {
        HdTokens->lightLink,
        HdTokens->shadowLink,
        HdTokens->filterLink 
    };

    return names;
}

// HdLightSchema is barebones at the moment, but that's where the linking
// tokens are housed under.
const TfTokenVector&
_GetLightLinkingSchemaTokens()
{
    static const TfTokenVector names = {
        HdTokens->lightLink,
        HdTokens->shadowLink,
        HdTokens->lightFilterLink // not filterLink!
    };

    return names;
}

// Returns the value of the token array data source from the container if
// available. Otherwise, returns `fallback`.
//
VtArray<TfToken>
_GetPrimTypes(
    const HdContainerDataSourceHandle &inputArgs,
    const TfToken &argName,
    const VtArray<TfToken> &fallback)
{
    if (!inputArgs) {
        return fallback;
    }

    if (HdTokenArrayDataSourceHandle tokenArrayHandle =
            HdTokenArrayDataSource::Cast(inputArgs->Get(argName))) {

        return tokenArrayHandle->GetTypedValue(0);
    }

    return fallback;
}

// Returns whether `tokens` contains `key`. 
bool
_Contains(
    const VtArray<TfToken> &tokens,
    const TfToken &key)
{
    return std::find(tokens.cbegin(), tokens.cend(), key) != tokens.cend();
}

bool
_Contains(
    const TfTokenVector &tokens,
    const TfToken &key)
{
    return std::find(tokens.cbegin(), tokens.cend(), key) != tokens.cend();
}

bool _IsInstanced(
    const HdContainerDataSourceHandle &primContainer)
{
    const auto schema = HdInstancedBySchema::GetFromParent(primContainer);
    const auto pathArrayDs = schema.GetPaths();
    return pathArrayDs && !pathArrayDs->GetTypedValue(0.0).empty();
}

void
_AddIfAbsent(
    const TfToken &token,
    TfTokenVector *tokens) 
{
    if (!_Contains(*tokens, token)) {
        tokens->push_back(token);
    }
}

// Queries the cache to compute the (light linking) categories that include
// `primPath` and returns a container data source with the result.
//
HdContainerDataSourceHandle
_BuildCategoriesDataSource(
    const _CacheSharedPtr &cache,
    const SdfPath &primPath)
{
    const TfTokenVector categories =
        cache->ComputeCategoriesForPrimPath(primPath);

    if (categories.empty()) {
        return nullptr;
    }
    
    return HdCategoriesSchema::BuildRetained(
        categories.size(), categories.data(),
        /*excludedNameCount = */ 0,
        /*excludedNames = */nullptr);
}

// Queries the cache to compute the categories for each *direct* instance
// of the instancer. Returns a container data source with the result.
//
// XXX The approach below works only for linking to direct instances of a
// non-nested instancer.
// It does not support linking to
// - instance proxy prims
// - nested instances
//
HdContainerDataSourceHandle
_BuildInstanceCategoriesDataSource(
    const _CacheSharedPtr &cache,
    const SdfPath &instancerPrimPath,
    const HdContainerDataSourceHandle &instancerPrimDs)
{
    // Use the instanceLocations data source under the instancerTopology to
    // query the instances for the provided instancer.
    //
    // XXX This is populated only for native instancing instancer prims,
    //     so this doesn't handle point instancing instancer prims.
    //     By using only the instance's path, we don't handle instance proxy
    //     matches.
    //     Note that the scene delegate API as well as the instance categories
    //     schema doesn't cater to specifying categories per instance-prototype
    //     tuple.
    //     HdSceneDelegate::GetInstanceCategories returns a 
    //     std::vector<VtArray<TfToken>> indexed by the instance idx.
    //
    const HdInstancerTopologySchema topologySchema =
            HdInstancerTopologySchema::GetFromParent(instancerPrimDs);

    const HdPathArrayDataSourceHandle instancePathsDs =
        topologySchema.GetInstanceLocations();
    
    if (!instancePathsDs) {
        // Point instancer.
        // We can't link to instances of a point instancer (since they
        // don't exist as prims in the scene description).
        // While we could link to prototype prims under the point instancer
        // (thereby linking all instances of that prototype), we don't
        // support this because the prototypes may exist anywhere in the
        // scene namespace.
        //
        // Linking to point instancers uses the categories data source
        // (rather than instanceCategories). The categories returned apply
        // to all its instances.
        //
        return nullptr;
    }

    const VtArray<SdfPath> instancePaths = instancePathsDs->GetTypedValue(0.0);
    if (instancePaths.empty()) {
        return nullptr;
    }

    std::vector<HdDataSourceBaseHandle> dataSources;
    dataSources.reserve(instancePaths.size());

    // XXX Brute force for now. This can be improved.
    for (const SdfPath &instancePath : instancePaths) {
        dataSources.push_back(
            _BuildCategoriesDataSource(cache, instancePath));
    }

    return
        HdInstanceCategoriesSchema::Builder()
        .SetCategoriesValues(
            HdRetainedSmallVectorDataSource::New(
                dataSources.size(), dataSources.data()))
        .Build();
}

// Add dependency from the instancer to the instance prims it serves to
// invalidate its instanceCategories locator. 
//
HdContainerDataSourceHandle
_BuildDependenciesDataSource(
    const HdContainerDataSourceHandle &instancerPrimContainer)
{
    const HdInstancerTopologySchema topologySchema =
            HdInstancerTopologySchema::GetFromParent(instancerPrimContainer);

    const HdPathArrayDataSourceHandle instancePathsDs =
        topologySchema.GetInstanceLocations();
    
    if (!instancePathsDs) {
        // XXX Point instancer. Per-instance categories does not make sense for
        //     point instancers. Should we use categories to reflect that they
        //     apply to all instances (of all prototypes)?
        return nullptr;
    }

    const VtArray<SdfPath> instancePaths = instancePathsDs->GetTypedValue(0.0);
    const size_t numInstances = instancePaths.size();
    std::vector<TfToken> names;
    names.reserve(numInstances);
    std::vector<HdDataSourceBaseHandle> dataSources;
    dataSources.reserve(numInstances);

    static const auto categoriesLoc =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdCategoriesSchema::GetDefaultLocator());
    static const auto instanceCategoriesLoc =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdInstanceCategoriesSchema::GetDefaultLocator());

    // XXX This is a bit hacky and relies on the invalidation behavior in the
    //     cache. Specifically, we rely on invalidating the categories
    //     on all prims targeted by the collection, including instance prims.
    //     
    //     We publish categories only for geometry prims and not instance
    //     prims. See HdsiLightLinkingSceneIndex::GetPrim.
    //
    size_t idx = 0;
    for (const SdfPath &instancePath : instancePaths) {
        std::string depName = "dep_" + std::to_string(idx++);
        names.push_back(TfToken(std::move(depName)));
        dataSources.push_back(
            HdDependencySchema::Builder()
            .SetDependedOnPrimPath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(instancePath))
            .SetDependedOnDataSourceLocator(categoriesLoc)
            .SetAffectedDataSourceLocator(instanceCategoriesLoc)
            .Build());
    }

    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), dataSources.data());
}

// -----------------------------------------------------------------------------
// Data source overrides.
// -----------------------------------------------------------------------------

// Prim data source wrapper for geometry prims that provides the data source
// for the 'categories' locator.
// 
class _GprimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_GprimDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _inputPrimDs->GetNames();
        _AddIfAbsent(HdCategoriesSchemaTokens->categories, &names);
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _inputPrimDs->Get(name);

        if (name == HdCategoriesSchemaTokens->categories) {
            if (HdContainerDataSourceHandle categoriesContainer =
                    _BuildCategoriesDataSource(_cache, _primPath)) {

                return categoriesContainer;
            }
        }

        return result;
    }

private:
    _GprimDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const SdfPath &primPath,
        const _CacheSharedPtr &cache)
      : _inputPrimDs(primContainer)
      , _primPath(primPath)
      , _cache(cache)
    {
    }

    const HdContainerDataSourceHandle _inputPrimDs;
    const SdfPath _primPath;
    const _CacheSharedPtr &_cache;
};

// Prim data source wrapper for instancer prims.
// Currently does nothing, but will be updated to provide an overlay for
// the 'instanceCategories' locator.
//
class _InstancerPrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstancerPrimDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _inputPrimDs->GetNames();
        // instanceCategories is relevant for (hydra) instancer prims that 
        // implement native instancing USD semantics.
        _AddIfAbsent(
            HdInstanceCategoriesSchemaTokens->instanceCategories, &names);

        // categories is relevant for (hydra) instancer prims that correspond to
        // point instancer prims; the categories returned apply to all its
        // instances.
        _AddIfAbsent(
            HdCategoriesSchemaTokens->categories, &names);

        _AddIfAbsent(
            HdDependenciesSchemaTokens->__dependencies, &names);
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdInstanceCategoriesSchemaTokens->instanceCategories) {
            if (HdContainerDataSourceHandle instanceCategoriesContainer =
                _BuildInstanceCategoriesDataSource(
                    _cache, _primPath, _inputPrimDs)) {

                return instanceCategoriesContainer;
            }
        }

        if (name == HdCategoriesSchemaTokens->categories) {
            const HdInstancerTopologySchema topologySchema =
                HdInstancerTopologySchema::GetFromParent(_inputPrimDs);
            
            const HdPathArrayDataSourceHandle instancePathsDs =
                topologySchema.GetInstanceLocations();
    
            const bool isPointInstancer =
                !instancePathsDs || instancePathsDs->GetTypedValue(0.0).empty();

            if (isPointInstancer) {
                if (HdContainerDataSourceHandle categoriesContainer =
                    _BuildCategoriesDataSource(_cache, _primPath)) {

                    return categoriesContainer;
                }
            }
        }

        HdDataSourceBaseHandle result = _inputPrimDs->Get(name);

        if (name == HdDependenciesSchemaTokens->__dependencies) {
            return
                HdOverlayContainerDataSource::OverlayedContainerDataSources(
                    _BuildDependenciesDataSource(_inputPrimDs),
                    HdContainerDataSource::Cast(result));
        }

        return result;
    }

private:
    _InstancerPrimDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const SdfPath &primPath,
        const _CacheSharedPtr &cache)
      : _inputPrimDs(primContainer)
      , _primPath(primPath)
      , _cache(cache)
    {
    }

    const HdContainerDataSourceHandle _inputPrimDs;
    const SdfPath _primPath;
    const _CacheSharedPtr &_cache;
};

class _LightDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LightDataSource);

    TfTokenVector GetNames() override
    {
        if (_lightDs) {
            TfTokenVector names = _lightDs->GetNames();
            for (const TfToken &name : _GetLightLinkingSchemaTokens()) {
                _AddIfAbsent(name, &names);
            }
            return names;
        }
        return _GetLightLinkingSchemaTokens();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const TfTokenVector &schemaTokens = _GetLightLinkingSchemaTokens();
        if (_Contains(schemaTokens, name)) {
            
            const TfToken &collectionName =
                name == HdTokens->lightFilterLink
                ? HdTokens->filterLink
                : name;
            
            // Note: Since this scene index relies on linking collections to
            //       be transported, use an overlay only when we have a
            //       collections data source on the prim to provide the category
            //       ID for the collections, including an empty token for the
            //       trivial case.
            //       For legacy scene delegates that implement light linking
            //       and don't transport collections, we leave the light
            //       data source as-is (e.g. UsdImagingDelegate).
            //
            const auto collectionsSchema =
                HdCollectionsSchema::GetFromParent(_primDs);
            
            if (collectionsSchema.GetCollection(collectionName)) {
                TfToken id;
                if (_cache->GetCategoryIdForLightLinkingCollection(
                    _primPath, collectionName, &id)) {
                    
                    return HdRetainedTypedSampledDataSource<TfToken>::New(id);
                }

                static const auto trivialIdDs =
                    HdRetainedTypedSampledDataSource<TfToken>::New(TfToken());
                return trivialIdDs;
            }
        }

        return _lightDs? _lightDs->Get(name) : nullptr;
    }

private:
    _LightDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const HdContainerDataSourceHandle &lightContainer,
        const SdfPath &primPath,
        const _CacheSharedPtr &cache)
      : _primDs(primContainer)
      , _lightDs(lightContainer)
      , _primPath(primPath)
      , _cache(cache)
    {
        TF_VERIFY(primContainer);
        // Note: lightContainer may be null.
    }

    const HdContainerDataSourceHandle _primDs;
    const HdContainerDataSourceHandle _lightDs;
    const SdfPath _primPath;
    const _CacheSharedPtr &_cache;
};


// Prim data source wrapper for light and light filter prims that provides a
// container override for the 'light' locator with the category IDs for the
// linking collections when the prim has a collections data source.
//
class _LightPrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_LightPrimDataSource);

    TfTokenVector GetNames() override
    {
        return _inputPrimDs->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _inputPrimDs->Get(name);

        if (name == HdLightSchemaTokens->light) {
            return _LightDataSource::New(
                _inputPrimDs,
                HdContainerDataSource::Cast(result), _primPath, _cache);
        }

        return result;
    }

private:
    _LightPrimDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const SdfPath &primPath,
        const _CacheSharedPtr &cache)
      : _inputPrimDs(primContainer)
      , _primPath(primPath)
      , _cache(cache)
    {
        TF_VERIFY(primContainer);
    }

    const HdContainerDataSourceHandle _inputPrimDs;
    const SdfPath _primPath;
    const _CacheSharedPtr &_cache;
};


} // anon

///////////////////////////////////////////////////////////////////////////////
//
// Scene index implementation
//
///////////////////////////////////////////////////////////////////////////////

// This includes implicit primitive types.
static const VtArray<TfToken> GEOMETRY_PRIM_TYPES = {
    HdRprimTypeTokens->allTokens.begin(),
    HdRprimTypeTokens->allTokens.end()
};

static const VtArray<TfToken> LIGHT_PRIM_TYPES = {
    HdLightTypeTokens->allTokens.begin(),
    HdLightTypeTokens->allTokens.end(),
};

static const VtArray<TfToken> LIGHT_FILTER_PRIM_TYPES = {
    HdLightFilterTypeTokens->allTokens.begin(),
    HdLightFilterTypeTokens->allTokens.end()
};

HdSceneIndexBaseRefPtr
HdsiLightLinkingSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    HdSceneIndexBaseRefPtr sceneIndex = 
        TfCreateRefPtr(
            new HdsiLightLinkingSceneIndex(inputSceneIndex, inputArgs));

    sceneIndex->SetDisplayName("Light Linking Scene Index");
    
    return sceneIndex;
}

HdsiLightLinkingSceneIndex::HdsiLightLinkingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _cache(std::make_shared<_Cache>(inputSceneIndex))
  , _lightPrimTypes(_GetPrimTypes(
        inputArgs,
        HdsiLightLinkingSceneIndexTokens->lightPrimTypes,
        LIGHT_PRIM_TYPES))
  , _lightFilterPrimTypes(_GetPrimTypes(
        inputArgs,
        HdsiLightLinkingSceneIndexTokens->lightFilterPrimTypes,
        LIGHT_FILTER_PRIM_TYPES))
  , _geometryPrimTypes(_GetPrimTypes(
        inputArgs,
        HdsiLightLinkingSceneIndexTokens->geometryPrimTypes,
        GEOMETRY_PRIM_TYPES))
{
}

HdSceneIndexPrim
HdsiLightLinkingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Since we are not using a flattening scene index approach to waterfall
    // categories to descendants, we can safely use a prim type check and
    // require a valid data source to limit the data source wrapping.
    //
    if (prim.dataSource) {
        if (_IsGeometry(prim.primType) && !_IsInstanced(prim.dataSource)) {
            prim.dataSource = _GprimDataSource::New(
                prim.dataSource, primPath, _cache);

        } else if (prim.primType == HdPrimTypeTokens->instancer) {
            prim.dataSource = _InstancerPrimDataSource::New(
                prim.dataSource, primPath, _cache);

        } else if (_IsLight(prim.primType) || _IsLightFilter(prim.primType)) {
            
            prim.dataSource = _LightPrimDataSource::New(
                prim.dataSource, primPath, _cache);
        }
    }

    return prim;
}

SdfPathVector
HdsiLightLinkingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    // This scene index doesn't change the topology.
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiLightLinkingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }
    
    TRACE_FUNCTION();

    // Notices for prims that need to refetch their categories.
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;

    for (const auto &entry : entries) {
        if (_IsLight(entry.primType)) {
            // Update internal tracking.
            _lightAndFilterPrimPaths.insert(entry.primPath);

             _ProcessAddedLightOrFilter(
                entry,
                {HdTokens->lightLink, HdTokens->shadowLink},
                &dirtiedEntries);

        } else if (_IsLightFilter(entry.primType)) {
            // Update internal tracking.
            _lightAndFilterPrimPaths.insert(entry.primPath);

            _ProcessAddedLightOrFilter(
                entry, {HdTokens->filterLink}, &dirtiedEntries);

        } else if (auto it =_lightAndFilterPrimPaths.find(entry.primPath);
            it != _lightAndFilterPrimPaths.end()) {

            // The prim is no longer a light/light filter.
            for (const TfToken &colName : _GetAllLinkingCollectionNames()) {
                _cache->RemoveCollection(entry.primPath, colName);
            }

            _lightAndFilterPrimPaths.erase(it);
        }
    }

    _cache->InvalidatePrimsAndClearDirtyState(&dirtiedEntries);

    _SendPrimsAdded(entries);
    _SendPrimsDirtied(dirtiedEntries);
}

void
HdsiLightLinkingSceneIndex::_ProcessAddedLightOrFilter(
    const HdSceneIndexObserver::AddedPrimEntry &entry,
    const TfTokenVector &collectionNames,
    HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
{
    TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
        "Processing added notice for %s.\n", entry.primPath.GetText());

    const HdSceneIndexPrim prim = 
            _GetInputSceneIndex()->GetPrim(entry.primPath);

    HdCollectionsSchema collectionsSchema =
        HdCollectionsSchema::GetFromParent(prim.dataSource);

    for (const TfToken &colName : collectionNames) {

        HdCollectionSchema colSchema =
            collectionsSchema.GetCollection(colName);

        if (const auto exprDs = colSchema.GetMembershipExpression()) {
            SdfPathExpression expr = exprDs->GetTypedValue(0.0);

            if (HdsiLightLinkingSceneIndex_Impl::_Cache::IsTrivial(expr)) {
                // If the expression is trivial, we do nothing!
                // NOTE: Compare with _PrimsDirtied.
                TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                    "   ... %s:%s is trivial. Nothing to do.\n",
                    entry.primPath.GetText(), colName.GetText());
                
                continue;
            }

            _cache->ProcessCollection(entry.primPath, colName, expr);
        }
    }
}

void 
HdsiLightLinkingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;

    for (const auto &entry : entries) {
        // Recall that all descendants of the prim are also removed...
        const auto itRange = SdfPathFindPrefixedRange(
            _lightAndFilterPrimPaths.begin(),
            _lightAndFilterPrimPaths.end(),
            entry.primPath);

        const auto &begin = itRange.first;
        if (begin == _lightAndFilterPrimPaths.end()) {
            continue;
        }

        TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
            "Processing removed notice for %s.\n", entry.primPath.GetText());

        const auto &end = itRange.second;
        for (auto it = begin; it != end; ++it) {
            const SdfPath &trackedPrimPath = *it;

            // XXX We could track lights and light filters separately to 
            // loop over only the relevant collections.
            //
            for (const TfToken &colName : _GetAllLinkingCollectionNames()) {
                _cache->RemoveCollection(trackedPrimPath, colName);
            }
        }

        _lightAndFilterPrimPaths.erase(begin, end);
    }

    _cache->InvalidatePrimsAndClearDirtyState(&dirtiedEntries);

    _SendPrimsRemoved(entries);
    _SendPrimsDirtied(dirtiedEntries);
}

void
HdsiLightLinkingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    static const HdDataSourceLocatorSet collectionLocators =
    {
        HdCollectionsSchema::GetDefaultLocator().Append(
            HdTokens->lightLink),
        HdCollectionsSchema::GetDefaultLocator().Append(
            HdTokens->shadowLink),
        HdCollectionsSchema::GetDefaultLocator().Append(
            HdTokens->filterLink),
    };

    HdSceneIndexObserver::DirtiedPrimEntries newEntries;

    for (const auto &entry : entries) {
        const SdfPath &primPath = entry.primPath;

        if (_lightAndFilterPrimPaths.find(primPath)
                == _lightAndFilterPrimPaths.end()) {
            continue;
        }

        if (!entry.dirtyLocators.Intersects(collectionLocators)) {
            continue;
        }
        
        const HdSceneIndexPrim prim = 
            _GetInputSceneIndex()->GetPrim(primPath);

        HdCollectionsSchema collectionsSchema =
            HdCollectionsSchema::GetFromParent(prim.dataSource);
        
        if (!collectionsSchema) {
            continue;
        }

        // XXX We could track lights and light filters separately to loop 
        // over only the relevant collection locators.
        //
        for (const auto &locator : collectionLocators) {
            const TfToken &collectionName = locator.GetLastElement();

            HdCollectionSchema colSchema =
                collectionsSchema.GetCollection(collectionName);
            if (!colSchema) {
                continue;
            }
            if (!entry.dirtyLocators.Intersects(locator)) {
                continue;
            }

            if (const auto exprDs = colSchema.GetMembershipExpression()) {

                TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                    "Processing dirtied notice for prim %s for "
                    " collection %s...\n",
                    primPath.GetText(), collectionName.GetText());

                // NOTE: We need to process the expression even if it
                //       is trivial because it might not have been
                //       earlier. Compare with _PrimsAdded.
                const SdfPathExpression expr =
                    exprDs->GetTypedValue(0.0);

                _cache->ProcessCollection(primPath, collectionName, expr);
                
            } else {
                // XXX Issue warning? We do always expect a value
                //     for the locator. Invoke RemoveCollections to clean
                //     up?
            }
        }
    }

    _cache->InvalidatePrimsAndClearDirtyState(&newEntries);

    _SendPrimsDirtied(entries);
    _SendPrimsDirtied(newEntries);
}

bool
HdsiLightLinkingSceneIndex::_IsLight(
    const TfToken &primType) const
{
    return _Contains(_lightPrimTypes, primType);
}

bool
HdsiLightLinkingSceneIndex::_IsLightFilter(
    const TfToken &primType) const
{
    return _Contains(_lightFilterPrimTypes, primType);
}

bool
HdsiLightLinkingSceneIndex::_IsGeometry(
    const TfToken &primType) const
{
    return _Contains(_geometryPrimTypes, primType);
}

PXR_NAMESPACE_CLOSE_SCOPE