//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdSt/renderPassVisibilitySceneIndexPlugin.h"

#if PXR_VERSION >= 2408

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/schema.h" 
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hdsi/utils.h"
#include "pxr/base/trace/trace.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderVisibility)
    ((sceneIndexPluginName, "HdSt_RenderPassVisibilitySceneIndexPlugin"))
);

static const char* const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdSt_RenderPassVisibilitySceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We want this plugin...
    // - Downstream of procedural expansion
    //   (HdGpSceneIndexPlugin::GetInsertionPhase(), currently 2) so that any
    //   render pass visibility rules apply to generated prims.
    // - Upstream of scene indices that may override visibility, but we don't
    //   have a good way to query/know this.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 4;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr, // No input args.
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

namespace {

bool
_IsGeometryType(const TfToken &primType)
{
    static const TfTokenVector extraGeomTypes = {
        HdPrimTypeTokens->cone,
        HdPrimTypeTokens->cylinder,
        HdPrimTypeTokens->sphere
    };
    return HdPrimTypeIsGprim(primType) ||
        std::find(extraGeomTypes.begin(), extraGeomTypes.end(), primType)
            != extraGeomTypes.end();
}

// Returns true if the renderVisibility rules apply to this prim type.
bool
_ShouldApplyPassVisibility(const TfToken &primType)
{
    return _IsGeometryType(primType) || HdPrimTypeIsLight(primType);
}

bool
_IsVisible(const HdContainerDataSourceHandle& primSource)
{
    if (const auto visSchema = HdVisibilitySchema::GetFromParent(primSource)) {
        if (const HdBoolDataSourceHandle visDs = visSchema.GetVisibility()) {
            return visDs->GetTypedValue(0.0f);
        }
    }
    return true;
}

//////////////////////////////////
// Render Pass Visibility State //
//////////////////////////////////
VtArray<bool>
_GetInstancerMask(const HdContainerDataSourceHandle& primSource)
{
    const HdInstancerTopologySchema topoSchema =
        HdInstancerTopologySchema::GetFromParent(primSource);
    if (!topoSchema) {
        return {};
    }
    if (const HdBoolArrayDataSourceHandle maskDs = topoSchema.GetMask()) {
        return maskDs->GetTypedValue(0.0f);
    }
    return {};
}


struct _RenderPassVisibilityState {
    SdfPath renderPassPath;

    // Retain the expression so we can compare old vs. new state.
    SdfPathExpression renderVisExpr;

    // Evalulator for the pattern expression.
    std::optional<HdCollectionExpressionEvaluator> renderVisEval;

    bool DoesOverrideVis(
        const SdfPath &primPath,
        HdSceneIndexPrim const& prim) const
    {
        return renderVisEval
            && _ShouldApplyPassVisibility(prim.primType)
            && !renderVisEval->Match(primPath)
            && _IsVisible(prim.dataSource);
    }

    // Returns per-instance render visibility mask values for instancer prims
    // that use implicit instancing (i.e., have instanceLocations set). Entry i
    // is true if instanceLocations[i] matches the renderVisibility collection,
    // false otherwise (instance is masked out). Returns an empty array if not
    // applicable or if all instances match (no override needed).
    VtArray<bool> GetRenderVisPerInstance(
        HdSceneIndexPrim const& prim) const
    {
        if (!renderVisEval || prim.primType != HdPrimTypeTokens->instancer) {
            return {};
        }
        const HdInstancerTopologySchema topoSchema =
            HdInstancerTopologySchema::GetFromParent(prim.dataSource);
        if (!topoSchema) {
            return {};
        }
        const HdPathArrayDataSourceHandle instanceLocationsDs =
            topoSchema.GetInstanceLocations();
        if (!instanceLocationsDs) {
            return {};
        }
        const VtArray<SdfPath> instanceLocations =
            instanceLocationsDs->GetTypedValue(0.0f);
        if (instanceLocations.empty()) {
            return {};
        }
        TRACE_FUNCTION();
        const VtArray<bool> existingMask =
            _GetInstancerMask(prim.dataSource);
        VtArray<bool> mask =
            (!existingMask.empty() &&
             existingMask.size() == instanceLocations.size())
            ? existingMask
            : VtArray<bool>(instanceLocations.size(), true);
        bool modified = false;
        for (size_t i = 0; i < instanceLocations.size(); ++i) {
            if (!renderVisEval->Match(instanceLocations[i]) && mask[i]) {
                mask[i] = false;
                modified = true;
            }
        }
        return modified ? mask : existingMask;
    }
};

////////////////////////////////////////
// Render Pass Visibility Scene Index //
////////////////////////////////////////

TF_DECLARE_WEAK_AND_REF_PTRS(_RenderPassVisibilitySceneIndex);

class _RenderPassVisibilitySceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    static _RenderPassVisibilitySceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _RenderPassVisibilitySceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    _RenderPassVisibilitySceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
     : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    friend class _RenderPassVisibilityDataSource;

