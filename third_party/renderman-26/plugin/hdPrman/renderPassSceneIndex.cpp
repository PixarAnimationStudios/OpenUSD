// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/renderPassSceneIndex.h"
#if PXR_VERSION >= 2405
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/version.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/imaging/hd/collectionSchema.h"
#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/renderPassSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/schema.h" 
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hdsi/utils.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderVisibility)
    (cameraVisibility)
    (matte)
    (prune)
    ((riAttributesRiMatte, "ri:attributes:Ri:Matte"))
    ((riAttributesVisibilityCamera, "ri:attributes:visibility:camera"))
);

/* static */
HdPrman_RenderPassSceneIndexRefPtr
HdPrman_RenderPassSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(  
        new HdPrman_RenderPassSceneIndex(inputSceneIndex));
}

HdPrman_RenderPassSceneIndex::HdPrman_RenderPassSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

static bool
_IsGeometryType(const TfToken &primType)
{
    // Additional gprim types supported by HdPrman, beyond those in
    // HdPrimTypeIsGprim().
    static const TfTokenVector extraGeomTypes = {
        HdPrimTypeTokens->cone,
        HdPrimTypeTokens->cylinder,
        HdPrimTypeTokens->sphere,
        HdPrmanTokens->meshLightSourceMesh,
        HdPrmanTokens->meshLightSourceVolume
    };
    return HdPrimTypeIsGprim(primType) ||
        std::find(extraGeomTypes.begin(), extraGeomTypes.end(), primType)
            != extraGeomTypes.end();
}

// Returns true if the renderVisibility rules apply to this prim type.
static bool
_ShouldApplyPassVisibility(const TfToken &primType)
{
    return _IsGeometryType(primType) || HdPrimTypeIsLight(primType) ||
        primType == HdPrimTypeTokens->lightFilter;
}

static bool
_IsVisible(const HdContainerDataSourceHandle& primSource)
{
    if (const auto visSchema = HdVisibilitySchema::GetFromParent(primSource)) {
        if (const HdBoolDataSourceHandle visDs = visSchema.GetVisibility()) {
            return visDs->GetTypedValue(0.0f);
        }
    }
    return true;
}

static bool
_IsVisibleToCamera(const HdContainerDataSourceHandle& primSource)
{
    // XXX Primvar queries like this might be a good candidate for
    // helper API in hdsi/utils.h.
    if (const HdPrimvarsSchema primvarsSchema =
        HdPrimvarsSchema::GetFromParent(primSource)) {
        if (HdPrimvarSchema primvarSchema =
            primvarsSchema.GetPrimvar(_tokens->riAttributesVisibilityCamera)) {
            if (const auto sampledDataSource =
                primvarSchema.GetPrimvarValue()) {
                const VtValue value = sampledDataSource->GetValue(0);
                if (!value.IsEmpty()) {
                    if (value.IsHolding<VtArray<bool>>()) {
                        return value.UncheckedGet<bool>();
                    }
                }
            }
        }
    }
    return true;
}

bool
HdPrman_RenderPassSceneIndex::_RenderPassState::DoesOverrideMatte(
    SdfPath const& primPath,
    HdSceneIndexPrim const& prim) const
{
    return matteEval
        && _IsGeometryType(prim.primType)
        && matteEval->Match(primPath);
}

bool
HdPrman_RenderPassSceneIndex::_RenderPassState::DoesOverrideVis(
    SdfPath const& primPath,
    HdSceneIndexPrim const& prim) const
{
    return renderVisEval
        && _ShouldApplyPassVisibility(prim.primType)
        && !renderVisEval->Match(primPath)
        && _IsVisible(prim.dataSource);
}

bool
HdPrman_RenderPassSceneIndex::_RenderPassState::DoesOverrideCameraVis(
    SdfPath const& primPath,
    HdSceneIndexPrim const& prim) const
{
    return cameraVisEval
        && _ShouldApplyPassVisibility(prim.primType)
        && !cameraVisEval->Match(primPath)
        && _IsVisibleToCamera(prim.dataSource);
}

