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
#include "pxr/imaging/hd/instanceCategoriesSchema.h"
#include "pxr/imaging/hd/lightSchema.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/trace/trace.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    HDSI_LIGHT_LINK_COLLECTION_CACHE,
    HDSI_LIGHT_LINK_VERBOSE
);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDSI_LIGHT_LINK_COLLECTION_CACHE,
        "Log cache update operations and invalidations.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDSI_LIGHT_LINK_VERBOSE,
        "Log notice processing for lights and light filters.");
}

TF_DEFINE_PUBLIC_TOKENS(HdsiLightLinkingSceneIndexTokens,
    HDSI_LIGHT_LINKING_SCENE_INDEX_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((groupPrefix, "group_"))
);

static bool
_LogDebugMsg()
{
    return TfDebug::IsEnabled(HDSI_LIGHT_LINK_COLLECTION_CACHE) ||
           TfDebug::IsEnabled(HDSI_LIGHT_LINK_VERBOSE);
}

///////////////////////////////////////////////////////////////////////////////
//
//                HdsiLightLinkingSceneIndex_Impl
//
///////////////////////////////////////////////////////////////////////////////

namespace HdsiLightLinkingSceneIndex_Impl
{

// Cache of light linking collections discovered on light and light filter
// prims that tracks the correspondence of collection paths, their membership
// expressions and the category ID assigned to each unique expression.
//
// Collections that have the same membership expression are assigned the same
// category ID. For efficiency, trivial expressions that include
// all prims in the scene are not tracked by the cache.
//
struct _Cache
{
public:
    using _Expr = SdfPathExpression;
    using _CategoryId = TfToken;
    using _Eval = HdCollectionExpressionEvaluator;

    _Cache(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : _si(inputSceneIndex)
    {}

    // Updates tables and adds invalidation notices when processing a new
    // collection or when its expression has changed.
    // 
    void UpdateCollection(
        const SdfPath &primPath,
        const TfToken &collectionName,
        const SdfPathExpression &expr,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
    {
        TRACE_FUNCTION();

        const SdfPath collectionPath =
            _MakeCollectionPath(primPath, collectionName);

        // Have we seen this collection before?
        const auto pathIdEntry = _collectionPathToId.find(collectionPath);
        const bool collectionExists =
            (pathIdEntry != _collectionPathToId.end());
        
        if (collectionExists) {
            // Yes, we have. Has the expression changed?
            const auto &id = pathIdEntry->second;
            if (!TF_VERIFY(!id.IsEmpty())) {
                return;
            }

            const auto idExprEntry = _idToExpr.find(id);
            if (!TF_VERIFY(idExprEntry != _idToExpr.end())) {
                return;
            }
            const auto &curExpr = idExprEntry->second;

            if (curExpr == expr) {
                TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                    "* UpdateCollection -- Membership expression for %s has "
                    "not changed (%s).\n",
                    collectionPath.GetText(), expr.GetText().c_str());

                return;
            }
        }

        // Two possibilities here:
        // 1. Collection exists and expression has changed. The new expression
        //    may be trivial.
        //
        // 2. We're processing the collection for the first time. It may be
        //    trivial. While _PrimsAdded short circuits trivial expressions,
        //    _PrimsDirtied doesn't.
        //
        if (collectionExists) {
            // Remove the collection from all tables. This in turn adds
            // invalidation notices for affected targets.
            _RemoveCollection(collectionPath, dirtiedEntries);

            // Notify the light to re-query its category ID.
            _InvalidateLight(primPath, collectionName, dirtiedEntries);
        }

        if (IsTrivial(expr)) {
            // Nothing more to do. Trivial collection's are not tracked.
            TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                "* UpdateCollection -- Expression for %s is trivial.\n",
                collectionPath.GetText());

            return;
        }

        // Have we seen this expression before?
        auto & [id, eval] = _exprToIdAndEval[expr];
        const bool isNewExpr = id.IsEmpty();
        if (isNewExpr) {
            // Nope. Assign a category ID and ...
            id = _GetNewCategoryId();
            _idToExpr[id] = expr;

            if (_LogDebugMsg()) {
                TfDebug::Helper().Msg(
                "* UpdateCollection -- Assigned ID %s for collection %s "
                "(expression = %s).\n",
                id.GetText(), collectionPath.GetText(), expr.GetText().c_str());
            }

            // ... create evaluator.
            eval = _MakePathExpressionEvaluator(expr);

        } else {
            if (_LogDebugMsg()) {
                TfDebug::Helper().Msg(
                "* UpdateCollection -- Using shared ID %s for collection %s.\n",
                id.GetText(), collectionPath.GetText());
            }
        }

        _collectionPathToId[collectionPath] = id;
        _idToCollectionPaths[id].insert(collectionPath);
        
        // The categories on the targets don't change when using a shared ID.
        // We still conservatively invalidate the targets to give them a chance
        // to process any changes to internal lighting state.
        // This is necessary for e.g. in hdPrman to update the
        // lighting_subset & lightfilter_subset attributes on geometry.
        // We do a similar conservative invalidation in _RemoveCollection.
        const SdfPathVector targets = _ComputeAllMatches(eval);
        _InvalidateCategoriesOnTargets(targets, dirtiedEntries);

        // If the collection existed, we pushed a dirty notice for the light
        // above.
        if (!collectionExists) {
            _InvalidateLight(primPath, collectionName, dirtiedEntries);
        }
    }

