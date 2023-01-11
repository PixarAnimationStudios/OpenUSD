//
// Copyright 2016 Pixar
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

#include "pxr/imaging/hdSt/simpleText.h"
#include "pxr/imaging/hdSt/simpleTextTopology.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/basisCurvesComputations.h"
#include "pxr/imaging/hdSt/basisCurvesShaderKey.h"
#include "pxr/imaging/hdSt/basisCurvesTopology.h"
#include "pxr/imaging/hdSt/basisCurves.h"
#include "pxr/imaging/hdSt/computation.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/primUtils.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textShaderKey.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/base/arch/hash.h"
#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStSimpleText::HdStSimpleText(SdfPath const& id)
    : HdSimpleText(id)
    , _topology()
    , _topologyId(0)
    , _customDirtyBitsInUse(0)
    , _refineLevel(0)
    , _displayOpacity(false)
    , _lineTopology()
    , _lineTopologyId(0)
{
    /*NOTHING*/
}


HdStSimpleText::~HdStSimpleText() = default;

void
HdStSimpleText::UpdateRenderTag(HdSceneDelegate *delegate,
    HdRenderParam *renderParam)
{
    HdStUpdateRenderTag(delegate, renderParam, this);
}

void
HdStSimpleText::Sync(HdSceneDelegate *delegate,
    HdRenderParam *renderParam,
    HdDirtyBits *dirtyBits,
    TfToken const &reprToken)
{
    bool updateMaterialTag = false;
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        HdStSetMaterialId(delegate, renderParam, this);
        updateMaterialTag = true;
    }
    if (*dirtyBits & HdChangeTracker::DirtyDisplayStyle) {
        updateMaterialTag = true;
    }

    // Check if either the material or geometric shaders need updating for
    // draw items of all the reprs.
    bool updateMaterialShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyMaterialId |
        HdChangeTracker::NewRepr)) {
        updateMaterialShader = true;
    }

    bool updateGeometricShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyDisplayStyle |
        HdChangeTracker::DirtyMaterialId |
        HdChangeTracker::DirtyInstancer |
        HdChangeTracker::NewRepr)) {
        updateGeometricShader = true;
    }

    bool displayOpacity = _displayOpacity;
    _UpdateRepr(delegate, renderParam, reprToken, dirtyBits);

    if (updateMaterialTag ||
        (GetMaterialId().IsEmpty() && displayOpacity != _displayOpacity)) {
        for (auto const &reprPair : _reprs) {
            HdReprSharedPtr repr = reprPair.second;
            HdStDrawItem* drawItem = static_cast<HdStDrawItem*>(repr->GetDrawItem(0));
            HdStRenderParam* const stRenderParam =
                static_cast<HdStRenderParam*>(renderParam);
            TfToken prevMaterialTag = drawItem->GetMaterialTag();
            TfToken newMaterialTag;

            // Opinion precedence:
            //   Show occluded selection > Material opinion > displayOpacity primvar
            const HdStMaterial *material =
                static_cast<const HdStMaterial*>(
                    delegate->GetRenderIndex().GetSprim(
                        HdPrimTypeTokens->material, GetMaterialId()));
            if (material) {
                newMaterialTag = material->GetMaterialTag();
            }
            else {
                newMaterialTag = HdStMaterialTagTokens->translucent;
            }

            if (prevMaterialTag != newMaterialTag) {
                stRenderParam->DecreaseMaterialTagCount(prevMaterialTag);
                stRenderParam->IncreaseMaterialTagCount(newMaterialTag);
                drawItem->SetMaterialTag(newMaterialTag);
                // Trigger invalidation of the draw items cache of the render pass(es).
                HdStMarkMaterialTagsDirty(renderParam);
            }

            // Line drawItems.
            const HdRepr::DrawItemUniquePtrVector& drawItems = repr->GetDrawItems();
            for (size_t index = 1; index < drawItems.size(); ++index)
            {
                HdStDrawItem* lineDrawItem = static_cast<HdStDrawItem*>(drawItems[index].get());
                if (!TF_VERIFY(lineDrawItem))
                    continue;

                TfToken prevLineMaterialTag = lineDrawItem->GetMaterialTag();

                if (prevLineMaterialTag != newMaterialTag) {
                    stRenderParam->DecreaseMaterialTagCount(prevLineMaterialTag);
                    stRenderParam->IncreaseMaterialTagCount(newMaterialTag);
                    lineDrawItem->SetMaterialTag(newMaterialTag);

                    // Trigger invalidation of the draw items cache of the render pass(es).
                    HdStMarkMaterialTagsDirty(renderParam);
                }
            }
        }
    }

    if (updateMaterialShader || updateGeometricShader) {
        _UpdateShadersForAllReprs(delegate, renderParam,
            updateMaterialShader, updateGeometricShader);
    }

    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdStSimpleText::_UpdateDrawItem(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdStDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdRprimSharedData& sharedData,
    size_t drawItemIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const &id = GetId();

    /* MATERIAL SHADER (may affect subsequent primvar population) */
    if ((*dirtyBits & HdChangeTracker::NewRepr) ||
        HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        if(drawItemIndex == 0)
            drawItem->SetMaterialNetworkShader(HdStGetMaterialNetworkShader(this, sceneDelegate));
        else
        {
            HdStMaterial const* material = static_cast<HdStMaterial const*>(
                sceneDelegate->GetRenderIndex().GetFallbackSprim(HdPrimTypeTokens->material));
            drawItem->SetMaterialNetworkShader(material->GetMaterialNetworkShader());
        }
    }

    // Reset value of _displayOpacity
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _displayOpacity = false;
    }

    /* INSTANCE PRIMVARS */
    _UpdateInstancer(sceneDelegate, dirtyBits);
    HdStUpdateInstancerData(sceneDelegate->GetRenderIndex(),
        renderParam,
        this,
        drawItem,
        &sharedData,
        *dirtyBits);

    _displayOpacity = _displayOpacity ||
        HdStIsInstancePrimvarExistentAndValid(
            sceneDelegate->GetRenderIndex(), this, HdTokens->displayOpacity);

    /* CONSTANT PRIMVARS, TRANSFORM, EXTENT AND PRIMID */
    if (HdStShouldPopulateConstantPrimvars(dirtyBits, id)) {
        HdPrimvarDescriptorVector constantPrimvars =
            HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                HdInterpolationConstant);

        HdStPopulateConstantPrimvars(this,
            &sharedData,
            sceneDelegate,
            renderParam,
            drawItem,
            dirtyBits,
            constantPrimvars);

        _displayOpacity = _displayOpacity ||
            HdStIsPrimvarExistentAndValid(this, sceneDelegate,
                constantPrimvars, HdTokens->displayOpacity);
    }

    /* TOPOLOGY */
    // XXX: _PopulateTopology should be split into two phase
    //      for scene dirtybits and for repr dirtybits.
    if (*dirtyBits & (HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyDisplayStyle
        | DirtyIndices)) {
        if (drawItemIndex == 0) {
            // Text topology.
            _PopulateTopology(
                sceneDelegate, renderParam, drawItem, dirtyBits);
        }
        else {
            // Decoration line(underline, overline, strikethrough) topology.
            _PopulateLineTopology(
                sceneDelegate, renderParam, drawItem, dirtyBits, sharedData, drawItemIndex);
        }
    }

    /* PRIMVAR */
    if ((*dirtyBits & HdChangeTracker::NewRepr) ||
        HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        if (drawItemIndex == 0) {
            // Text vertex.
            _PopulateVertexPrimvars(
                sceneDelegate, renderParam, drawItem, dirtyBits);
        }
        else {
            // Decoration line(underline, overline, strikethrough) vertex.
            _PopulateLineVertexPrimvars(
                sceneDelegate, renderParam, drawItem, dirtyBits, sharedData, drawItemIndex);
        }
    }

    // Topology and VertexPrimvar may be null.
    TF_VERIFY(drawItem->GetConstantPrimvarRange());
}

