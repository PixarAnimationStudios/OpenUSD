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
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/primUtils.h"

#include "pxr/imaging/hdSt/computation.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/subdivision.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/diagnostic.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/arch/hash.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDST_ENABLE_SHARED_VERTEX_PRIMVAR, 1,
                      "Enable sharing of vertex primvar");

TF_MAKE_STATIC_DATA(
    HdSt_MaterialNetworkShaderSharedPtr,
    _fallbackWidgetShader)
{
    *_fallbackWidgetShader = std::make_shared<HdStGLSLFXShader>(
        std::make_shared<HioGlslfx>(HdStPackageWidgetShader()));
}

// -----------------------------------------------------------------------------
// Draw invalidation utilities
// -----------------------------------------------------------------------------
void
HdStMarkDrawBatchesDirty(HdRenderParam *renderParam)
{
    if (TF_VERIFY(renderParam)) {
        HdStRenderParam *stRenderParam =
            static_cast<HdStRenderParam*>(renderParam);
        stRenderParam->MarkDrawBatchesDirty();
    }
}

void
HdStMarkMaterialTagsDirty(HdRenderParam *renderParam)
{
    if (TF_VERIFY(renderParam)) {
        HdStRenderParam *stRenderParam =
            static_cast<HdStRenderParam*>(renderParam);
        stRenderParam->MarkMaterialTagsDirty();
    }
}

void
HdStMarkGeomSubsetDrawItemsDirty(HdRenderParam *renderParam)
{
    if (TF_VERIFY(renderParam)) {
        HdStRenderParam *stRenderParam =
            static_cast<HdStRenderParam*>(renderParam);
        stRenderParam->MarkGeomSubsetDrawItemsDirty();
    }
}

void
HdStMarkGarbageCollectionNeeded(HdRenderParam *renderParam)
{
    if (TF_VERIFY(renderParam)) {
        HdStRenderParam *stRenderParam =
            static_cast<HdStRenderParam*>(renderParam);
        stRenderParam->SetGarbageCollectionNeeded();
    }
}

// -----------------------------------------------------------------------------
// Primvar descriptor filtering utilities
// -----------------------------------------------------------------------------
static bool
_IsEnabledPrimvarFiltering(HdStDrawItem const * drawItem)
{
    HdSt_MaterialNetworkShaderSharedPtr materialNetworkShader =
        drawItem->GetMaterialNetworkShader();
    return materialNetworkShader &&
           materialNetworkShader->IsEnabledPrimvarFiltering();
}

static TfTokenVector const &
_GetFilterNamesForMaterial(HdStDrawItem const * drawItem)
{
    HdSt_MaterialNetworkShaderSharedPtr materialNetworkShader =
        drawItem->GetMaterialNetworkShader();
    if (materialNetworkShader) {
        return materialNetworkShader->GetPrimvarNames();
    }

    static const TfTokenVector fallback = TfTokenVector();
    return fallback;
}

static TfTokenVector
_GetFilterNames(HdRprim const * prim,
             HdStDrawItem const * drawItem,
             HdStInstancer const * instancer = nullptr)
{
    TfTokenVector filterNames = prim->GetBuiltinPrimvarNames();

    const TfTokenVector &matPvNames = _GetFilterNamesForMaterial(drawItem);
    filterNames.insert(filterNames.end(), matPvNames.begin(), 
        matPvNames.end());

    if (instancer) {
        TfTokenVector const & names = instancer->GetBuiltinPrimvarNames();
        filterNames.insert(filterNames.end(), names.begin(), names.end());
    }
    return filterNames;
}

static HdPrimvarDescriptorVector
_FilterPrimvarDescriptors(HdPrimvarDescriptorVector primvars,
                          TfTokenVector const & filterNames)
{
    primvars.erase(
        std::remove_if(primvars.begin(), primvars.end(),
            [&filterNames](HdPrimvarDescriptor const &desc) {
                return std::find(filterNames.begin(), filterNames.end(),
                                 desc.name) == filterNames.end();
            }),
        primvars.end());

    return primvars;
}

HdPrimvarDescriptorVector
HdStGetPrimvarDescriptors(
    HdRprim const * prim,
    HdStDrawItem const * drawItem,
    HdSceneDelegate * delegate,
    HdInterpolation interpolation,
    HdReprSharedPtr const &repr,
    HdMeshGeomStyle descGeomStyle,
    int geomSubsetDescIndex,
    size_t numGeomSubsets)
{
    HD_TRACE_FUNCTION();

    HdPrimvarDescriptorVector primvars =
        prim->GetPrimvarDescriptors(delegate, interpolation);

    TfTokenVector filterNames;
    if (_IsEnabledPrimvarFiltering(drawItem)) {
        filterNames = _GetFilterNames(prim, drawItem);
    }

    if (numGeomSubsets != 0 && 
        repr && 
        descGeomStyle != HdMeshGeomStyleInvalid && 
        descGeomStyle != HdMeshGeomStylePoints) {
        for (size_t i = 0; i < numGeomSubsets; ++i) {
            HdStDrawItem const * subsetDrawItem =
                static_cast<HdStDrawItem*>(repr->GetDrawItemForGeomSubset(
                    geomSubsetDescIndex, numGeomSubsets, i));
            if (!TF_VERIFY(subsetDrawItem)) {
                continue;
            }
            if (_IsEnabledPrimvarFiltering(subsetDrawItem)) {
                const TfTokenVector matPvNames = _GetFilterNamesForMaterial(
                    subsetDrawItem);
                filterNames.insert(filterNames.end(), matPvNames.begin(), 
                    matPvNames.end());
            }
        }
        std::sort(filterNames.begin(), filterNames.end());
        filterNames.erase(std::unique(filterNames.begin(), filterNames.end()),
            filterNames.end());
    }

    if (filterNames.empty()) {
        return primvars;
    }
    return _FilterPrimvarDescriptors(primvars, filterNames);
}

