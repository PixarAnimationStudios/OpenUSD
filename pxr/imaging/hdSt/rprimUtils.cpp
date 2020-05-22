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
#include "pxr/imaging/hdSt/rprimUtils.h"

#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/mixinShader.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/diagnostic.h"

#include "pxr/imaging/hio/glslfx.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

// -----------------------------------------------------------------------------
// Primvar descriptor filtering utilities
// -----------------------------------------------------------------------------
static bool
_IsEnabledPrimvarFiltering(HdStDrawItem const * drawItem) {
    HdStShaderCodeSharedPtr materialShader = drawItem->GetMaterialShader();
    return materialShader && materialShader->IsEnabledPrimvarFiltering();
}

static TfTokenVector
_GetFilterNames(HdRprim const * prim,
             HdStDrawItem const * drawItem,
             HdStInstancer const * instancer = nullptr)
{
    TfTokenVector filterNames = prim->GetBuiltinPrimvarNames();

    HdStShaderCodeSharedPtr materialShader = drawItem->GetMaterialShader();
    if (materialShader) {
        TfTokenVector const & names = materialShader->GetPrimvarNames();
        filterNames.insert(filterNames.end(), names.begin(), names.end());
    }
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
    HdInterpolation interpolation)
{
    HdPrimvarDescriptorVector primvars =
        prim->GetPrimvarDescriptors(delegate, interpolation);

    if (_IsEnabledPrimvarFiltering(drawItem)) {
        TfTokenVector filterNames = _GetFilterNames(prim, drawItem);

        return _FilterPrimvarDescriptors(primvars, filterNames);
    }

    return primvars;
}

HdPrimvarDescriptorVector
HdStGetInstancerPrimvarDescriptors(
    HdStInstancer const * instancer,
    HdRprim const * prim,
    HdStDrawItem const * drawItem,
    HdSceneDelegate * delegate)
{
    HdPrimvarDescriptorVector primvars =
        delegate->GetPrimvarDescriptors(instancer->GetId(),
                                        HdInterpolationInstance);

    if (_IsEnabledPrimvarFiltering(drawItem)) {
        TfTokenVector filterNames = _GetFilterNames(prim, drawItem, instancer);

        return _FilterPrimvarDescriptors(primvars, filterNames);
    }

    return primvars;
}

// -----------------------------------------------------------------------------
// Material shader utility
// -----------------------------------------------------------------------------
HDST_API
HdStShaderCodeSharedPtr
HdStGetMaterialShader(
    HdRprim const * prim,
    HdSceneDelegate * delegate,
    std::string const & mixinSource)
{
    SdfPath const & materialId = prim->GetMaterialId();

    // Resolve the prim's material or use the fallback material.
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdStMaterial const * material = static_cast<HdStMaterial const *>(
            renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
    if (material == nullptr) {
        TF_DEBUG(HD_RPRIM_UPDATED).Msg("Using fallback material for %s\n",
            prim->GetId().GetText());

        material = static_cast<HdStMaterial const *>(
                renderIndex.GetFallbackSprim(HdPrimTypeTokens->material));
    }

    // Augment the shader source if mixinSource is provided.
    HdStShaderCodeSharedPtr shaderCode = material->GetShaderCode();
    if (!mixinSource.empty()) {
        shaderCode.reset(new HdStMixinShader(mixinSource, shaderCode));
    }

    return shaderCode;
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
    HdComputationSharedPtrVector const& computations,
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
        sources, HdComputationSharedPtrVector(), curRange, dirtyBits);
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

    // Get the buffer specs for the existing BAR...
    HdBufferSpecVector curBarSpecs;
    curRange->GetBufferSpecs(&curBarSpecs);

    // ... and check if it has buffers that are neither in the new source list
    // nor are internally generated.
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
    TfTokenVector const& internallyGeneratedPrimvarNames,
    SdfPath const& rprimId)
{
    return HdStGetRemovedPrimvarBufferSpecs(curRange, newPrimvarDescs,
        HdExtComputationPrimvarDescriptorVector(),
        internallyGeneratedPrimvarNames, rprimId);
}