void
HdStSimpleText::_PopulateTopology(HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdStDrawItem *drawItem,
    HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const &id = GetId();
    HdStResourceRegistrySharedPtr const &resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());
    HdChangeTracker &changeTracker =
        sceneDelegate->GetRenderIndex().GetChangeTracker();

    if (*dirtyBits & HdChangeTracker::DirtyDisplayStyle) {
        HdDisplayStyle ds = GetDisplayStyle(sceneDelegate);
        _refineLevel = ds.refineLevel;
    }

    // XXX: is it safe to get topology even if it's not dirty?
    bool dirtyTopology = HdChangeTracker::IsTopologyDirty(*dirtyBits, id);

    if (dirtyTopology || HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {

        const HdSimpleTextTopology &srcTopology =
            GetSimpleTextTopology(sceneDelegate);

        // compute id.
        _topologyId = srcTopology.ComputeHash();
        bool refined = (_refineLevel > 0);
        _topologyId = ArchHash64((const char*)&refined, sizeof(refined),
            _topologyId);

        // ask the registry if there is a sharable simpleText topology
        HdInstance<HdSt_SimpleTextTopologySharedPtr> topologyInstance =
            resourceRegistry->RegisterSimpleTextTopology(_topologyId);

        if (topologyInstance.IsFirstInstance()) {
            // if this is the first instance, create a new stream topology
            // representation and use that.
            HdSt_SimpleTextTopologySharedPtr topology =
                HdSt_SimpleTextTopology::New(srcTopology);

            topologyInstance.SetValue(topology);
        }

        _topology = topologyInstance.GetValue();
        TF_VERIFY(_topology);

        // hash collision check
        if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
            TF_VERIFY(srcTopology == *_topology);
        }
    }

    if ((*dirtyBits & DirtyIndices) == 0) return;
    TfToken indexToken;
    indexToken = HdTokens->indices;

    HdInstance<HdBufferArrayRangeSharedPtr> rangeInstance =
        resourceRegistry->RegisterSimpleTextIndexRange(
            _topologyId, indexToken);

    if (rangeInstance.IsFirstInstance()) {
        HdBufferSourceSharedPtrVector sources;
        HdBufferSpecVector bufferSpecs;

        sources.push_back(
            _topology->GetTriangleIndexBuilderComputation());

        HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

        HdBufferArrayUsageHint usageHint =
                HdBufferArrayUsageHintBitsIndex |
                HdBufferArrayUsageHintBitsStorage;
        // Set up the usage hints to mark topology as varying if
        // there is a previously set range.
        if (drawItem->GetTopologyRange()) {
            usageHint |= HdBufferArrayUsageHintBitsSizeVarying;
        }

        // allocate new range
        HdBufferArrayRangeSharedPtr range
            = resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs, usageHint);

        // add sources to update queue
        resourceRegistry->AddSources(range, std::move(sources));
        rangeInstance.SetValue(range);
    }

    HdBufferArrayRangeSharedPtr const &newRange = rangeInstance.GetValue();

    HdStUpdateDrawItemBAR(
        newRange,
        drawItem->GetDrawingCoord()->GetTopologyIndex(),
        &_sharedData,
        renderParam,
        &changeTracker);
}


