//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/renderPassVisibilityAndMatteSceneIndexPlugin.h"

#if PXR_VERSION >= 2408

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/collectionsSchema.h"
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

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderVisibility)
    (cameraVisibility)
    (matte)
    ((riAttributesRiMatte, "ri:attributes:Ri:Matte"))
    ((riAttributesVisibilityCamera, "ri:attributes:visibility:camera"))
    ((sceneIndexPluginName, "HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We need an "insertion point" that's *after* general material resolve.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;

    for (auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // No input args.
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

namespace {

bool
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
bool
_ShouldApplyPassVisibility(const TfToken &primType)
{
    return _IsGeometryType(primType) || HdPrimTypeIsLight(primType) ||
        primType == HdPrimTypeTokens->lightFilter;
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

bool
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

////////////////////////////////////////////
// Render Pass Visibility And Matte State //
////////////////////////////////////////////

struct _RenderPassVisibilityAndMatteState {
    SdfPath renderPassPath;

    // Retain the expressions so we can compare old vs. new state.
    SdfPathExpression matteExpr;
    SdfPathExpression renderVisExpr;
    SdfPathExpression cameraVisExpr;

    // Evalulators for each pattern expression.
    std::optional<HdCollectionExpressionEvaluator> matteEval;
    std::optional<HdCollectionExpressionEvaluator> renderVisEval;
    std::optional<HdCollectionExpressionEvaluator> cameraVisEval;

    bool DoesOverrideMatte(
        const SdfPath &primPath,
        HdSceneIndexPrim const& prim) const
    {
        return matteEval
            && _IsGeometryType(prim.primType)
            && matteEval->Match(primPath);
    }

    bool DoesOverrideVis(
        const SdfPath &primPath,
        HdSceneIndexPrim const& prim) const
    {
        return renderVisEval
            && _ShouldApplyPassVisibility(prim.primType)
            && !renderVisEval->Match(primPath)
            && _IsVisible(prim.dataSource);
    }

    bool DoesOverrideCameraVis(
        const SdfPath &primPath,
        HdSceneIndexPrim const& prim) const
    {
        return cameraVisEval
            && _ShouldApplyPassVisibility(prim.primType)
            && !cameraVisEval->Match(primPath)
            && _IsVisibleToCamera(prim.dataSource);
    }
};

//////////////////////////////////////////////////
// Render Pass Visibility And Matte Scene Index //
//////////////////////////////////////////////////

TF_DECLARE_WEAK_AND_REF_PTRS(_RenderPassVisibilityAndMatteSceneIndex);

class _RenderPassVisibilityAndMatteSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    static _RenderPassVisibilityAndMatteSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _RenderPassVisibilityAndMatteSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    _RenderPassVisibilityAndMatteSceneIndex(
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
    friend class _RenderPassVisibilityAndMatteDataSource;

    // Pull on the scene globals schema for the active render pass,
    // computing and caching its visibility state in _activeRenderPass.
    void _UpdateActiveRenderPassState(
        HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries);

    // Visibility and matte state for the active render pass.
    _RenderPassVisibilityAndMatteState _activeRenderPass;

    // Flag used to track the first time prims have been added.
    bool _hasPopulated = false;
};

//////////////////////////////////////////////////
// Render Pass Visibility And Matte Data Source //
//////////////////////////////////////////////////

class _RenderPassVisibilityAndMatteDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RenderPassVisibilityAndMatteDataSource);

    TfTokenVector GetNames() override {
        return _prim.dataSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    _RenderPassVisibilityAndMatteDataSource(
        _RenderPassVisibilityAndMatteSceneIndexConstPtr const& sceneIndex,
        SdfPath const& primPath,
        HdSceneIndexPrim const& prim)
     : _sceneIndex(sceneIndex)
     , _primPath(primPath)
     , _prim(prim)
    {
    }

    // This dataSource accesses scene state tracked by the scene index.
    const _RenderPassVisibilityAndMatteSceneIndexConstPtr _sceneIndex;
    const SdfPath _primPath;
    const HdSceneIndexPrim _prim;
};

HdDataSourceBaseHandle
_RenderPassVisibilityAndMatteDataSource::Get(const TfToken &name)
{
    if (!_sceneIndex || !_prim.dataSource) {
        return nullptr;
    }

    // State from the scene index.
    _RenderPassVisibilityAndMatteState const& renderPass =
        _sceneIndex->_activeRenderPass;

    // Primvars
    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        HdContainerDataSourceHandle primvarsDs =
            HdPrimvarsSchema::GetFromParent(_prim.dataSource).GetContainer();
        HdContainerDataSourceEditor primvarEditor(primvarsDs);

        // Camera Visibility -> ri:visibility:camera
        //
        // Renderable prims that are camera-visible in the upstream scene index,
        // but excluded from the pass cameraVisibility collection, get their
        // riAttributesVisibilityCamera primvar overriden to 0.
        //
        if (renderPass.DoesOverrideCameraVis(_primPath, _prim)) {
            static const HdContainerDataSourceHandle invisDs =
                HdPrimvarSchema::Builder()
                    .SetPrimvarValue(
                        HdRetainedTypedSampledDataSource<int>::New(0))
                    .SetInterpolation(
                        HdPrimvarSchema::BuildInterpolationDataSource(
                            HdPrimvarSchemaTokens->constant))
                    .Build();
            primvarEditor.Overlay(
                HdDataSourceLocator(_tokens->riAttributesVisibilityCamera),
                invisDs);
        }

        // Matte -> ri:Matte
        //
        // If the matte pattern matches this prim, set ri:Matte=1.
        // Matte only applies to geometry types.
        // We do not bother to check if the upstream prim already
        // has matte set since that is essentially never the case.
        //
        if (renderPass.DoesOverrideMatte(_primPath, _prim)) {
            static const HdContainerDataSourceHandle matteDs =
                HdPrimvarSchema::Builder()
                    .SetPrimvarValue(
                        HdRetainedTypedSampledDataSource<int>::New(1))
                    .SetInterpolation(HdPrimvarSchema::
                        BuildInterpolationDataSource(
                            HdPrimvarSchemaTokens->constant))
                    .Build();
            primvarEditor.Overlay(
                HdDataSourceLocator(_tokens->riAttributesRiMatte),
                matteDs);
        }

        return primvarEditor.Finish();
    }

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

    return _prim.dataSource->Get(name);
}

//////////////////////////////////////////////////////////
// Render Pass Visibility And Matte Scene Index (cont.) //
//////////////////////////////////////////////////////////

HdSceneIndexPrim 
_RenderPassVisibilityAndMatteSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.dataSource) {
        // Overrides happen in the prim-level data source.
        prim.dataSource = _RenderPassVisibilityAndMatteDataSource::New(
            TfCreateWeakPtr(this), primPath, prim);
    }

    return prim;
}

SdfPathVector 
_RenderPassVisibilityAndMatteSceneIndex::GetChildPrimPaths(
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
_RenderPassVisibilityAndMatteSceneIndex::_PrimsAdded(
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
_RenderPassVisibilityAndMatteSceneIndex::_PrimsRemoved(
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
_RenderPassVisibilityAndMatteSceneIndex::_PrimsDirtied(
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
_RenderPassVisibilityAndMatteSceneIndex::_UpdateActiveRenderPassState(
    HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries)
{
    TRACE_FUNCTION();

    // Swap out the prior pass state to compare against.
    _RenderPassVisibilityAndMatteState &state = _activeRenderPass;
    _RenderPassVisibilityAndMatteState priorState;
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
            HdsiUtilsCompileCollection(collections, _tokens->matte,
                                       inputSceneIndex,
                                       &state.matteExpr,
                                       &state.matteEval);
            HdsiUtilsCompileCollection(collections, _tokens->renderVisibility,
                                       inputSceneIndex,
                                       &state.renderVisExpr,
                                       &state.renderVisEval);
            HdsiUtilsCompileCollection(collections, _tokens->cameraVisibility,
                                       inputSceneIndex,
                                       &state.cameraVisExpr,
                                       &state.cameraVisEval);
        }
    }

    // Short-circuit the analysis below based on which patterns changed.
    const bool visOrMatteExprDidChange =
        state.matteExpr != priorState.matteExpr ||
        state.renderVisExpr != priorState.renderVisExpr ||
        state.cameraVisExpr != priorState.cameraVisExpr;

    if (!visOrMatteExprDidChange || !_hasPopulated) {
        // No patterns changed or no prims have been populated previously;
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

} // anon

/////////////////////////////////////////////////////////
// Render Pass Visibility And Matte Scene Index Plugin //
/////////////////////////////////////////////////////////

HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin::
HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_RenderPassVisibilityAndMatteSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _RenderPassVisibilityAndMatteSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif //PXR_VERSION >= 2408