void
HdStUpdateDrawItemBAR(
    HdBufferArrayRangeSharedPtr const& newRange,
    int drawCoordIndex,
    HdRprimSharedData *sharedData,
    HdRenderIndex &renderIndex)
{
    if (!sharedData) {
        TF_CODING_ERROR("Null shared data ptr received\n");
        return;
    }

    HdBufferArrayRangeSharedPtr const& curRange =
        sharedData->barContainer.Get(drawCoordIndex);

    if (curRange == newRange) {
        // Nothing to do. The draw item's BAR hasn't been changed.
        return;
    }

    SdfPath const& id = sharedData->rprimID;

    if (HdStIsValidBAR(curRange)) {
        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking garbage collection needed to possibly reclaim BAR %p"
            " at draw coord index %d\n",
            id.GetText(), (void*)curRange.get(), drawCoordIndex);

        renderIndex.GetChangeTracker().SetGarbageCollectionNeeded();
        
        // If the new BAR is associated with a buffer array that fails the
        // aggregation test (used during batching), we need to use the big
        // hammer, and rebuild all draw batches.
        if (!newRange->IsAggregatedWith(curRange)) {
            TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                "%s: Marking all batches dirty since the new BAR (%p) doesn't"
                " aggregate with the existing BAR (%p)\n",
                id.GetText(), (void*)newRange.get(), (void*)curRange.get());

            renderIndex.GetChangeTracker().MarkBatchesDirty();
        }
    }

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        TfDebug::Helper().Msg(
            "%s: Updating BAR at draw coord index %d from %p to %p\n",
            id.GetText(), drawCoordIndex, (void*)curRange.get(),
            (void*)newRange.get());

        if (HdStIsValidBAR(curRange)) {
            TfDebug::Helper().Msg("Old buffer specs:\n");
            HdBufferSpecVector oldSpecs;
            curRange->GetBufferSpecs(&oldSpecs);
            HdBufferSpec::Dump(oldSpecs);

            TfDebug::Helper().Msg("New buffer specs:\n");
            HdBufferSpecVector newSpecs;
            newRange->GetBufferSpecs(&newSpecs);
            HdBufferSpec::Dump(newSpecs);
        }
    }

    // Note: This should happen at the end since curRange is a reference to
    // the BAR at the drawCoordIndex.
    sharedData->barContainer.Set(drawCoordIndex, newRange);
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
    HdRprim* prim,
    HdRprimSharedData *sharedData,
    HdSceneDelegate* delegate,
    HdDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdPrimvarDescriptorVector const& constantPrimvars)
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
        GfMatrix4d transform = delegate->GetTransform(id);
        sharedData->bounds.SetMatrix(transform); // for CPU frustum culling

        HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                           HdTokens->transform,
                                           transform));
        sources.push_back(source);
        source.reset(new HdVtBufferSource(HdTokens->transformInverse,
                                          transform.GetInverse()));
        sources.push_back(source);

        // If this is a prototype (has instancer),
        // also push the instancer transform separately.
        if (!instancerId.IsEmpty()) {
            // Gather all instancer transforms in the instancing hierarchy
            VtMatrix4dArray rootTransforms = 
                prim->GetInstancerTransforms(delegate);
            VtMatrix4dArray rootInverseTransforms(rootTransforms.size());
            bool leftHanded = transform.IsLeftHanded();
            for (size_t i = 0; i < rootTransforms.size(); ++i) {
                rootInverseTransforms[i] = rootTransforms[i].GetInverse();
                // Flip the handedness if necessary
                leftHanded ^= rootTransforms[i].IsLeftHanded();
            }

            source.reset(new HdVtBufferSource(
                             HdInstancerTokens->instancerTransform,
                             rootTransforms,
                             rootTransforms.size()));
            sources.push_back(source);
            source.reset(new HdVtBufferSource(
                             HdInstancerTokens->instancerTransformInverse,
                             rootInverseTransforms,
                             rootInverseTransforms.size()));
            sources.push_back(source);

            // XXX: It might be worth to consider to have isFlipped
            // for non-instanced prims as well. It can improve
            // the drawing performance on older-GPUs by reducing
            // fragment shader cost, although it needs more GPU memory.

            // Set as int (GLSL needs 32-bit align for bool)
            source.reset(new HdVtBufferSource(
                             HdTokens->isFlipped, VtValue(int(leftHanded))));
            sources.push_back(source);
        }
    }
    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        // Note: If the scene description doesn't provide the extents, we use
        // the default constructed GfRange3d which is [FLT_MAX, -FLT_MAX],
        // which disables frustum culling for the prim.
        sharedData->bounds.SetRange(prim->GetExtent(delegate));

        GfVec3d const & localMin = drawItem->GetBounds().GetBox().GetMin();
        HdBufferSourceSharedPtr sourceMin(new HdVtBufferSource(
                                           HdTokens->bboxLocalMin,
                                           VtValue(GfVec4f(
                                               localMin[0],
                                               localMin[1],
                                               localMin[2],
                                               1.0f))));
        sources.push_back(sourceMin);

        GfVec3d const & localMax = drawItem->GetBounds().GetBox().GetMax();
        HdBufferSourceSharedPtr sourceMax(new HdVtBufferSource(
                                           HdTokens->bboxLocalMax,
                                           VtValue(GfVec4f(
                                               localMax[0],
                                               localMax[1],
                                               localMax[2],
                                               1.0f))));
        sources.push_back(sourceMax);
    }

    if (HdChangeTracker::IsPrimIdDirty(*dirtyBits, id)) {
        int32_t primId = prim->GetPrimId();
        HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                           HdTokens->primID,
                                           VtValue(primId)));
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
                    HdBufferSourceSharedPtr source(
                        new HdVtBufferSource(pv.name, value,
                            value.IsArrayValued() ? value.GetArraySize() : 1));

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
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, constantPrimvars, 
            internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

    HdBufferArrayRangeSharedPtr range =
        hdStResourceRegistry->UpdateShaderStorageBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHint());
    
     HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetConstantPrimvarIndex(),
        sharedData,
        renderIndex);

    TF_VERIFY(drawItem->GetConstantPrimvarRange()->IsValid());

    if (!sources.empty()) {
        hdStResourceRegistry->AddSources(
            drawItem->GetConstantPrimvarRange(), std::move(sources));
    }
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
            HF_VALIDATION_WARN(rprimId,
                "Topological invisibility data (%d) is not in the range [0, %d)"
                ".", *i, numTotalIndices);
            continue;
        }
        size_t arrayIndex = *i/numBitsPerUInt;
        size_t bitIndex   = *i % numBitsPerUInt;
        visibility[arrayIndex] &= ~(1 << bitIndex); // set bit to 0
    }

    return HdBufferSourceSharedPtr(
        new HdVtBufferSource(bufferName, VtValue(visibility), numUIntsNeeded));
}

void HdStProcessTopologyVisibility(
    VtIntArray invisibleElements,
    int numTotalElements,
    VtIntArray invisiblePoints,
    int numTotalPoints,
    HdRprimSharedData *sharedData,
    HdStDrawItem *drawItem,
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

        changeTracker->MarkBatchesDirty();

        if (barNeedsReallocation) {
            changeTracker->SetGarbageCollectionNeeded();
        }
    }

    TF_VERIFY(drawItem->GetTopologyVisibilityRange()->IsValid());

    resourceRegistry->AddSources(
        drawItem->GetTopologyVisibilityRange(), std::move(sources));
}

PXR_NAMESPACE_CLOSE_SCOPE