void
HdStSimpleText::_PopulateLineTopology(HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdStDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdRprimSharedData& sharedData,
    int drawItemIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const &id = GetId();
    HdStResourceRegistrySharedPtr const &resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());
    HdChangeTracker &changeTracker =
        sceneDelegate->GetRenderIndex().GetChangeTracker();

    if (*dirtyBits & HdChangeTracker::DirtyDisplayStyle) {
        HdDisplayStyle ds = GetDisplayStyle(sceneDelegate);
        _refineLevel = ds.refineLevel;
        _occludedSelectionShowsThrough = ds.occludedSelectionShowsThrough;
        _pointsShadingEnabled = ds.pointsShadingEnabled;
    }

    bool dirtyTopology = HdChangeTracker::IsTopologyDirty(*dirtyBits, id);

    if (dirtyTopology || HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id)) {
        // Hard code the count of underline and overline curve points to 2.
        const VtIntArray curveVertexCounts = { 2 };

        HdBasisCurvesTopology srcLineTopology(
            HdTokens->linear, HdTokens->bezier, HdTokens->nonperiodic,
            curveVertexCounts,
            VtIntArray());       

        // Topological visibility (of points, curves) comes in as DirtyTopology.
        // We encode this information in a separate BAR.
        if (dirtyTopology) {
            // The points primvar is permitted to be larger than the number of
            // CVs implied by the topology.  So here we allow for
            // invisiblePoints being larger as well.
            size_t minInvisiblePointsCapacity = srcLineTopology.GetNumPoints();

            HdStProcessTopologyVisibility(
                srcLineTopology.GetInvisibleCurves(),
                srcLineTopology.GetNumCurves(),
                srcLineTopology.GetInvisiblePoints(),
                minInvisiblePointsCapacity,
                &sharedData,
                drawItem,
                renderParam,
                &changeTracker,
                resourceRegistry,
                id);
        }

        // compute id.
        _lineTopologyId = srcLineTopology.ComputeHash();
        bool refined = (_refineLevel > 0);
        _lineTopologyId = ArchHash64((const char*)&refined, sizeof(refined),
            _lineTopologyId);

        // ask the registry if there is a sharable basisCurves topology
        HdInstance<HdSt_BasisCurvesTopologySharedPtr> topologyInstance =
            resourceRegistry->RegisterBasisCurvesTopology(_lineTopologyId);

        if (topologyInstance.IsFirstInstance()) {
            // if this is the first instance, create a new stream topology
            // representation and use that.
            HdSt_BasisCurvesTopologySharedPtr overlineTopology =
                HdSt_BasisCurvesTopology::New(srcLineTopology);

            topologyInstance.SetValue(overlineTopology);
        }

        _lineTopology = topologyInstance.GetValue();
        TF_VERIFY(_lineTopology);

        // hash collision check
        if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
            TF_VERIFY(srcLineTopology == *_lineTopology);
        }
    }

    // bail out if the index bar is already synced
    TfToken indexToken = HdTokens->indices;
    {
        HdInstance<HdBufferArrayRangeSharedPtr> rangeInstance =
            resourceRegistry->RegisterBasisCurvesIndexRange(
                _lineTopologyId, indexToken);

        if (rangeInstance.IsFirstInstance()) {
            HdBufferSourceSharedPtrVector sources;
            HdBufferSpecVector bufferSpecs;

            sources.push_back(_lineTopology->GetIndexBuilderComputation(true));

            HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

            HdBufferArrayUsageHint usageHint =
                HdBufferArrayUsageHintBitsIndex |
                HdBufferArrayUsageHintBitsStorage;
            // Set up the usage hints to mark topology as varying if
            // there is a previously set range.
            if (drawItem->GetTopologyRange()) {
                usageHint |= HdBufferArrayUsageHintBitsSizeVarying;
            }

            // allocate new range
            HdBufferArrayRangeSharedPtr range
                = resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->topology, bufferSpecs, usageHint);

            // add sources to update queue
            resourceRegistry->AddSources(range, std::move(sources));
            rangeInstance.SetValue(range);
        }

        HdBufferArrayRangeSharedPtr const &newRange = rangeInstance.GetValue();

        HdStUpdateDrawItemBAR(
            newRange,
            drawItem->GetDrawingCoord()->GetTopologyIndex(),
            &sharedData,
            renderParam,
            &changeTracker);
    }
}