bool
HdPrman_RenderPassSceneIndex::_RenderPassState::DoesPrune(
    SdfPath const& primPath) const
{
    return pruneEval && pruneEval->Match(primPath);
}

HdSceneIndexPrim 
HdPrman_RenderPassSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    // Pruning
    //
    // Note that we also apply pruning in GetChildPrimPaths(), but
    // this ensures that even if a downstream scene index asks
    // for a pruned path, it will remain pruned.
    if (_activeRenderPass.DoesPrune(primPath)) {
        return HdSceneIndexPrim();
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Temp storage for overriding primvars.
    TfSmallVector<TfToken, 2> primvarNames;
    TfSmallVector<HdDataSourceBaseHandle, 2> primvarVals;

    // Render Visibility -> HdVisibilitySchema
    //
    // Renderable prims that are visible in the upstream scene index,
    // but excluded from the pass renderVisibility collection, get their
    // visibility overriden to 0.
    //
    if (_activeRenderPass.DoesOverrideVis(primPath, prim)) {
        static const HdContainerDataSourceHandle invisDs =
            HdRetainedContainerDataSource::New(
                HdVisibilitySchema::GetSchemaToken(),
                HdVisibilitySchema::Builder()
                    .SetVisibility(
                        HdRetainedTypedSampledDataSource<bool>::New(0))
                    .Build());
        prim.dataSource =
            HdOverlayContainerDataSource::New(invisDs, prim.dataSource);
    }

    // Camera Visibility -> ri:visibility:camera
    //
    // Renderable prims that are camera-visible in the upstream scene index,
    // but excluded from the pass cameraVisibility collection, get their
    // riAttributesVisibilityCamera primvar overriden to 0.
    //
    if (_activeRenderPass.DoesOverrideCameraVis(primPath, prim)) {
        static const HdContainerDataSourceHandle cameraInvisDs =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<int>::New(0))
                .SetInterpolation(
                    HdPrimvarSchema::BuildInterpolationDataSource(
                        HdPrimvarSchemaTokens->constant))
                .Build();
        primvarNames.push_back(_tokens->riAttributesVisibilityCamera);
        primvarVals.push_back(cameraInvisDs);
    }

    // Matte -> ri:Matte
    //
    // If the matte pattern matches this prim, set ri:Matte=1.
    // Matte only applies to geometry types.
    // We do not bother to check if the upstream prim already
    // has matte set since that is essentially never the case.
    //
    if (_activeRenderPass.DoesOverrideMatte(primPath, prim)) {
        static const HdContainerDataSourceHandle matteDs =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<int>::New(1))
                .SetInterpolation(HdPrimvarSchema::
                    BuildInterpolationDataSource(
                        HdPrimvarSchemaTokens->constant))
                .Build();
        primvarNames.push_back(_tokens->riAttributesRiMatte);
        primvarVals.push_back(matteDs);
    }

    // Apply any accumulated primvar overrides.
    if (!primvarNames.empty()) {
        prim.dataSource =
            HdOverlayContainerDataSource::New(
                HdRetainedContainerDataSource::New(
                    HdPrimvarsSchema::GetSchemaToken(),
                    HdPrimvarsSchema::BuildRetained(
                        primvarNames.size(),
                        primvarNames.data(),
                        primvarVals.data())),
                prim.dataSource);
    }

    return prim;
}

SdfPathVector 
HdPrman_RenderPassSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    if (_activeRenderPass.pruneEval) {
        SdfPathVector childPathVec =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        HdsiUtilsRemovePrunedChildren(primPath, *_activeRenderPass.pruneEval,
                                      &childPathVec);
        return childPathVec;
    } else {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }
}

/*

General notes on change processing and invalidation:

- Rather than lazily evaluate the active render pass state,
  and be prepared to do so from multiple caller threads, we
  instead greedily set up the active render pass state.
  Though greedy, this is a small amount of computation,
  and only triggered on changes to two specific scene locations:
  the root scope where HdSceneGlobalsSchema lives, and the
  scope where the designated active render pass lives.

- The list of entries for prims added, dirtied, or removed
  must be filtered against the active render pass prune collection.

- The list of entries for prims added, dirtied, or removed
  can imply changes to which render pass is active, or to the
  contents of the active render pass.  In either case, if the
  effective render pass state changes, downstream observers
  must be notified about the effects.

*/