    // Pull on the scene globals schema for the active render pass,
    // computing and caching its visibility state in _activeRenderPass.
    void _UpdateActiveRenderPassState(
        HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries);

    // Visibility state for the active render pass.
    _RenderPassVisibilityState _activeRenderPass;

    // Flag used to track the first time prims have been added.
    bool _hasPopulated = false;
};

////////////////////////////////////////
// Render Pass Visibility Data Source //
////////////////////////////////////////

class _RenderPassVisibilityDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderPassVisibilityDataSource);

    TfTokenVector GetNames() override {
        return _prim.dataSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    _RenderPassVisibilityDataSource(
        _RenderPassVisibilitySceneIndexConstPtr const& sceneIndex,
        SdfPath const& primPath,
        HdSceneIndexPrim const& prim)
     : _sceneIndex(sceneIndex)
     , _primPath(primPath)
     , _prim(prim)
    {
    }

    // This dataSource accesses scene state tracked by the scene index.
    const _RenderPassVisibilitySceneIndexConstPtr _sceneIndex;
    const SdfPath _primPath;
    const HdSceneIndexPrim _prim;
};

HdDataSourceBaseHandle
_RenderPassVisibilityDataSource::Get(const TfToken &name)
{
    if (!_sceneIndex || !_prim.dataSource) {
        return nullptr;
    }

    // State from the scene index.
    _RenderPassVisibilityState const& renderPass =
        _sceneIndex->_activeRenderPass;

    // Render Visibility -> HdVisibilitySchema
    //
    // Renderable prims that are visible in the upstream scene index,
    // but excluded from the pass renderVisibility collection, get their
    // visibility overriden to 0.
    //
    if (name == HdVisibilitySchema::GetSchemaToken()) {
        if (renderPass.DoesOverrideVis(_primPath, _prim)) {
            return HdVisibilitySchema::Builder()
                .SetVisibility(
                    HdRetainedTypedSampledDataSource<bool>::New(false))
                .Build();
        }
    }

    // Instancer Render Visibility -> instancerTopology.mask
    //
    // For instancer prims using implicit instancing (instanceLocations
    // present), evaluate the renderVisibility pattern against each instance
    // location and set the instancer mask to false for non-matching instances.
    //
    if (name == HdInstancerTopologySchema::GetSchemaToken()) {
        const VtArray<bool> renderVisPerInstance =
            renderPass.GetRenderVisPerInstance(_prim);
        if (!renderVisPerInstance.empty()) {
            HdContainerDataSourceHandle existingTopoDs =
                HdContainerDataSource::Cast(_prim.dataSource->Get(name));
            return HdOverlayContainerDataSource::New(
                HdInstancerTopologySchema::Builder()
                    .SetMask(
                        HdRetainedTypedSampledDataSource<VtArray<bool>>::New(
                            renderVisPerInstance))
                    .Build(),
                existingTopoDs);
        }
    }

    return _prim.dataSource->Get(name);
}

////////////////////////////////////////////////
// Render Pass Visibility Scene Index (cont.) //
////////////////////////////////////////////////