void
HdStSimpleText::_PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdStDrawItem *drawItem,
    HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const &id = GetId();
    HdStResourceRegistrySharedPtr const &resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // The "points" attribute is expected to be in this list.
    HdPrimvarDescriptorVector primvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
            HdInterpolationVertex);

    HdExtComputationPrimvarDescriptorVector compPrimvars =
        sceneDelegate->GetExtComputationPrimvarDescriptors(id,
            HdInterpolationVertex);

    HdBufferSourceSharedPtrVector sources;
    HdBufferSourceSharedPtrVector reserveOnlySources;
    HdBufferSourceSharedPtrVector separateComputationSources;
    HdStComputationComputeQueuePairVector computations;
    sources.reserve(primvars.size());

    HdSt_GetExtComputationPrimvarsComputations(
        id,
        sceneDelegate,
        compPrimvars,
        *dirtyBits,
        &sources,
        &reserveOnlySources,
        &separateComputationSources,
        &computations);

    for (HdPrimvarDescriptor const &primvar : primvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name))
            continue;

        // TODO: We don't need to pull primvar metadata every time a value
        // changes, but we need support from the delegate.

        // Having a null topology is possible, but shouldn't happen when there
        // are points
        if (!_topology) {
            if (primvar.name == HdTokens->points) {
                TF_CODING_ERROR("No topology set for SimpleText %s",
                    id.GetName().c_str());
                break;
            }
            continue;
        }

        //assert name not in range.bufferArray.GetResources()
        if (primvar.name != HdTokens->linePoints) {
            VtValue value = GetPrimvar(sceneDelegate, primvar.name);
            if (!value.IsEmpty()) {
                HdBufferSourceSharedPtr source(
                    new HdVtBufferSource(primvar.name, value));
                sources.push_back(source);
            }
        }
    }

    HdBufferArrayRangeSharedPtr const &bar = drawItem->GetVertexPrimvarRange();

    if (HdStCanSkipBARAllocationOrUpdate(sources, computations, bar,
        *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        TfTokenVector internallyGeneratedPrimvars; // none
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars,
            compPrimvars, internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdBufferSpec::GetBufferSpecs(reserveOnlySources, &bufferSpecs);
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);

    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsVertex);

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));

    if (!sources.empty() || !computations.empty()) {
        // If sources or computations are to be queued against the resulting
        // BAR, we expect it to be valid.
        if (!TF_VERIFY(drawItem->GetVertexPrimvarRange()->IsValid())) {
            return;
        }
    }

    // add sources to update queue
    if (!sources.empty()) {
        resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
            std::move(sources));
    }
    // add gpu computations to queue.
    for (auto const &compQueuePair : computations) {
        HdStComputationSharedPtr const &comp = compQueuePair.first;
        HdStComputeQueue queue = compQueuePair.second;
        resourceRegistry->AddComputation(
            drawItem->GetVertexPrimvarRange(), comp, queue);
    }
    if (!separateComputationSources.empty()) {
        TF_FOR_ALL(it, separateComputationSources) {
            resourceRegistry->AddSource(*it);
        }
    }
}