// Helper to scan an entry vector for an entry that
// could affect the active render pass.
template <typename ENTRIES>
inline static bool
_EntryCouldAffectPass(
    const ENTRIES &entries,
    SdfPath const& activeRenderPassPath)
{
    for (const auto& entry: entries) {
        // The prim at the root path contains the HdSceneGlobalsSchema.
        // The prim at the render pass path controls its behavior.
        if (entry.primPath.IsAbsoluteRootPath()
            || entry.primPath == activeRenderPassPath) {
            return true;
        }
    }
    return false;
}

// Helper to apply pruning to an entry list.
// Returns true if any pruning was applied, putting surviving entries
// into *postPruneEntries.
template <typename ENTRIES>
inline static bool
_PruneEntries(
    std::optional<HdCollectionExpressionEvaluator> &pruneEval,
    const ENTRIES &entries, ENTRIES *postPruneEntries)
{
    if (!pruneEval) {
        // No pruning active.
        return false;
    }
    // Pre-pass to see if any prune applies to the list.
    bool foundEntryToPrune = false;
    for (const auto& entry: entries) {
        if (pruneEval->Match(entry.primPath)) {
            foundEntryToPrune = true;
            break;
        }
    }
    if (!foundEntryToPrune) {
        // No entries to prune.
        return false;
    } else {
        // Prune matching entries.
        for (const auto& entry: entries) {
            if (!pruneEval->Match(entry.primPath)) {
                // Accumulate survivors.
                postPruneEntries->push_back(entry);
            }
        }
        return true;
    }
}