HdPrimvarDescriptorVector
HdStGetInstancerPrimvarDescriptors(
    HdStInstancer const * instancer,
    HdSceneDelegate * delegate)
{
    HdPrimvarDescriptorVector primvars =
        delegate->GetPrimvarDescriptors(instancer->GetId(),
                                        HdInterpolationInstance);

    // XXX: Can we do filtering?
    return primvars;
}

// -----------------------------------------------------------------------------
// Tracking render tag changes
// -----------------------------------------------------------------------------

void
HdStUpdateRenderTag(HdSceneDelegate *delegate,
                    HdRenderParam *renderParam,
                    HdRprim *rprim)
{
    HdStRenderParam * const stRenderParam =
        static_cast<HdStRenderParam*>(renderParam);

    const TfToken prevRenderTag = rprim->GetRenderTag();
    rprim->HdRprim::UpdateRenderTag(delegate, renderParam);
    const TfToken &renderTag = rprim->GetRenderTag();
    if (renderTag == prevRenderTag) {
        return;
    }
    stRenderParam->DecreaseRenderTagCount(prevRenderTag);
    stRenderParam->IncreaseRenderTagCount(renderTag);
}

// -----------------------------------------------------------------------------
// Material processing utilities
// -----------------------------------------------------------------------------

void
HdStSetMaterialId(HdSceneDelegate *delegate,
                  HdRenderParam *renderParam,
                  HdRprim *rprim)
{
    SdfPath const& newMaterialId = delegate->GetMaterialId(rprim->GetId());
    if (rprim->GetMaterialId() != newMaterialId) {
        rprim->SetMaterialId(newMaterialId);

        // The batches need to be validated and rebuilt since a changed shader
        // may change aggregation.
        HdStMarkDrawBatchesDirty(renderParam);
    }
}

void
HdStSetMaterialTag(HdRenderParam * const renderParam,
                   HdDrawItem *drawItem,
                   const TfToken &materialTag)
{
    HdStRenderParam * const stRenderParam =
        static_cast<HdStRenderParam*>(renderParam);

    {
        // prevMaterialTag scoped to express that it is a reference
        // to a field modified by SetMaterialTag later.
        const TfToken &prevMaterialTag = drawItem->GetMaterialTag();

        if (materialTag == prevMaterialTag) {
            return;
        }
        
        stRenderParam->DecreaseMaterialTagCount(prevMaterialTag);
    }
    {
        stRenderParam->IncreaseMaterialTagCount(materialTag);
        drawItem->SetMaterialTag(materialTag);
    }

    // Trigger invalidation of the draw items cache of the render pass(es).
    HdStMarkMaterialTagsDirty(renderParam);
}

// Opinion precedence:
// Show occluded selection > Material opinion > displayOpacity primvar
//
static
TfToken
_ComputeMaterialTag(HdSceneDelegate * const delegate,
                    SdfPath const & materialId,
                    const bool hasDisplayOpacityPrimvar,
                    const bool occludedSelectionShowsThrough)
{
    if (occludedSelectionShowsThrough) {
        return HdStMaterialTagTokens->translucentToSelection;
    }

    const HdStMaterial *material =
        static_cast<const HdStMaterial *>(
            delegate->GetRenderIndex().GetSprim(
                HdPrimTypeTokens->material, materialId));
    if (material) {
        return material->GetMaterialTag();
    }

    if (hasDisplayOpacityPrimvar) {
        return HdStMaterialTagTokens->masked;
    }

    return HdMaterialTagTokens->defaultMaterialTag;
}

void
HdStSetMaterialTag(HdSceneDelegate * const delegate,
                   HdRenderParam * const renderParam,
                   HdDrawItem *drawItem,
                   SdfPath const & materialId,
                   const bool hasDisplayOpacityPrimvar,
                   const bool occludedSelectionShowsThrough)
{
    HdStSetMaterialTag(
        renderParam, drawItem,
        _ComputeMaterialTag(
            delegate, materialId, hasDisplayOpacityPrimvar, 
            occludedSelectionShowsThrough));
}

HdSt_MaterialNetworkShaderSharedPtr
HdStGetMaterialNetworkShader(
    HdRprim const * prim,
    HdSceneDelegate * delegate)
{
    return HdStGetMaterialNetworkShader(prim, delegate, prim->GetMaterialId());
}