void
HdStSimpleText::_PopulateLineVertexPrimvars(HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdStDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdRprimSharedData& sharedData,
    int drawItemIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const &id = GetId();
    HdStResourceRegistrySharedPtr const &resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // The "points" attribute is expected to be in this list.
    HdPrimvarDescriptorVector primvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
            HdInterpolationVertex);

    HdExtComputationPrimvarDescriptorVector compPrimvars =
        sceneDelegate->GetExtComputationPrimvarDescriptors(id,
            HdInterpolationVertex);

    HdBufferSourceSharedPtrVector sources;
    HdBufferSourceSharedPtrVector reserveOnlySources;
    HdBufferSourceSharedPtrVector separateComputationSources;
    HdStComputationComputeQueuePairVector computations;
    sources.reserve(primvars.size());

    HdSt_GetExtComputationPrimvarsComputations(
        id,
        sceneDelegate,
        compPrimvars,
        *dirtyBits,
        &sources,
        &reserveOnlySources,
        &separateComputationSources,
        &computations);

    for (HdPrimvarDescriptor const &primvar : primvars) {
        if (primvar.name == HdTokens->linePoints)
        {
            // Get line points data from sceneDelegate.
            VtValue value = GetPrimvar(sceneDelegate, primvar.name);
            if (!value.IsEmpty()) {
                VtVec3fArray lineGeometries = value.Get<VtVec3fArray>();
                VtVec3fArray currentLineGeometry;
                currentLineGeometry.push_back(lineGeometries[(drawItemIndex - 1) * 2]);
                currentLineGeometry.push_back(lineGeometries[(drawItemIndex - 1) * 2 + 1]);
                HdBufferSourceSharedPtr source(
                    new HdVtBufferSource(HdTokens->points, VtValue(currentLineGeometry)));
                sources.push_back(source);
            }
        }
    }

    HdBufferArrayRangeSharedPtr const &bar = drawItem->GetVertexPrimvarRange();
    if (HdStCanSkipBARAllocationOrUpdate(sources, computations, bar,
        *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        TfTokenVector internallyGeneratedPrimvars; // none
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars,
            compPrimvars, internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdBufferSpec::GetBufferSpecs(reserveOnlySources, &bufferSpecs);
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);

    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsVertex);

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(),
        &sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));

    if (!sources.empty() || !computations.empty()) {
        // If sources or computations are to be queued against the resulting
        // BAR, we expect it to be valid.
        if (!TF_VERIFY(drawItem->GetVertexPrimvarRange()->IsValid())) {
            return;
        }
    }

    // add sources to update queue
    if (!sources.empty()) {
        resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
            std::move(sources));
    }
    // add gpu computations to queue.
    for (auto const &compQueuePair : computations) {
        HdStComputationSharedPtr const &comp = compQueuePair.first;
        HdStComputeQueue queue = compQueuePair.second;
        resourceRegistry->AddComputation(
            drawItem->GetVertexPrimvarRange(), comp, queue);
    }
    if (!separateComputationSources.empty()) {
        TF_FOR_ALL(it, separateComputationSources) {
            resourceRegistry->AddSource(*it);
        }
    }
}

