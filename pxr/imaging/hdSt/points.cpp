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

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/points.h"
#include "pxr/imaging/hdSt/pointsShaderKey.h"
#include "pxr/imaging/hdSt/primUtils.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStPoints::HdStPoints(SdfPath const& id)
  : HdPoints(id)
  , _displayOpacity(false)
{
    /*NOTHING*/
}

HdStPoints::~HdStPoints() = default;

void
HdStPoints::Sync(HdSceneDelegate *delegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits,
                 TfToken const   &reprToken)
{
    bool updateMaterialTag = false;
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        HdStSetMaterialId(delegate, renderParam, this);
        updateMaterialTag = true;
    }

    bool displayOpacity = _displayOpacity;
    _UpdateRepr(delegate, renderParam, reprToken, dirtyBits);

    if (updateMaterialTag || 
        (GetMaterialId().IsEmpty() && displayOpacity != _displayOpacity)) {

        HdStSetMaterialTag(delegate, renderParam, this, _displayOpacity,
                           /*occludedSelectionShowsThrough = */false);
    }

    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdStPoints::Finalize(HdRenderParam *renderParam)
{
    HdStMarkGarbageCollectionNeeded(renderParam);
}

void
HdStPoints::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                            HdRenderParam *renderParam,
                            HdStDrawItem *drawItem,
                            HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    /* VISIBILITY */
    _UpdateVisibility(sceneDelegate, dirtyBits);

    /* MATERIAL SHADER (may affect subsequent primvar population) */
    drawItem->SetMaterialShader(HdStGetMaterialShader(this, sceneDelegate));

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
                                &_sharedData,
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
                                     &_sharedData,
                                     sceneDelegate,
                                     renderParam,
                                     drawItem,
                                     dirtyBits,
                                     constantPrimvars);
        
        _displayOpacity = _displayOpacity ||
            HdStIsPrimvarExistentAndValid(this, sceneDelegate, 
            constantPrimvars, HdTokens->displayOpacity);
    }

    HdSt_PointsShaderKey shaderKey;
    HdStResourceRegistrySharedPtr resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());
    drawItem->SetGeometricShader(
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry));

    /* PRIMVAR */
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _PopulateVertexPrimvars(
            sceneDelegate, renderParam, drawItem, dirtyBits);
    }

    // VertexPrimvar may be null, if there are no points in the prim.

    TF_VERIFY(drawItem->GetConstantPrimvarRange());
}

void
HdStPoints::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                        HdRenderParam *renderParam,
                        TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX: We only support smoothHull for now
    _PointsReprConfig::DescArray descs = _GetReprDesc(HdReprTokens->smoothHull);
    HdReprSharedPtr const &curRepr = _smoothHullRepr;

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        TfDebug::Helper().Msg(
            "HdStPoints::_UpdateRepr for %s : Repr = %s\n",
            GetId().GetText(), reprToken.GetText());
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
        const HdPointsReprDesc &desc = descs[descIdx];

        if (desc.geomStyle != HdPointsGeomStyleInvalid) {
            HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                curRepr->GetDrawItem(drawItemIndex++));
            if (HdChangeTracker::IsDirty(*dirtyBits)) {
                _UpdateDrawItem(
                    sceneDelegate, renderParam, drawItem, dirtyBits);
            }
        }
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

void
HdStPoints::_PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                    HdRenderParam *renderParam,
                                    HdStDrawItem *drawItem,
                                    HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // Gather vertex and varying primvars
    HdPrimvarDescriptorVector primvars;
    {
        primvars = HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                                    HdInterpolationVertex);

        HdPrimvarDescriptorVector varyingPvs =
            HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                                    HdInterpolationVarying);
        primvars.insert(primvars.end(), varyingPvs.begin(), varyingPvs.end());
    } 

    // Get computed vertex primvars
    HdExtComputationPrimvarDescriptorVector compPrimvars =
        sceneDelegate->GetExtComputationPrimvarDescriptors(id,
            HdInterpolationVertex);

    HdBufferSourceSharedPtrVector sources;
    HdBufferSourceSharedPtrVector reserveOnlySources;
    HdBufferSourceSharedPtrVector separateComputationSources;
    HdStComputationSharedPtrVector computations;
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

    for (HdPrimvarDescriptor const& primvar: primvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name)) {
            continue;
        }

        VtValue value = GetPrimvar(sceneDelegate, primvar.name);

        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source(
                new HdVtBufferSource(primvar.name, value));
            sources.push_back(source);

            if (primvar.name == HdTokens->displayOpacity) {
                _displayOpacity = true;
            }
        }
    }

    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetVertexPrimvarRange();
    
    if (HdStCanSkipBARAllocationOrUpdate(
            sources, computations, bar, *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        TfTokenVector internallyGeneratedPrimvars; // none
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars, 
            internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdBufferSpec::GetBufferSpecs(reserveOnlySources, &bufferSpecs);
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);
    
    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHint());

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
    for (auto const& compQueuePair : computations) {
        HdComputationSharedPtr const& comp = compQueuePair.first;
        HdStComputeQueue queue = compQueuePair.second;
        resourceRegistry->AddComputation(
            drawItem->GetVertexPrimvarRange(), comp, queue);
    }
    if (!separateComputationSources.empty()) {
        for(HdBufferSourceSharedPtr const& compSrc : 
                separateComputationSources) {
            resourceRegistry->AddSource(compSrc);
        }
    }
}

HdDirtyBits 
HdStPoints::GetInitialDirtyBitsMask() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyWidths
        | HdChangeTracker::DirtyInstancer
        ;

    return mask;
}

HdDirtyBits
HdStPoints::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdStPoints::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    // We only support smoothHull for now, everything else points to it.
    // TODO: Handle other styles
    if (!_smoothHullRepr) {
        _smoothHullRepr = HdReprSharedPtr(new HdRepr());
        *dirtyBits |= HdChangeTracker::NewRepr;

        _PointsReprConfig::DescArray const &descs = _GetReprDesc(reprToken);
        // allocate all draw items
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdPointsReprDesc &desc = descs[descIdx];

            if (desc.geomStyle != HdPointsGeomStyleInvalid) {
                HdRepr::DrawItemUniquePtr drawItem =
                    std::make_unique<HdStDrawItem>(&_sharedData);
                HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
                _smoothHullRepr->AddDrawItem(std::move(drawItem));

                // Set up drawing coord instance primvars.
                drawingCoord->SetInstancePrimvarBaseIndex(
                    HdStPoints::InstancePrimvar);
            }
        }
    }
     
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        it = _reprs.insert(_reprs.end(),
                std::make_pair(reprToken, _smoothHullRepr));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