HdSt_MaterialNetworkShaderSharedPtr
HdStGetMaterialNetworkShader(
    HdRprim const * prim,
    HdSceneDelegate * delegate,
    SdfPath const & materialId)
{
    // Resolve the prim's material or use the fallback material.
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdStMaterial const * material = static_cast<HdStMaterial const *>(
            renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
    if (material == nullptr) {
        if (prim->GetRenderTag(delegate) == HdRenderTagTokens->widget) {
            TF_DEBUG(HD_RPRIM_UPDATED).Msg("Using built-in widget material for "
                "%s\n", prim->GetId().GetText());
               
            return *_fallbackWidgetShader;
        } else {
            TF_DEBUG(HD_RPRIM_UPDATED).Msg("Using fallback material for %s\n",
                prim->GetId().GetText());

            material = static_cast<HdStMaterial const *>(
                renderIndex.GetFallbackSprim(HdPrimTypeTokens->material));
        }
    }

    return material->GetMaterialNetworkShader();
}

// -----------------------------------------------------------------------------
// Primvar processing and BAR allocation utilities
// -----------------------------------------------------------------------------
bool
HdStIsValidBAR(HdBufferArrayRangeSharedPtr const& range)
{
    return (range && range->IsValid());
}

bool
HdStCanSkipBARAllocationOrUpdate(
    HdBufferSourceSharedPtrVector const& sources,
    HdStComputationComputeQueuePairVector const& computations,
    HdBufferArrayRangeSharedPtr const& curRange,
    HdDirtyBits dirtyBits)
{
    TF_UNUSED(dirtyBits);
    // XXX: DirtyPrimvar is serving a double role of indicating primvar value
    // dirtyness as well as descriptor dirtyness.
    // We should move to a separate dirty bit for the latter.
    bool mayHaveDirtyPrimvarDesc = (dirtyBits & HdChangeTracker::DirtyPrimvar);

    // If we have no buffer/computation sources, we can skip processing in the
    // following cases:
    // - we haven't allocated a BAR previously
    // - we have an existing BAR and its primvar descriptors haven't changed
    bool noDataSourcesToUpdate = sources.empty() && computations.empty();
    return noDataSourcesToUpdate && 
           (!HdStIsValidBAR(curRange) || !mayHaveDirtyPrimvarDesc);
}

bool
HdStCanSkipBARAllocationOrUpdate(
    HdBufferSourceSharedPtrVector const& sources,
    HdBufferArrayRangeSharedPtr const& curRange,
    HdDirtyBits dirtyBits)
{
    return HdStCanSkipBARAllocationOrUpdate(
        sources, HdStComputationComputeQueuePairVector(), curRange, dirtyBits);
}

HdBufferSpecVector
_GetRemovedPrimvarBufferSpecs(
    HdBufferSpecVector const& curBarSpecs,
    HdPrimvarDescriptorVector const& newPrimvarDescs,
    HdExtComputationPrimvarDescriptorVector const& newCompPrimvarDescs,
    TfTokenVector const& internallyGeneratedPrimvarNames,
    SdfPath const& rprimId)
{
    HdBufferSpecVector removedPrimvarSpecs;
    // Get the new list of primvar sources for the BAR. We need to use both
    // the primvar descriptor list (that we get via the scene delegate), as
    // well as any internally generated primvars that are always added (such as
    // primId). This may contain primvars that fail validation, but we're only
    // interested in finding out existing primvars that aren't in the list.
    TfTokenVector newPrimvarNames;
    newPrimvarNames.reserve(newPrimvarDescs.size());
    for (auto const& desc : newPrimvarDescs) {
        newPrimvarNames.emplace_back(desc.name);
    }
    for (auto const& desc : newCompPrimvarDescs) {
        newPrimvarNames.emplace_back(desc.name);
    }

    // Check if the existing BAR has buffers that are neither in the new source
    // list nor are internally generated.
    for (auto const& spec : curBarSpecs) {

        bool isInNewList =
            std::find(newPrimvarNames.begin(), newPrimvarNames.end(), spec.name)
            != newPrimvarNames.end();
        
        if (isInNewList) {
            continue; // avoid the search below
        }

        bool isInGeneratedList =
            std::find(internallyGeneratedPrimvarNames.begin(),
                      internallyGeneratedPrimvarNames.end(), spec.name)
            != internallyGeneratedPrimvarNames.end();
        
        if (!isInGeneratedList) {
             TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                "%s: Found primvar %s that has been removed\n",
                rprimId.GetText(), spec.name.GetText());
            removedPrimvarSpecs.emplace_back(spec);
        }
    }

    return removedPrimvarSpecs;  
}

HdBufferSpecVector
HdStGetRemovedPrimvarBufferSpecs(
    HdBufferArrayRangeSharedPtr const& curRange,
    HdPrimvarDescriptorVector const& newPrimvarDescs,
    HdExtComputationPrimvarDescriptorVector const& newCompPrimvarDescs,
    TfTokenVector const& internallyGeneratedPrimvarNames,
    SdfPath const& rprimId)
{
    if (!HdStIsValidBAR(curRange)) {
        return HdBufferSpecVector();
    }

    HdBufferSpecVector curBarSpecs;
    curRange->GetBufferSpecs(&curBarSpecs);

    return _GetRemovedPrimvarBufferSpecs(curBarSpecs, newPrimvarDescs,
        newCompPrimvarDescs, internallyGeneratedPrimvarNames, rprimId);
}

HdBufferSpecVector
HdStGetRemovedPrimvarBufferSpecs(
    HdBufferArrayRangeSharedPtr const& curRange,
    HdPrimvarDescriptorVector const& newPrimvarDescs,
    TfTokenVector const& internallyGeneratedPrimvarNames,
    SdfPath const& rprimId)
{
    return HdStGetRemovedPrimvarBufferSpecs(curRange, newPrimvarDescs,
        HdExtComputationPrimvarDescriptorVector(),
        internallyGeneratedPrimvarNames, rprimId);
}