void
HdStSimpleText::_UpdateRepr(HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    TfToken const &reprToken,
    HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSharedPtr const &curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    // Filter custom dirty bits to only those in use.
    *dirtyBits &= (_customDirtyBitsInUse |
        HdChangeTracker::AllSceneDirtyBits |
        HdChangeTracker::NewRepr);

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        TfDebug::Helper().Msg(
            "HdStSimpleText::_UpdateRepr for %s : Repr = %s\n",
            GetId().GetText(), reprToken.GetText());
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
        curRepr->GetDrawItem(0));

    if (HdChangeTracker::IsDirty(*dirtyBits)) {
        /* VISIBILITY */
        _UpdateVisibility(sceneDelegate, dirtyBits);
        _UpdateDrawItem(sceneDelegate, renderParam,
            drawItem, dirtyBits, _sharedData, 0);

        // Check if we will add underline/overline/strike through draw items.
        // First initialize the sharedData for the lines.
        TF_VERIFY(_topology);
        if (_topology->GetDecorationCount() != _sharedDataLines.size())
        {
            _sharedDataLines.clear();
            for (size_t index = 0; index < _topology->GetDecorationCount(); ++index)
            {
                HdRprimSharedData sharedDataLine(HdDrawingCoord::DefaultNumSlots,
                    /*visible=*/true);
                _sharedDataLines.push_back(sharedDataLine);
            }
        }

        // Then add line draw items.
        const HdRepr::DrawItemUniquePtrVector& drawItems = curRepr->GetDrawItems();
        if ((drawItems.size() - 1) != _sharedDataLines.size())
        {
            if (drawItems.size() != 1)
            {
                TF_CODING_ERROR("There should be only one draw item here.");
                return;
            }
            for (size_t index = 0; index < _topology->GetDecorationCount(); ++index)
            {
                // Add line drawItem.
                HdRepr::DrawItemUniquePtr lineDrawItem =
                    std::make_unique<HdStDrawItem>(&_sharedDataLines[index]);
                HdDrawingCoord* lineDrawingCoord = lineDrawItem->GetDrawingCoord();
                lineDrawingCoord->SetTopologyIndex(HdStSimpleText::LineTopology);
                lineDrawingCoord->SetVertexPrimvarIndex(HdStSimpleText::LinePointsTopology);
                // Set up drawing coord instance primvars.
                lineDrawingCoord->SetInstancePrimvarBaseIndex(
                    HdStSimpleText::LineInstancePrimvar);
                curRepr->AddDrawItem(std::move(lineDrawItem));
            }
        }
    }

    const HdRepr::DrawItemUniquePtrVector& drawItems = curRepr->GetDrawItems();
    for (size_t index = 1; index < drawItems.size(); ++index)
    {
        drawItem = static_cast<HdStDrawItem*>(drawItems[index].get());
        if (!TF_VERIFY(drawItem))
            continue;

        if (HdChangeTracker::IsDirty(*dirtyBits)) {
            _sharedDataLines[index - 1].visible = _sharedData.visible;
            _UpdateDrawItem(sceneDelegate, renderParam,
                drawItem, dirtyBits, _sharedDataLines[index - 1], index);
        }
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

void
HdStSimpleText::_UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    bool updateMaterialShader,
    bool updateGeometricShader)
{
    TF_DEBUG(HD_RPRIM_UPDATED).Msg(
        "(%s) - Updating geometric and material shaders for draw "
        "items of all reprs.\n", GetId().GetText());

    HdSt_MaterialNetworkShaderSharedPtr materialShader;
    if (updateMaterialShader) {
        materialShader = HdStGetMaterialNetworkShader(this, sceneDelegate);
    }

    for (auto const &reprPair : _reprs) {
        HdReprSharedPtr repr = reprPair.second;
        int drawItemIndex = 0;
        HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
            repr->GetDrawItem(drawItemIndex));

        if (updateMaterialShader) {
            drawItem->SetMaterialNetworkShader(materialShader);
        }
        if (updateGeometricShader) {
            if (!TF_VERIFY(_topology)) return;
            HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

            // Usd the resolution independent curve shader to render the text.
            HdSt_TextShaderKey shaderKey;

            HdStResourceRegistrySharedPtr resourceRegistry =
                std::static_pointer_cast<HdStResourceRegistry>(
                    renderIndex.GetResourceRegistry());

            HdSt_GeometricShaderSharedPtr geomShader =
                HdSt_GeometricShader::Create(shaderKey, resourceRegistry);

            TF_VERIFY(geomShader);

            if (geomShader != drawItem->GetGeometricShader())
            {
                drawItem->SetGeometricShader(geomShader);

                // If the gometric shader changes, we need to do a deep validation of
                // batches, so they can be rebuilt if necessary.
                HdStMarkDrawBatchesDirty(renderParam);

                TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                    "%s: Marking all batches dirty to trigger deep validation because"
                    " the geometric shader was updated.\n", GetId().GetText());
            }
        }

        // Update line drawItems.
        const HdRepr::DrawItemUniquePtrVector& drawItems = repr->GetDrawItems();
        HdSt_MaterialNetworkShaderSharedPtr fallbackMaterialShader;
        if (drawItems.size() > 1)
        {
            HdStMaterial const* material = static_cast<HdStMaterial const*>(
                sceneDelegate->GetRenderIndex().GetFallbackSprim(HdPrimTypeTokens->material));
            fallbackMaterialShader = material->GetMaterialNetworkShader();
        }
        for (size_t index = 1; index < drawItems.size(); ++index)
        {
            HdStDrawItem* lineDrawItem = static_cast<HdStDrawItem*>(
                repr->GetDrawItem(index));
            if (!TF_VERIFY(lineDrawItem))
                continue;

            if (updateMaterialShader) {
                lineDrawItem->SetMaterialNetworkShader(fallbackMaterialShader);
            }
            _UpdateCurveDrawItemGeometricShader(
                sceneDelegate, renderParam, lineDrawItem);
        }
    }
}

