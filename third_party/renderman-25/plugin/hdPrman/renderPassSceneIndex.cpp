// Copyright 2024 Pixar
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

#include "hdPrman/renderPassSceneIndex.h"
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
#include "pxr/imaging/hd/schema.h" 
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hdsi/utils.h"

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

HdSceneIndexPrim 
HdPrman_RenderPassSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    // Apply active render pass state to upstream prim.
    _RenderPassState const& state = _PullActiveRenderPasssState();

    // Pruning
    //
    // Note that we also apply pruning in GetChildPrimPaths(), but
    // this ensures that even if a downstream scene index still assks
    // for a pruned path, it will remain pruned.
    if (state.pruneEval) {
        const bool pruned = state.pruneEval->Match(primPath);
        if (pruned) {
            return HdSceneIndexPrim();
        }
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
    if (state.renderVisEval &&
        _ShouldApplyPassVisibility(prim.primType) &&
        !state.renderVisEval->Match(primPath) &&
        _IsVisible(prim.dataSource))
    {
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
    if (state.cameraVisEval &&
        _ShouldApplyPassVisibility(prim.primType) &&
        !state.cameraVisEval->Match(primPath) &&
        _IsVisibleToCamera(prim.dataSource))
    {
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
    if (state.matteEval &&
        _IsGeometryType(prim.primType) &&
        state.matteEval->Match(primPath))
    {
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
    // Apply active render pass state to upstream prim.
    _RenderPassState const& state = _PullActiveRenderPasssState();

    if (state.pruneEval) {
        SdfPathVector childPathVec =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        HdsiUtilsRemovePrunedChildren(primPath, *state.pruneEval,
                                      &childPathVec);
        return childPathVec;
    } else {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }
}

void
HdPrman_RenderPassSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{    
    {
        // TODO: Invalidation
        std::unique_lock<std::mutex> lock(_activeRenderPassMutex);
        _activeRenderPass.reset();
    }
    _SendPrimsAdded(entries);
}

void 
HdPrman_RenderPassSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    {
        // TODO: Invalidation
        std::unique_lock<std::mutex> lock(_activeRenderPassMutex);
        _activeRenderPass.reset();
    }
    _SendPrimsRemoved(entries);
}

void
HdPrman_RenderPassSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    {
        // TODO: Invalidation
        std::unique_lock<std::mutex> lock(_activeRenderPassMutex);
        _activeRenderPass.reset();
    }
    _SendPrimsDirtied(entries);
}


HdPrman_RenderPassSceneIndex::~HdPrman_RenderPassSceneIndex() = default;

// Helper method to compile a collection evaluator.
static void
_CompileCollection(
    HdCollectionsSchema &collections,
    TfToken const& collectionName,
    HdSceneIndexBaseRefPtr const& sceneIndex,
    std::optional<HdCollectionExpressionEvaluator> *result)
{
    if (HdCollectionSchema collection =
        collections.GetCollection(collectionName)) {
        if (HdPathExpressionDataSourceHandle pathExprDs =
            collection.GetMembershipExpression()) {
            SdfPathExpression pathExpr = pathExprDs->GetTypedValue(0.0);
            if (!pathExpr.IsEmpty()) {
                *result = HdCollectionExpressionEvaluator(sceneIndex, pathExpr);
            }
        }
    }
}

HdPrman_RenderPassSceneIndex::_RenderPassState const&
HdPrman_RenderPassSceneIndex::_PullActiveRenderPasssState() const
{
    std::unique_lock<std::mutex> lock(_activeRenderPassMutex);
    if (_activeRenderPass) {
        // Note: we assume callers invoke this method in a context
        // that protects against invalidation of the pointer.
        // Ex: GetPrim(), but not _PrimsDirtied().
        return *_activeRenderPass;
    }
    HdSceneIndexBaseRefPtr inputSceneIndex = _GetInputSceneIndex();
    _activeRenderPass = std::make_unique<_RenderPassState>();
    _RenderPassState &state = *_activeRenderPass;
    HdSceneGlobalsSchema globals =
        HdSceneGlobalsSchema::GetFromSceneIndex(inputSceneIndex);
    if (HdPathDataSourceHandle pathDs = globals.GetActiveRenderPassPrim()) {
        state.renderPassPath = pathDs->GetTypedValue(0.0);
    }
    const HdSceneIndexPrim passPrim =
        inputSceneIndex->GetPrim(state.renderPassPath);
    if (HdCollectionsSchema collections =
        HdCollectionsSchema::GetFromParent(passPrim.dataSource)) {
        // Prepare evaluators for render pass collections.
        _CompileCollection(collections, _tokens->matte,
                           inputSceneIndex, &state.matteEval);
        _CompileCollection(collections, _tokens->renderVisibility,
                           inputSceneIndex, &state.renderVisEval);
        _CompileCollection(collections, _tokens->cameraVisibility,
                           inputSceneIndex, &state.cameraVisEval);
        _CompileCollection(collections, _tokens->prune,
                           inputSceneIndex, &state.pruneEval);
    }
    return state;
}

PXR_NAMESPACE_CLOSE_SCOPE