// XXX: Not currently exported; does anyone else need it?
HdBufferSpecVector
HdStGetRemovedOrReplacedPrimvarBufferSpecs(
    HdBufferArrayRangeSharedPtr const& curRange,
    HdPrimvarDescriptorVector const& newPrimvarDescs,
    HdExtComputationPrimvarDescriptorVector const& newCompPrimvarDescs,
    TfTokenVector const& internallyGeneratedPrimvarNames,
    HdBufferSpecVector const& updatedSpecs,
    SdfPath const& rprimId)
{
    if (!HdStIsValidBAR(curRange)) {
        return HdBufferSpecVector();
    }

    HdBufferSpecVector curBarSpecs;
    curRange->GetBufferSpecs(&curBarSpecs);

    HdBufferSpecVector removedOrReplacedSpecs = _GetRemovedPrimvarBufferSpecs(
        curBarSpecs, newPrimvarDescs, newCompPrimvarDescs,
        internallyGeneratedPrimvarNames, rprimId);

    // Sometimes the buffer spec for a given named primvar has changed, e.g.,
    // when an array-valued primvar has changed size. Such specs are not
    // in removedSpecs at this point, so we need to add them to ensure that
    // the old spec gets removed. Otherwise we will get shader compilation
    // errors after the new spec has been added because the primvar variable
    // will be defined twice.

    for (const auto& curSpec : curBarSpecs) {
        const auto newSpec = std::find_if(updatedSpecs.begin(),
            updatedSpecs.end(), 
            [&](const auto& spec) { return spec.name == curSpec.name; });
        // If we find a new spec that matches by name, we check if it is
        // different from the old spec. If it is, it needs to be removed.
        // The call to UpdateShaderStorageBufferArrayRange below will add
        // the new spec regardless, but will only remove the old one if it
        // is in removedSpecs. This fixes the case where resized array-valued
        // constant primvars were being declared multiple times causing
        // shader compilation failures.
        if (newSpec != updatedSpecs.end() && 
            curSpec != *newSpec) {
                TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                    "%s: Found primvar %s that has been replaced\n",
                    rprimId.GetText(), curSpec.name.GetText());
                removedOrReplacedSpecs.push_back(curSpec);
        }
    }
    return removedOrReplacedSpecs;
}

// XXX: Not currently exported; does anyone else need it?
HdBufferSpecVector
HdStGetRemovedOrReplacedPrimvarBufferSpecs(
    HdBufferArrayRangeSharedPtr const& curRange,
    HdPrimvarDescriptorVector const& newPrimvarDescs,
    TfTokenVector const& internallyGeneratedPrimvarNames,
    HdBufferSpecVector const& updatedSpecs,
    SdfPath const& rprimId)
{
    return HdStGetRemovedOrReplacedPrimvarBufferSpecs(
        curRange, newPrimvarDescs,
        HdExtComputationPrimvarDescriptorVector(),
        internallyGeneratedPrimvarNames, updatedSpecs, rprimId);
}

void
HdStUpdateDrawItemBAR(
    HdBufferArrayRangeSharedPtr const& newRange,
    int drawCoordIndex,
    HdRprimSharedData *sharedData,
    HdRenderParam *renderParam,
    HdChangeTracker *changeTracker)
{
    if (!sharedData) {
        TF_CODING_ERROR("Null shared data ptr received\n");
        return;
    }

    HdBufferArrayRangeSharedPtr const& curRange =
        sharedData->barContainer.Get(drawCoordIndex);
    SdfPath const& id = sharedData->rprimID;

    if (curRange == newRange) {
        // Nothing to do. The draw item's BAR hasn't been changed.
        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: BAR at draw coord %d is still (%p)\n",
            id.GetText(), drawCoordIndex, curRange.get());

        return;
    }

    bool const curRangeValid = HdStIsValidBAR(curRange);
    bool const newRangeValid = HdStIsValidBAR(newRange);

    if (curRangeValid) {
        HdStMarkGarbageCollectionNeeded(renderParam);

        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking garbage collection needed to possibly reclaim BAR %p"
            " at draw coord index %d\n",
            id.GetText(), (void*)curRange.get(), drawCoordIndex);

    }

    // Flag deep batch invalidation for the following scenarios:
    // 1. Invalid <-> Valid transitions.
    // 2. When the new range is associated with a buffer array that
    // fails the aggregation test (used during batching).
    // 3. When the dispatch buffer needs to be updated for MDI batches.
    //    Note: This is needed only for indirect draw batches to update the
    //    dispatch buffer, but we prefer to not hardcode a check for
    //    the same.
    bool const rebuildDispatchBuffer = curRangeValid && newRangeValid &&
               curRange->GetElementOffset() != newRange->GetElementOffset();

    if (curRangeValid != newRangeValid ||
        !newRange->IsAggregatedWith(curRange) ||
        rebuildDispatchBuffer) {

        HdStMarkDrawBatchesDirty(renderParam);

        if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
            if (curRangeValid != newRangeValid) {
                TfDebug::Helper().Msg(
                    "%s: Marking all batches dirty due to an invalid <-> valid"
                    " transition (new BAR %p, existing BAR %p)\n",
                    id.GetText(), newRange.get(), curRange.get());

            } else if (!newRange->IsAggregatedWith(curRange)) {
                TfDebug::Helper().Msg(
                    "%s: Marking all batches dirty since the new BAR (%p) "
                    "doesn't aggregate with the existing BAR (%p)\n",
                    id.GetText(), newRange.get(), curRange.get());
            
            } else {
                TfDebug::Helper().Msg(
                    "%s: Marking all batches dirty since the new BAR (%p) "
                    "doesn't aggregate with the existing BAR (%p)\n",
                    id.GetText(), newRange.get(), curRange.get());
            }
        }
    }

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        TfDebug::Helper().Msg(
            "%s: Updating BAR at draw coord index %d from %p to %p\n",
            id.GetText(), drawCoordIndex, curRange.get(), newRange.get());
        
        if (newRangeValid) {
            TfDebug::Helper().Msg(
                "Buffer array version for the new range is %lu\n",
                newRange->GetVersion());
        }

        HdBufferSpecVector oldSpecs;
        if (curRangeValid) {
            curRange->GetBufferSpecs(&oldSpecs);
        }
        HdBufferSpecVector newSpecs;
        if (newRangeValid) {
            newRange->GetBufferSpecs(&newSpecs);
        }
        if (oldSpecs != newSpecs) {
            TfDebug::Helper().Msg("Old buffer specs:\n");
            HdBufferSpec::Dump(oldSpecs);

            TfDebug::Helper().Msg("New buffer specs:\n");
            HdBufferSpec::Dump(newSpecs);
        }
    }

    // Note: This should happen at the end since curRange is a reference to
    // the BAR at the drawCoordIndex.
    sharedData->barContainer.Set(drawCoordIndex, newRange);
}