void
HdStSimpleText::_UpdateCurveDrawItemGeometricShader(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdStDrawItem *drawItem)
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    HdStResourceRegistrySharedPtr resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    TfToken curveType = HdTokens->linear;
    TfToken curveBasis = TfToken();
    HdSt_BasisCurvesShaderKey::DrawStyle drawStyle =
        HdSt_BasisCurvesShaderKey::WIRE;
    HdSt_BasisCurvesShaderKey::NormalStyle normalStyle =
        HdSt_BasisCurvesShaderKey::HAIR;

    bool hasAuthoredTopologicalVisiblity =
        (bool)drawItem->GetTopologyVisibilityRange();

    // Process shadingTerminal (including shadingStyle)
    TfToken shadingTerminal = HdBasisCurvesReprDescTokens->surfaceShader;
    TfToken shadingStyle =
        sceneDelegate->GetShadingStyle(GetId()).GetWithDefault<TfToken>();
    if (shadingStyle == HdStTokens->constantLighting) {
        shadingTerminal = HdBasisCurvesReprDescTokens->surfaceShaderUnlit;
    }

    bool _basisWidthInterpolation = true;
    bool _basisNormalInterpolation = true;
    bool _pointsShadingEnabled = false;
    bool _hasMetalTessellation = false;

    HdSt_BasisCurvesShaderKey shaderKey(curveType,
        curveBasis,
        drawStyle,
        normalStyle,
        _basisWidthInterpolation,
        _basisNormalInterpolation,
        shadingTerminal,
        hasAuthoredTopologicalVisiblity,
        _pointsShadingEnabled, 
        _hasMetalTessellation);

    HdSt_GeometricShaderSharedPtr geomShader =
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry);

    TF_VERIFY(geomShader);

    if (geomShader != drawItem->GetGeometricShader())
    {
        drawItem->SetGeometricShader(geomShader);

        // If the gometric shader changes, we need to do a deep validation of
        // batches, so they can be rebuilt if necessary.
        HdStMarkDrawBatchesDirty(renderParam);
    }
}