void
HdPrman_RenderPassSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{    
    HdSceneIndexObserver::AddedPrimEntries extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(
            &extraAddedEntries, &extraDirtyEntries, &extraRemovedEntries);
    }

    // Filter entries against any active render pass prune collection.
    if (!_PruneEntries(_activeRenderPass.pruneEval, entries,
                       &extraAddedEntries)) {
        _SendPrimsAdded(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
    _SendPrimsDirtied(extraDirtyEntries);
}

void 
HdPrman_RenderPassSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    HdSceneIndexObserver::AddedPrimEntries extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(
            &extraAddedEntries, &extraDirtyEntries, &extraRemovedEntries);
    }

    // Filter entries against any active render pass prune collection.
    if (!_PruneEntries(_activeRenderPass.pruneEval, entries,
                       &extraRemovedEntries)) {
        _SendPrimsRemoved(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
    _SendPrimsDirtied(extraDirtyEntries);
}

void
HdPrman_RenderPassSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    HdSceneIndexObserver::AddedPrimEntries extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(
            &extraAddedEntries, &extraDirtyEntries, &extraRemovedEntries);
    }

    // Filter entries against any active render pass prune collection.
    if (!_PruneEntries(_activeRenderPass.pruneEval, entries,
                       &extraDirtyEntries)) {
        _SendPrimsDirtied(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
    _SendPrimsDirtied(extraDirtyEntries);
}

HdPrman_RenderPassSceneIndex::~HdPrman_RenderPassSceneIndex() = default;

// Helper method to compile a collection evaluator.
static void
_CompileCollection(
    HdCollectionsSchema &collections,
    TfToken const& collectionName,
    HdSceneIndexBaseRefPtr const& sceneIndex,
    SdfPathExpression *expr,
    std::optional<HdCollectionExpressionEvaluator> *eval)
{
    if (HdCollectionSchema collection =
        collections.GetCollection(collectionName)) {
        if (HdPathExpressionDataSourceHandle pathExprDs =
            collection.GetMembershipExpression()) {
            *expr = pathExprDs->GetTypedValue(0.0);
            if (!expr->IsEmpty()) {
                *eval = HdCollectionExpressionEvaluator(sceneIndex, *expr);
            }
        }
    }
}

void
HdPrman_RenderPassSceneIndex::_UpdateActiveRenderPassState(
    HdSceneIndexObserver::AddedPrimEntries *addedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries,
    HdSceneIndexObserver::RemovedPrimEntries *removedEntries)
{
    TRACE_FUNCTION();

    // Swap out the prior pass state to compare against.
    _RenderPassState &state = _activeRenderPass;
    _RenderPassState priorState;
    std::swap(state, priorState);

    // Check upstream scene index for an active render pass.
    HdSceneIndexBaseRefPtr inputSceneIndex = _GetInputSceneIndex();
    HdSceneGlobalsSchema globals =
        HdSceneGlobalsSchema::GetFromSceneIndex(inputSceneIndex);
    if (HdPathDataSourceHandle pathDs = globals.GetActiveRenderPassPrim()) {
        state.renderPassPath = pathDs->GetTypedValue(0.0);
    }
    if (state.renderPassPath.IsEmpty() && priorState.renderPassPath.IsEmpty()) {
        // Avoid further work if no render pass was or is active.
        return;
    }
    if (!state.renderPassPath.IsEmpty()) {
        const HdSceneIndexPrim passPrim =
            inputSceneIndex->GetPrim(state.renderPassPath);
        if (HdCollectionsSchema collections =
            HdCollectionsSchema::GetFromParent(passPrim.dataSource)) {
            // Prepare evaluators for render pass collections.
            _CompileCollection(collections, _tokens->matte,
                               inputSceneIndex,
                               &state.matteExpr,
                               &state.matteEval);
            _CompileCollection(collections, _tokens->renderVisibility,
                               inputSceneIndex,
                               &state.renderVisExpr,
                               &state.renderVisEval);
            _CompileCollection(collections, _tokens->cameraVisibility,
                               inputSceneIndex,
                               &state.cameraVisExpr,
                               &state.cameraVisEval);
            _CompileCollection(collections, _tokens->prune,
                               inputSceneIndex,
                               &state.pruneExpr,
                               &state.pruneEval);
        }
    }

    // Short-circuit the analysis below based on which patterns changed.
    const bool visOrMatteExprDidChange =
        state.matteExpr != priorState.matteExpr ||
        state.renderVisExpr != priorState.renderVisExpr ||
        state.cameraVisExpr != priorState.cameraVisExpr;

    if (state.pruneExpr == priorState.pruneExpr && !visOrMatteExprDidChange) {
        // No patterns changed; nothing to invalidate.
        return;
    }

    // Generate change entries for affected prims.
    // Consider all upstream prims.
    //
    // TODO: HdCollectionExpressionEvaluator::PopulateAllMatches()
    // should be used here instead, since in the future it will handle
    // instance matches as well as parallel traversal.
    //
    for (const SdfPath &path: HdSceneIndexPrimView(_GetInputSceneIndex())) {
        if (priorState.DoesPrune(path)) {
            // The prim had been pruned.
            if (!state.DoesPrune(path)) {
                // The prim is no longer pruned, so add it back.
                HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
                addedEntries->push_back({path, prim.primType});
            } else {
                // The prim is still pruned, so nothing to do.
            }
        } else if (state.DoesPrune(path)) {
            // The prim is newly pruned, so remove it.
            removedEntries->push_back({path});
        } else if (visOrMatteExprDidChange) {
            // Determine which (if any) locators on the upstream prim
            // are dirtied by the change in render pass state.
            const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
            const bool visibilityDidChange =
                (priorState.DoesOverrideVis(path, prim)
                 != state.DoesOverrideVis(path, prim));
            const bool primvarsDidChange =
                (priorState.DoesOverrideCameraVis(path, prim)
                 != state.DoesOverrideCameraVis(path, prim)) ||
                (priorState.DoesOverrideMatte(path, prim)
                 != state.DoesOverrideMatte(path, prim));
            if (primvarsDidChange || visibilityDidChange) {
                HdDataSourceLocatorSet locators;
                if (primvarsDidChange) {
                    locators.insert(HdPrimvarsSchema::GetDefaultLocator());
                }
                if (visibilityDidChange) {
                    locators.insert(HdVisibilitySchema::GetDefaultLocator());
                }
               dirtyEntries->push_back({path, locators});
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif //PXR_VERSION >= 2405