bool HdStIsPrimvarExistentAndValid(
    HdRprim *prim,
    HdSceneDelegate *delegate,
    HdPrimvarDescriptorVector const& primvars,
    TfToken const& primvarName)
{
    SdfPath const& id = prim->GetId();
    
    for (const HdPrimvarDescriptor& pv: primvars) {
        // Note: the value check here should match
        // HdStIsInstancePrimvarExistentAndValid.
        if (pv.name == primvarName) {
            VtValue value = delegate->Get(id, pv.name);

            if (value.IsHolding<std::string>() ||
                value.IsHolding<VtStringArray>()) {
                return false;
            }

            if (value.IsArrayValued() && value.GetArraySize() == 0) {
                // Catch empty arrays
                return false;
            }
            
            return (!value.IsEmpty());
        }
    }

    return false;
}

// -----------------------------------------------------------------------------
// Constant primvar processing utilities
// -----------------------------------------------------------------------------
bool
HdStShouldPopulateConstantPrimvars(
    HdDirtyBits const *dirtyBits,
    SdfPath const& id)
{
    return HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id) ||
           HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
           HdChangeTracker::IsExtentDirty(*dirtyBits, id) ||
           HdChangeTracker::IsPrimIdDirty(*dirtyBits, id);
}

void
HdStPopulateConstantPrimvars(
    HdRprim *prim,
    HdRprimSharedData *sharedData,
    HdSceneDelegate *delegate,
    HdRenderParam *renderParam,
    HdStDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdPrimvarDescriptorVector const& constantPrimvars,
    bool *hasMirroredTransform)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = prim->GetId();
    SdfPath const& instancerId = prim->GetInstancerId();

    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const& hdStResourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    // Update uniforms
    HdBufferSourceSharedPtrVector sources;
    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        const GfMatrix4d transform = delegate->GetTransform(id);
        sharedData->bounds.SetMatrix(transform); // for CPU frustum culling

        HgiCapabilities const * capabilities =
            hdStResourceRegistry->GetHgi()->GetCapabilities();
        bool const doublesSupported = capabilities->IsSet(
            HgiDeviceCapabilitiesBitsShaderDoublePrecision);

        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->transform, transform, doublesSupported));

        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdTokens->transformInverse, transform.GetInverse(),
                doublesSupported));

        bool leftHanded = transform.IsLeftHanded();

        // If this is a prototype (has instancer),
        // also push the instancer transform separately.
        if (!instancerId.IsEmpty()) {
            // Gather all instancer transforms in the instancing hierarchy
            const VtMatrix4dArray rootTransforms = 
                prim->GetInstancerTransforms(delegate);
            VtMatrix4dArray rootInverseTransforms(rootTransforms.size());
            for (size_t i = 0; i < rootTransforms.size(); ++i) {
                rootInverseTransforms[i] = rootTransforms[i].GetInverse();
                // Flip the handedness if necessary
                leftHanded ^= rootTransforms[i].IsLeftHanded();
            }

            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdInstancerTokens->instancerTransform,
                    rootTransforms,
                    rootTransforms.size(),
                    doublesSupported));
            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdInstancerTokens->instancerTransformInverse,
                    rootInverseTransforms,
                    rootInverseTransforms.size(),
                    doublesSupported));

            // XXX: It might be worth to consider to have isFlipped
            // for non-instanced prims as well. It can improve
            // the drawing performance on older-GPUs by reducing
            // fragment shader cost, although it needs more GPU memory.

            // Set as int (GLSL needs 32-bit align for bool)
            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdTokens->isFlipped,
                    VtValue(int(leftHanded))));
        }

        if (hasMirroredTransform) {
            *hasMirroredTransform = leftHanded;
        }
    }
    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        // Note: If the scene description doesn't provide the extents, we use
        // the default constructed GfRange3d which is [FLT_MAX, -FLT_MAX],
        // which disables frustum culling for the prim.
        sharedData->bounds.SetRange(prim->GetExtent(delegate));

        GfVec3d const & localMin = drawItem->GetBounds().GetBox().GetMin();
        HdBufferSourceSharedPtr sourceMin = std::make_shared<HdVtBufferSource>(
                                           HdTokens->bboxLocalMin,
                                           VtValue(GfVec4f(
                                               localMin[0],
                                               localMin[1],
                                               localMin[2],
                                               1.0f)));
        sources.push_back(sourceMin);

        GfVec3d const & localMax = drawItem->GetBounds().GetBox().GetMax();
        HdBufferSourceSharedPtr sourceMax = std::make_shared<HdVtBufferSource>(
                                           HdTokens->bboxLocalMax,
                                           VtValue(GfVec4f(
                                               localMax[0],
                                               localMax[1],
                                               localMax[2],
                                               1.0f)));
        sources.push_back(sourceMax);
    }

    if (HdChangeTracker::IsPrimIdDirty(*dirtyBits, id)) {
        int32_t primId = prim->GetPrimId();
        HdBufferSourceSharedPtr source = std::make_shared<HdVtBufferSource>(
                                           HdTokens->primID,
                                           VtValue(primId));
        sources.push_back(source);
    }

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        sources.reserve(sources.size()+constantPrimvars.size());
        for (const HdPrimvarDescriptor& pv: constantPrimvars) {
            if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, pv.name)) {
                VtValue value = delegate->Get(id, pv.name);

                // XXX Storm doesn't support string primvars yet
                if (value.IsHolding<std::string>() ||
                    value.IsHolding<VtStringArray>()) {
                    continue;
                }

                if (value.IsArrayValued() && value.GetArraySize() == 0) {
                    // A value holding an empty array does not count as an
                    // empty value. Catch that case here.
                    //
                    // Do nothing in this case.
                } else if (!value.IsEmpty()) {
                    // Given that this is a constant primvar, if it is
                    // holding VtArray then use that as a single array
                    // value rather than as one value per element.
                    HdBufferSourceSharedPtr source =
                        std::make_shared<HdVtBufferSource>(pv.name, value,
                            value.IsArrayValued() ? value.GetArraySize() : 1);

                    TF_VERIFY(source->GetTupleType().type != HdTypeInvalid);
                    TF_VERIFY(source->GetTupleType().count > 0);
                    sources.push_back(source);
                }
            }
        }
    }

    HdBufferArrayRangeSharedPtr const& bar =
        drawItem->GetConstantPrimvarRange();

    if (HdStCanSkipBARAllocationOrUpdate(sources, bar, *dirtyBits)) {
        return;
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        static TfTokenVector internallyGeneratedPrimvars =
        {
            HdTokens->transform,
            HdTokens->transformInverse,
            HdInstancerTokens->instancerTransform,
            HdInstancerTokens->instancerTransformInverse,
            HdTokens->isFlipped,
            HdTokens->bboxLocalMin,
            HdTokens->bboxLocalMax,
            HdTokens->primID
        };
        removedSpecs = HdStGetRemovedOrReplacedPrimvarBufferSpecs(bar,
            constantPrimvars, internallyGeneratedPrimvars, bufferSpecs, id);
    }

    HdBufferArrayRangeSharedPtr range =
        hdStResourceRegistry->UpdateShaderStorageBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHint());
    
     HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetConstantPrimvarIndex(),
        sharedData,
        renderParam,
        &(renderIndex.GetChangeTracker()));

    TF_VERIFY(drawItem->GetConstantPrimvarRange()->IsValid());

    if (!sources.empty()) {
        hdStResourceRegistry->AddSources(
            drawItem->GetConstantPrimvarRange(), std::move(sources));
    }
}