void
HdStSimpleText::Finalize(HdRenderParam *renderParam)
{
    HdStMarkGarbageCollectionNeeded(renderParam);

    HdStRenderParam* const stRenderParam =
        static_cast<HdStRenderParam*>(renderParam);

    // Decrement material tag counts for each draw item material tag
    for (auto const &reprPair : _reprs) {
        HdReprSharedPtr repr = reprPair.second;
        int drawItemIndex = 0;
        HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
            repr->GetDrawItem(drawItemIndex));
        stRenderParam->DecreaseMaterialTagCount(drawItem->GetMaterialTag());

        const HdRepr::DrawItemUniquePtrVector& drawItems = repr->GetDrawItems();
        for (size_t index = 1; index < drawItems.size(); ++index)
        {
            HdStDrawItem* lineDrawItem = static_cast<HdStDrawItem*>(
                repr->GetDrawItem(index));
            if (!TF_VERIFY(lineDrawItem))
                continue;

            stRenderParam->DecreaseMaterialTagCount(lineDrawItem->GetMaterialTag());
        }
    }
}


HdDirtyBits
HdStSimpleText::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // propagate scene-based dirtyBits into rprim-custom dirtyBits
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse & DirtyIndices;
    }

    return bits;
}

void
HdStSimpleText::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
        _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        _reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
        HdReprSharedPtr &repr = _reprs.back().second;

        *dirtyBits |= HdChangeTracker::NewRepr;
        _customDirtyBitsInUse |= DirtyIndices;
        *dirtyBits |= DirtyIndices;

        HdRepr::DrawItemUniquePtr drawItem =
            std::make_unique<HdStDrawItem>(&_sharedData);
        HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
        drawingCoord->SetTopologyIndex(HdStSimpleText::Topology);
        // Set up drawing coord instance primvars.
        drawingCoord->SetInstancePrimvarBaseIndex(
            HdStSimpleText::InstancePrimvar);
        repr->AddDrawItem(std::move(drawItem));
    }
}


HdDirtyBits
HdStSimpleText::GetInitialDirtyBitsMask() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyWidths
        | HdChangeTracker::DirtyComputationPrimvarDesc
        | HdChangeTracker::DirtyInstancer
        ;

    return mask;
}


/*override*/
TfTokenVector const&
HdStSimpleText::GetBuiltinPrimvarNames() const
{
    auto _ComputePrimvarNames = [this]() {
        TfTokenVector primvarNames =
            this->HdSimpleText::GetBuiltinPrimvarNames();
        return primvarNames;
    };
    static TfTokenVector primvarNames = _ComputePrimvarNames();
    return primvarNames;
}

PXR_NAMESPACE_CLOSE_SCOPE