static bool
_IsPrototype(HdSceneIndexPrim const& prim)
{
    if (HdInstancedBySchema instancedBySchema =
        HdInstancedBySchema::GetFromParent(prim.dataSource)) {
        // XXX The docs for the InstancedBy schema say it is
        // "A schema marking a prim as instanced by another prim"
        //
        // However, experimentally we observe prims from Presto Mf
        // have this schema regardless of being instanced.  So we
        // must pull apart the schema to check for instance paths.
        //
        // It seems like it would be better to only emit the
        // instancedBy schema for prims that are instanced.
        if (HdPathArrayDataSourceHandle pathsDs =
            instancedBySchema.GetPaths()) {
            VtArray<SdfPath> paths = pathsDs->GetTypedValue(0.0);
            return !paths.empty();
        }
    }
    return false;
}

HdSceneIndexPrim 
_RenderPassVisibilitySceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Do not modify prototype prims (those with instancedBy schema).
    // Prototypes are an internal detail of the system, not something
    // meant to be user-facing, so there must be no way to point
    // user-defined operations (such as render pass collections)
    // at prototypes.
    const HdInstancedBySchema instancedBy =
        HdInstancedBySchema::GetFromParent(prim.dataSource);

    if (prim.dataSource && !_IsPrototype(prim)) {
        // Overrides happen in the prim-level data source.
        prim.dataSource = _RenderPassVisibilityDataSource::New(
            TfCreateWeakPtr(this), primPath, prim);
    }

    return prim;
}

SdfPathVector 
_RenderPassVisibilitySceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
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
  can imply changes to which render pass is active, or to the
  contents of the active render pass.  In either case, if the
  effective render pass state changes, downstream observers
  must be notified about the effects.

*/

// Helper to scan an entry vector for an entry that
// could affect the active render pass.
template <typename ENTRIES>
inline bool
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

void
_RenderPassVisibilitySceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraDirtyEntries);
    }

    // Fast path: If this is the first time we are adding prims,
    // we do not need to check for invalidation of existing prims
    // inside _UpdateActiveRenderPassState().  From now on, we will.
    if (!_hasPopulated) {
        _hasPopulated = true;
    }

    _SendPrimsAdded(entries);
    _SendPrimsDirtied(extraDirtyEntries);
}

void 
_RenderPassVisibilitySceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraDirtyEntries);
    }

    _SendPrimsRemoved(entries);
    _SendPrimsDirtied(extraDirtyEntries);
}

void
_RenderPassVisibilitySceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraDirtyEntries);
    }

    _SendPrimsDirtied(entries);
    _SendPrimsDirtied(extraDirtyEntries);
}

void
_RenderPassVisibilitySceneIndex::_UpdateActiveRenderPassState(
    HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries)
{
    TRACE_FUNCTION();

    // Swap out the prior pass state to compare against.
    _RenderPassVisibilityState &state = _activeRenderPass;
    _RenderPassVisibilityState priorState;
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
            HdsiUtilsCompileCollection(collections, _tokens->renderVisibility,
                                       inputSceneIndex,
                                       &state.renderVisExpr,
                                       &state.renderVisEval);
        }
    }

    if (state.renderVisExpr == priorState.renderVisExpr || !_hasPopulated) {
        // Pattern unchanged or no prims have been populated previously;
        // nothing to invalidate.
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
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
        HdDataSourceLocatorSet locators;
        const bool visibilityDidChange =
            (priorState.DoesOverrideVis(path, prim)
             != state.DoesOverrideVis(path, prim));
        const bool instancerTopologyDidChange =
            (priorState.GetRenderVisPerInstance(prim)
             != state.GetRenderVisPerInstance(prim));
        if (visibilityDidChange) {
            locators.insert(HdVisibilitySchema::GetDefaultLocator());
        }
        if (instancerTopologyDidChange) {
            locators.insert(HdInstancerTopologySchema::GetDefaultLocator());
        }
        if (!locators.IsEmpty()) {
            dirtyEntries->push_back({path, locators});
        }
    }
}

} // anon

///////////////////////////////////////////////
// Render Pass Visibility Scene Index Plugin //
///////////////////////////////////////////////

HdSt_RenderPassVisibilitySceneIndexPlugin::
HdSt_RenderPassVisibilitySceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_RenderPassVisibilitySceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _RenderPassVisibilitySceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif //PXR_VERSION >= 2408