// -----------------------------------------------------------------------------
// Instancer processing utilities
// -----------------------------------------------------------------------------

void
HdStUpdateInstancerData(
    HdRenderIndex &renderIndex,
    HdRenderParam *renderParam,
    HdRprim *prim,
    HdStDrawItem *drawItem,
    HdRprimSharedData *sharedData,
    HdDirtyBits rprimDirtyBits)
{
    // If there's nothing to do, bail.
    if (!(rprimDirtyBits & HdChangeTracker::DirtyInstancer)) {
        return;
    }

    // XXX: This belongs in HdRenderIndex!!!
    HdInstancer::_SyncInstancerAndParents(renderIndex, prim->GetInstancerId());

    HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
    HdChangeTracker &changeTracker = renderIndex.GetChangeTracker();

    // If the instance topology changes, we want to force an instance index
    // rebuild even if the index dirty bit isn't set...
    bool forceIndexRebuild = false;

    if (rprimDirtyBits & HdChangeTracker::DirtyInstancer) {
        // If the instancer topology has changed, we might need to change
        // how many levels we allocate in the drawing coord.
        int instancerLevels = HdInstancer::GetInstancerNumLevels(
            renderIndex, *prim); 

        if (instancerLevels != sharedData->instancerLevels) {
            sharedData->barContainer.Resize(
                drawingCoord->GetInstancePrimvarIndex(0) + instancerLevels);
            sharedData->instancerLevels = instancerLevels;

            HdStMarkGarbageCollectionNeeded(renderParam);
            HdStMarkDrawBatchesDirty(renderParam);
            forceIndexRebuild = true;
        }
    }

    /* INSTANCE PRIMVARS */
    // Populate all instance primvars by backtracing hierarachy.
    int level = 0;
    SdfPath parentId = prim->GetInstancerId();
    while (!parentId.IsEmpty()) {
        HdInstancer * const instancer = renderIndex.GetInstancer(parentId);
        if(!TF_VERIFY(instancer)) {
            return;
        }
        const int drawCoordIndex = drawingCoord->GetInstancePrimvarIndex(level);
        HdBufferArrayRangeSharedPtr const instancerRange =
            static_cast<HdStInstancer*>(instancer)->GetInstancePrimvarRange();

        // If we need to update the BAR, that indicates an instancing topology
        // change and we want to force an index rebuild.
        if (instancerRange != sharedData->barContainer.Get(drawCoordIndex)) {
            forceIndexRebuild = true;
        }

        // update instance primvar slot in the drawing coordinate.
        HdStUpdateDrawItemBAR(
            static_cast<HdStInstancer*>(instancer)->GetInstancePrimvarRange(),
            drawCoordIndex,
            sharedData,
            renderParam,
            &changeTracker);

        parentId = instancer->GetParentId();
        ++level;
    }

    /* INSTANCE INDICES */
    // Note, GetInstanceIndices will check index sizes against primvar sizes.
    // The instance indices are a cartesian product of each level, so they need
    // to be recomputed per-rprim.
    if (HdChangeTracker::IsInstanceIndexDirty(rprimDirtyBits, prim->GetId()) ||
        forceIndexRebuild) {
        parentId = prim->GetInstancerId();
        if (!parentId.IsEmpty()) {
            HdInstancer * const instancer = renderIndex.GetInstancer(parentId);
            if (!TF_VERIFY(instancer)) {
                return;
            }

            // update instance indices
            //
            // We add a zero as the first value in instanceIndices. This is 
            // added as a way of avoiding correctness issues in the instance
            // frustum cull vertex shader. This issue happens when an instanced 
            // prim has geom subsets resulting in multiple draw items. See
            // ViewFrustumCull.VertexInstancing in frustumCull.glslfx for
            // details.
            VtIntArray const originalInstanceIndices =
                static_cast<HdStInstancer*>(instancer)->
                GetInstanceIndices(prim->GetId());
            VtIntArray instanceIndices =
                VtIntArray(originalInstanceIndices.size() + 1);
            instanceIndices[0] = 0;
            std::copy(originalInstanceIndices.cbegin(),
                      originalInstanceIndices.cend(),
                      instanceIndices.begin() + 1);

            HdStResourceRegistry* const resourceRegistry =
                static_cast<HdStResourceRegistry*>(
                    renderIndex.GetResourceRegistry().get());

            // Create the bar if needed.
            if (!drawItem->GetInstanceIndexRange()) {

                // Note: we add the instance indices twice, so that frustum
                // culling can compute culledInstanceIndices as instanceIndices
                // masked by visibility.
                HdBufferSpecVector bufferSpecs;
                bufferSpecs.emplace_back(
                    HdInstancerTokens->instanceIndices,
                    HdTupleType {HdTypeInt32, 1});
                bufferSpecs.emplace_back(
                    HdInstancerTokens->culledInstanceIndices,
                    HdTupleType {HdTypeInt32, 1});

                HdBufferArrayRangeSharedPtr const range =
                    resourceRegistry->AllocateNonUniformBufferArrayRange(
                        HdTokens->topology,
                        bufferSpecs,
                        HdBufferArrayUsageHint());

                HdStUpdateDrawItemBAR(
                    range,
                    drawingCoord->GetInstanceIndexIndex(),
                    sharedData,
                    renderParam,
                    &changeTracker);

                TF_VERIFY(drawItem->GetInstanceIndexRange()->IsValid());
            }

            // If the instance index range is too big to upload, it's very
            // dangerous since the shader could index into bad memory. If we're
            // not failing on asserts, we need to zero out the index array so no
            // instances draw.
            if (!TF_VERIFY(instanceIndices.size() <=
                    drawItem->GetInstanceIndexRange()->GetMaxNumElements())) {
                instanceIndices = VtIntArray();
            }

            HdBufferSourceSharedPtrVector sources;
            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdInstancerTokens->instanceIndices,
                    VtValue(instanceIndices)));
            sources.push_back(
                std::make_shared<HdVtBufferSource>(
                    HdInstancerTokens->culledInstanceIndices,
                    VtValue(instanceIndices)));

            resourceRegistry->AddSources(
                drawItem->GetInstanceIndexRange(), std::move(sources));
        }
    }
}