    // See _RemoveCollection.
    void RemoveCollection(
        const SdfPath &primPath,
        const TfToken &collectionName,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
    {
        const SdfPath collectionPath =
            _MakeCollectionPath(primPath, collectionName);

        _RemoveCollection(collectionPath, dirtiedEntries);
    }

    // Returns the categories that `primPath` belongs to.
    // XXX Support is currently limited to non-instanced geometry prims.
    //
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
        for (const auto & [expr, idAndEval] : _exprToIdAndEval) {
            if (idAndEval.eval.Match(primPath)) {
                categories.push_back(idAndEval.id);
            }
        }

        return categories;
    }

    // Returns true and updates `id` if the cache has an entry for the given
    // collection on the light or light filter prim.
    // Returns false otherwise (trivial collections).
    //
    bool
    GetCategoryIdForLightLinkingCollection(
        const SdfPath &primPath,
        const TfToken &collectionName,
        TfToken *id)
    {
        const SdfPath colPath = _MakeCollectionPath(primPath, collectionName);
        const auto pathIdEntry = _collectionPathToId.find(colPath);
        if (pathIdEntry != _collectionPathToId.end()) {
            *id = pathIdEntry->second;
            return true;
        }

        // Collection entry doesn't exist. This is likely because the collection
        // is trivial, but also the case when the collection doesn't exist.
        return false;
    }

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
    // Updates the various tables to remove any reference to `collectionPath`.
    // Adds notices to `dirtiedEntries` to invalidate the categories on
    // prims targeted by the collection.
    //
    // Note: The light/light filter isn't invalidated here. We do this
    //       only in UpdateCollection if necessary.
    //       For prim removals, we don't need to invalidate the prim since it
    //       is being removed.
    //
    void _RemoveCollection(
        const SdfPath &collectionPath,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
    {
        const auto pathIdEntry = _collectionPathToId.find(collectionPath);
        if (pathIdEntry == _collectionPathToId.end()) {
            // Nothing to do. The collection was never added, either because
            // it didn't exist, or because it was trivial.
            return;
        }

        TRACE_FUNCTION();

        if (_LogDebugMsg()) {
            TfDebug::Helper().Msg(
            "* RemoveCollection %s -- \n"
            "   * Removing cache entries referencing the collection.\n",
            collectionPath.GetText());
        }

        TfToken id = pathIdEntry->second;
        if (!TF_VERIFY(!id.IsEmpty())) {
            return;
        }
        _collectionPathToId.erase(pathIdEntry);

        auto idPathsEntry = _idToCollectionPaths.find(id);
        if (!TF_VERIFY(idPathsEntry != _idToCollectionPaths.end())) {
            return;
        }
        auto &collectionsUsingId = idPathsEntry->second;
        collectionsUsingId.erase(collectionPath);

        // Check if the category ID is being shared by other collections.
        const bool sharingId = !collectionsUsingId.empty();

        // XXX If the ID is being shared, the categories on targeted prims
        //     don't change, but we invalidate them nonetheless.
        //     See note in _UpdateCollection.
        const auto idExprEntry = _idToExpr.find(id);
        if (!TF_VERIFY(idExprEntry != _idToExpr.end())) {
            return;
        }
        const auto expr = idExprEntry->second;

        const auto exprIdEvalEntry = _exprToIdAndEval.find(expr);
        if (!TF_VERIFY(exprIdEvalEntry != _exprToIdAndEval.end())) {
            return;
        }

        auto &eval = exprIdEvalEntry->second.eval;
        const SdfPathVector targets = _ComputeAllMatches(eval);
        _InvalidateCategoriesOnTargets(targets, dirtiedEntries);

        if (sharingId) {
            TF_DEBUG(HDSI_LIGHT_LINK_VERBOSE).Msg(
                "   * Id (%s) for collection %s is still being used by %zu "
                "other collections.\n", id.GetText(), collectionPath.GetText(), 
                collectionsUsingId.size());

        } else {
            // Remove reference to `id` from tables.
            //
            if (_LogDebugMsg()) {
                TfDebug::Helper().Msg(
                "   * Removing cache entries referencing Id (%s).\n",
                id.GetText());
            }

            _idToCollectionPaths.erase(idPathsEntry);
            _idToExpr.erase(idExprEntry);
            _exprToIdAndEval.erase(exprIdEvalEntry);
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
        if (_LogDebugMsg()) {
            TfDebug::Helper().Msg(
            "   * Invalidating categories on %zu targets ....\n",
            targets.size());
        }

        for (const auto targetPath : targets) {
            dirtiedEntries->push_back(
                HdSceneIndexObserver::DirtiedPrimEntry(
                    targetPath, HdCategoriesSchema::GetDefaultLocator()));
        }
    }

    void
    _InvalidateLight(
        const SdfPath &primPath,
        const TfToken &collectionName,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtiedEntries)
    {
        if (_LogDebugMsg()) {
            TfDebug::Helper().Msg(
            "   * Invalidating category ID for %s:%s....\n",
            primPath.GetText(), collectionName.GetText());
        }

        dirtiedEntries->push_back(
            HdSceneIndexObserver::DirtiedPrimEntry(
                primPath,
                HdLightSchema::GetDefaultLocator().Append(collectionName)));
    }

    SdfPathVector
    _ComputeAllMatches(const _Eval &eval)
    {
        constexpr auto matchKind =
            HdCollectionExpressionEvaluator::MatchAll;

        SdfPathVector resultVec;
        // XXX This doesn't support instancing.
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

    static SdfPath
    _MakeCollectionPath(
        const SdfPath &primPath,
        const TfToken &collectionName)
    {
        return primPath.AppendProperty(collectionName);
    }

private:
    const HdSceneIndexBaseRefPtr _si;

    struct _CategoryIdAndEval
    {
        _CategoryId id;
        _Eval eval;
    };

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
    using _CollectionPathToCategoryIdMap =
        std::unordered_map<SdfPath, _CategoryId, TfHash>;

    // If several collections have the same membership expression, we share
    // the assigned category ID amongst them. This map tracks that association.
    //
    using _CategoryIdToCollectionPathsMap
        = std::unordered_map<_CategoryId, SdfPathSet, TfHash>;

    _ExprToCategoryIdAndEvalMap _exprToIdAndEval;
    _CategoryIdToExprMap _idToExpr;
    _CollectionPathToCategoryIdMap _collectionPathToId;
    _CategoryIdToCollectionPathsMap _idToCollectionPaths;

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

void
_AddIfNecessary(
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

// -----------------------------------------------------------------------------
// Data source overrides.
// -----------------------------------------------------------------------------

// Prim data source wrapper for geometry prims that provides a container
// overlay for the 'categories' locator.
// 
class _GprimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_GprimDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _inputPrimDs->GetNames();
        _AddIfNecessary(HdCategoriesSchemaTokens->categories, &names);
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _inputPrimDs->Get(name);

        if (name == HdCategoriesSchemaTokens->categories) {
            // XXX This can result in categories computed upstream showing up
            //     with scene delegates that implement light linking _and_
            //     transport the linking collections. Note that this is not
            //     the case with UsdImagingDelegate.
            //     Compare with lights and light filters, where we always
            //     provide a category ID when the collection is transported.
            //     See _LightDataSource below.
            //
            return HdOverlayContainerDataSource::OverlayedContainerDataSources(
                _BuildCategoriesDataSource(_cache, _primPath),
                HdContainerDataSource::Cast(result));
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
        _AddIfNecessary(
            HdInstanceCategoriesSchemaTokens->instanceCategories, &names);
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _inputPrimDs->Get(name);

        // XXX No support for instancing (instanceCategories) yet.
        // if (name == HdInstanceCategoriesSchemaTokens->instanceCategories) {
        //     return result;
        // }

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
                _AddIfNecessary(name, &names);
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

// XXX hd/tokens.h doesn't have a separate list of light types.
//     Perhaps, we should add one and reference that here?
//
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
        if (_IsGeometry(prim.primType)) {
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
        }
    }

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

            _cache->UpdateCollection(
                entry.primPath,
                colName,
                expr,
                dirtiedEntries);
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
                _cache->RemoveCollection(
                    trackedPrimPath, colName, &dirtiedEntries);
            }
        }

        _lightAndFilterPrimPaths.erase(begin, end);
    }

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

                _cache->UpdateCollection(
                    primPath,
                    collectionName,
                    expr,
                    &newEntries);
                
            } else {
                // XXX Issue warning? We do always expect a value
                //     for the locator. Invoke RemoveCollections to clean
                //     up?
            }
        }
    }

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