bool HdStIsInstancePrimvarExistentAndValid(
    HdRenderIndex &renderIndex,
    HdRprim *rprim,
    TfToken const& primvarName)
{
    SdfPath parentId = rprim->GetInstancerId();
    while (!parentId.IsEmpty()) {
        HdInstancer * const instancer = renderIndex.GetInstancer(parentId);
        if (!TF_VERIFY(instancer)) {
            return false;
        }

        HdPrimvarDescriptorVector const primvars =
            instancer->GetDelegate()->GetPrimvarDescriptors(instancer->GetId(),
                HdInterpolationInstance);
        
        for (const HdPrimvarDescriptor& pv : primvars) {
            // We're looking for a primvar with the given name at any level
            // (since instance primvars aggregate).  Note: the value check here
            // must match HdStIsPrimvarExistentAndValid.
            if (pv.name == primvarName) {
                const VtValue value =
                    instancer->GetDelegate()->Get(instancer->GetId(), pv.name);
                if (value.IsHolding<std::string>() ||
                    value.IsHolding<VtStringArray>()) {
                    return false;
                }
                if (value.IsArrayValued() && value.GetArraySize() == 0) {
                    return false;
                }
                return (!value.IsEmpty());
            }
        }

        parentId = instancer->GetParentId();
    }

    return false;
}

// -----------------------------------------------------------------------------
// Topological invisibility utility
// -----------------------------------------------------------------------------

// Construct and return a buffer source representing visibility of the
// topological entity (e.g., face, curve, point) using one bit for the
// visibility of each indexed entity.
static HdBufferSourceSharedPtr
_GetBitmaskEncodedVisibilityBuffer(VtIntArray invisibleIndices,
                                    int numTotalIndices,
                                    TfToken const& bufferName,
                                    SdfPath const& rprimId)
{
    size_t numBitsPerUInt = std::numeric_limits<uint32_t>::digits; // i.e, 32
    size_t numUIntsNeeded = ceil(numTotalIndices/(float) numBitsPerUInt);
    // Initialize all bits to 1 (visible)
    VtArray<uint32_t> visibility(numUIntsNeeded,
                                 std::numeric_limits<uint32_t>::max());

    for (VtIntArray::const_iterator i = invisibleIndices.begin(),
                                  end = invisibleIndices.end(); i != end; ++i) {
        if (*i >= numTotalIndices || *i < 0) {
            // This invisible index is out of range.  Ignore it silently.
            continue;
        }
        const size_t arrayIndex = *i/numBitsPerUInt;
        const size_t bitIndex   = *i % numBitsPerUInt;
        visibility[arrayIndex] &= ~(1 << bitIndex); // set bit to 0
    }

    return std::make_shared<HdVtBufferSource>(
        bufferName, VtValue(visibility), numUIntsNeeded);
}

void HdStProcessTopologyVisibility(
    VtIntArray invisibleElements,
    int numTotalElements,
    VtIntArray invisiblePoints,
    int numTotalPoints,
    HdRprimSharedData *sharedData,
    HdStDrawItem *drawItem,
    HdRenderParam *renderParam,
    HdChangeTracker *changeTracker,
    HdStResourceRegistrySharedPtr const &resourceRegistry,
    SdfPath const& rprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdBufferArrayRangeSharedPtr tvBAR = drawItem->GetTopologyVisibilityRange();
    HdBufferSourceSharedPtrVector sources;

    // For the general case wherein there is no topological invisibility, we
    // don't create a BAR.
    // If any topological invisibility is authored (points/elements), create the
    // BAR with both sources. Once the BAR is created, we don't attempt to
    // delete it when there's no topological invisibility authored; we simply
    // reset the bits to make all elements/points visible.
    if (tvBAR || (!invisibleElements.empty() || !invisiblePoints.empty())) {
        sources.push_back(_GetBitmaskEncodedVisibilityBuffer(
                                invisibleElements,
                                numTotalElements,
                                HdTokens->elementsVisibility,
                                rprimId));
         sources.push_back(_GetBitmaskEncodedVisibilityBuffer(
                                invisiblePoints,
                                numTotalPoints,
                                HdTokens->pointsVisibility,
                                rprimId));
    }

    // Exit early if the BAR doesn't need to be allocated.
    if (!tvBAR && sources.empty()) return;

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    bool barNeedsReallocation = false;
    if (tvBAR) {
        HdBufferSpecVector oldBufferSpecs;
        tvBAR->GetBufferSpecs(&oldBufferSpecs);
        if (oldBufferSpecs != bufferSpecs) {
            barNeedsReallocation = true;
        }
    }

    // XXX: Transition this code to use the Update* method instead.
    if (!tvBAR || barNeedsReallocation) {
        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateShaderStorageBufferArrayRange(
                HdTokens->topologyVisibility,
                bufferSpecs,
                HdBufferArrayUsageHint());
        sharedData->barContainer.Set(
            drawItem->GetDrawingCoord()->GetTopologyVisibilityIndex(), range);

        HdStMarkDrawBatchesDirty(renderParam);

        if (barNeedsReallocation) {
            HdStMarkGarbageCollectionNeeded(renderParam);
        }
    }

    TF_VERIFY(drawItem->GetTopologyVisibilityRange()->IsValid());

    resourceRegistry->AddSources(
        drawItem->GetTopologyVisibilityRange(), std::move(sources));
}

bool
HdStIsEnabledSharedVertexPrimvar()
{
    static bool enabled =
        (TfGetEnvSetting(HDST_ENABLE_SHARED_VERTEX_PRIMVAR) == 1);
    return enabled;
}

uint64_t
HdStComputeSharedPrimvarId(
    uint64_t baseId,
    HdBufferSourceSharedPtrVector const &sources,
    HdStComputationComputeQueuePairVector const &computations)
{
    size_t primvarId = baseId;
    for (HdBufferSourceSharedPtr const &bufferSource : sources) {
        size_t sourceId = bufferSource->ComputeHash();
        primvarId = ArchHash64((const char*)&sourceId,
                               sizeof(sourceId), primvarId);

        if (bufferSource->HasPreChainedBuffer()) {
            HdBufferSourceSharedPtr src = bufferSource->GetPreChainedBuffer();

            while (src) {
                size_t chainedSourceId = bufferSource->ComputeHash();
                primvarId = ArchHash64((const char*)&chainedSourceId,
                                       sizeof(chainedSourceId), primvarId);

                src = src->GetPreChainedBuffer();
            }
        }
    }

    for (const auto& computation : computations) {
        if (std::shared_ptr<HdSt_OsdRefineComputationGPU> refinedComputation =
            std::dynamic_pointer_cast<HdSt_OsdRefineComputationGPU>(
                computation.first)) {
            primvarId = TfHash::Combine(primvarId, 
                                        refinedComputation->GetInterpolation());
        }   
    }
    HdBufferSpecVector bufferSpecs;
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);

    return TfHash::Combine(primvarId, bufferSpecs);
}

void 
HdStGetBufferSpecsFromCompuations(
    HdStComputationComputeQueuePairVector const& computations,
    HdBufferSpecVector *bufferSpecs) 
{
    for (auto const &compQueuePair : computations) {
        HdStComputationSharedPtr const& comp = compQueuePair.first;
        if (comp->IsValid()) {
            comp->GetBufferSpecs(bufferSpecs);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
