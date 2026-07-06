//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"

#include "pxr/imaging/hdSt/sphere.h"
#include "pxr/imaging/hdSt/sphereShaderKey.h"

#include "pxr/imaging/hdSt/primUtils.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStSphere::HdStSphere(SdfPath const& id)
  : HdRprim(id)
  , _cullStyle(HdCullStyleDontCare)
  , _vertexCount(4)
  , _doubleSided(false)
  , _displayOpacityFromInstancer(false)
  , _displayOpacityFromPrimvars(false)
  , _displayInOverlay(false)
{
}

HdStSphere::~HdStSphere() = default;

TfTokenVector const &
HdStSphere::GetBuiltinPrimvarNames() const
{
    static const TfTokenVector primvarNames = {
        HdSphereSchemaTokens->radius
    };

    return primvarNames;
}

void
HdStSphere::UpdateRenderTag(HdSceneDelegate *delegate,
                            HdRenderParam *renderParam)
{
    HdStUpdateRenderTag(delegate, renderParam, this);
}

void
HdStSphere::Sync(HdSceneDelegate *delegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits,
                 TfToken const   &reprToken)
{
    _UpdateVisibility(delegate, dirtyBits);

    bool updateMaterialTags = false;
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        HdStSetMaterialId(delegate, renderParam, this);
        updateMaterialTags = true;
    }
    if (*dirtyBits & (HdChangeTracker::DirtyDisplayStyle|
                      HdChangeTracker::NewRepr)) {
        updateMaterialTags = true;
    }

    // Check if either the material or geometric shaders need updating for
    // draw items of all the reprs.
    bool updateMaterialNetworkShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyMaterialId|
                      HdChangeTracker::NewRepr)) {
        updateMaterialNetworkShader = true;
    }

    bool updateGeometricShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyCullStyle|
                      HdChangeTracker::DirtyDoubleSided|
                      HdChangeTracker::DirtyInstancer|
                      HdChangeTracker::NewRepr)) {
        updateGeometricShader = true;
    }

    bool displayOpacity =
        (_displayOpacityFromInstancer || _displayOpacityFromPrimvars);
    _UpdateRepr(delegate, renderParam, reprToken, dirtyBits);

    if (updateMaterialTags || 
        (GetMaterialId().IsEmpty() &&
         (displayOpacity !=
          (_displayOpacityFromInstancer || _displayOpacityFromPrimvars)))) {
        _UpdateMaterialTagsForAllReprs(delegate, renderParam);
    }

    if (updateMaterialNetworkShader || updateGeometricShader) {
        _UpdateShadersForAllReprs(delegate,
                                  renderParam,
                                  updateMaterialNetworkShader,
                                  updateGeometricShader);
    }

    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdStSphere::Finalize(HdRenderParam *renderParam)
{
    HdStMarkGarbageCollectionNeeded(renderParam);

    HdStRenderParam * const stRenderParam =
        static_cast<HdStRenderParam*>(renderParam);

    // Decrement material tag counts for each draw item material tag
    if (!_reprs.empty()) {
        int drawItemIndex = 0;
        HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
            _sphereDefaultRepr->GetDrawItem(drawItemIndex));
        stRenderParam->DecreaseMaterialTagCount(drawItem->GetMaterialTag());
    }

    stRenderParam->DecreaseRenderTagCount(GetRenderTag());
}

void
HdStSphere::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                        HdRenderParam *renderParam,
                        TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSharedPtr const &curRepr = _GetRepr(reprToken);
    if (!curRepr) return;

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        TfDebug::Helper().Msg(
            "HdStSphere::_UpdateRepr for %s : Repr = %s\n",
            GetId().GetText(), reprToken.GetText());
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    int drawItemIndex = 0;

    HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
        curRepr->GetDrawItem(drawItemIndex));

    if (HdChangeTracker::IsDirty(*dirtyBits)) {
        _UpdateDrawItem(sceneDelegate, renderParam,
                        drawItem, dirtyBits);
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

void
HdStSphere::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                            HdRenderParam *renderParam,
                            HdStDrawItem *drawItem,
                            HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    /* MATERIAL SHADER (may affect subsequent primvar population) */
    drawItem->SetMaterialNetworkShader(
        HdStGetMaterialNetworkShader(this, sceneDelegate));

    // Reset value of _displayOpacity
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
        HdTokens->displayOpacity)) {
        _displayOpacityFromPrimvars = false;
    }

    if (*dirtyBits & HdChangeTracker::DirtyDoubleSided) {
        _doubleSided = sceneDelegate->GetDoubleSided(id);
    }
    if (*dirtyBits & HdChangeTracker::DirtyCullStyle) {
        _cullStyle = sceneDelegate->GetCullStyle(id);
    }

    HdDisplayStyle const displayStyle = sceneDelegate->GetDisplayStyle(id);

    _displayInOverlay = displayStyle.displayInOverlay;
    _occludedSelectionShowsThrough =
        displayStyle.occludedSelectionShowsThrough;

    /* INSTANCE PRIMVARS */
    _UpdateInstancer(sceneDelegate, dirtyBits);
    {
        // The data members are part of a bitfield, so we can't pass pointers
        // to them directly. HdStUpdateInstancerData doesn't write to output
        // params if DirtyInstancer is not set, so we initialize the locals
        // to current member values to preserve existing state in that case.
        bool displayOpacityFromInstancer = _displayOpacityFromInstancer;
        HdStUpdateInstancerData(sceneDelegate->GetRenderIndex(),
                                renderParam,
                                this,
                                drawItem,
                                &_sharedData,
                                *dirtyBits,
                                &displayOpacityFromInstancer);
        _displayOpacityFromInstancer = displayOpacityFromInstancer;
    }

    static const HdBufferSpecVector customSpecs{
        HdBufferSpec(HdSphereSchemaTokens->radius, HdTupleType{HdTypeFloat, 1})
    };

    /* CONSTANT PRIMVARS, TRANSFORM, EXTENT AND PRIMID */
    if (HdStShouldPopulateConstantPrimvars(dirtyBits, id)) {
        bool hasDisplayOpacity = false;
        HdStPopulateConstantPrimvars(this,
                                     &_sharedData,
                                     sceneDelegate,
                                     renderParam,
                                     drawItem,
                                     dirtyBits,
                                     _sphereDefaultRepr, HdMeshGeomStyleInvalid,
                                     0, 0,
                                     customSpecs,
                                     nullptr, // hasMirroredTransform
                                     &hasDisplayOpacity);

        if (hasDisplayOpacity) {
            _displayOpacityFromPrimvars = true;
        }
    }

    if(HdChangeTracker::IsPrimvarDirty(
        *dirtyBits, id, HdSphereSchemaTokens->radius)) {
        _PopulateCustomConstantPrimvars(sceneDelegate, renderParam, drawItem);
    }
    
    if(*dirtyBits & HdChangeTracker::NewRepr) {
        _PopulateIndexBuffer(sceneDelegate, renderParam, drawItem);
    }

    TF_VERIFY(drawItem->GetConstantPrimvarRange());
}

void
HdStSphere::_UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                           HdRenderParam *renderParam,
                                           HdStDrawItem *drawItem)
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    HdStResourceRegistrySharedPtr resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    HdSt_SphereShaderKey shaderKey{_cullStyle, _doubleSided, _vertexCount};

    HdSt_GeometricShaderSharedPtr geomShader = HdSt_GeometricShader::Create(
                                                shaderKey, resourceRegistry);
    
    if (geomShader != drawItem->GetGeometricShader()) {
        drawItem->SetGeometricShader(geomShader);

        // If the geometric shader changes, we need to do a deep validation of
        // batches, so they can be rebuilt if necessary.
        HdStMarkDrawBatchesDirty(renderParam);

        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking all batches dirty to trigger deep validation because"
            " the geometric shader was updated.\n", GetId().GetText());
    }
}

void
HdStSphere::_UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                    HdRenderParam *renderParam,
                                    bool updateMaterialNetworkShader,
                                    bool updateGeometricShader)
{
    TF_DEBUG(HD_RPRIM_UPDATED). Msg(
        "(%s) - Updating geometric and material shaders for draw "
        "items of all reprs.\n", GetId().GetText());

    HdSt_MaterialNetworkShaderSharedPtr materialNetworkShader;

    const bool materialIsFinal = 
                        sceneDelegate->GetDisplayStyle(GetId()).materialIsFinal;
    bool materialIsFinalChanged = false;

    if (!_reprs.empty()) {
        int drawItemIndex = 0;
        HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
            _sphereDefaultRepr->GetDrawItem(drawItemIndex));

        if (materialIsFinal != drawItem->GetMaterialIsFinal()) {
            materialIsFinalChanged = true;
        }
        drawItem->SetMaterialIsFinal(materialIsFinal);

        if (updateMaterialNetworkShader) {
            materialNetworkShader =
                HdStGetMaterialNetworkShader(this, sceneDelegate);
            drawItem->SetMaterialNetworkShader(materialNetworkShader);
        }

        if (updateGeometricShader) {
            _UpdateDrawItemGeometricShader(sceneDelegate, renderParam, drawItem);
        }
    }

    if (materialIsFinalChanged) {
        HdStMarkDrawBatchesDirty(renderParam);
        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking all batches dirty to trigger deep validation because "
            "the materialIsFinal was updated.\n", GetId().GetText());
    }
}

void
HdStSphere::_UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                           HdRenderParam *renderParam)
{
    TF_DEBUG(HD_RPRIM_UPDATED). Msg(
        "(%s) - Updating material tags for draw items of all reprs.\n", 
        GetId().GetText());

    // All reprs in _reprs point to same repr for now
    // since we haven't implemented other reprs (e.g., wireframes) yet
    if (!_reprs.empty()) {
        int drawItemIndex = 0;
        HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
            _sphereDefaultRepr->GetDrawItem(drawItemIndex));

        HdStSetMaterialTag(sceneDelegate, renderParam, drawItem, 
            GetMaterialId(),
            _displayOpacityFromPrimvars || _displayOpacityFromInstancer, 
            _displayInOverlay,
            _occludedSelectionShowsThrough);
    }
}

void
HdStSphere::_PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                            HdRenderParam *renderParam,
                                            HdStDrawItem *drawItem)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistrySharedPtr resourceRegistry =
    std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdBufferArrayRangeSharedPtr const& bar = drawItem->GetConstantPrimvarRange();

    float radius = sceneDelegate->Get(GetId(),
                    HdSphereSchemaTokens->radius).Get<double>();

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdSphereSchemaTokens->radius, VtValue(radius))
    };

    resourceRegistry->AddSources(bar, std::move(sources));
}

void
HdStSphere::_PopulateIndexBuffer(HdSceneDelegate *sceneDelegate,
                                     HdRenderParam *renderParam,
                                     HdStDrawItem *drawItem)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdStResourceRegistrySharedPtr resourceRegistry =
    std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());
        
    HdInstance<HdBufferArrayRangeSharedPtr> rangeInstance =
        resourceRegistry->RegisterImplicitPrimsIndexRange(1, HdTokens->indices);

    if(rangeInstance.IsFirstInstance()) {
        HdBufferSpecVector bufferSpecs;

        static const VtVec3iArray quadIndices{
            GfVec3i(0, 1, 2), GfVec3i(0, 2, 3)};

        HdBufferSourceSharedPtrVector sources = {
            std::make_shared<HdVtBufferSource>(
                HdTokens->indices, VtValue(quadIndices))
        };

        HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

        HdBufferArrayUsageHint usageHint =
            HdBufferArrayUsageHintBitsIndex |
            HdBufferArrayUsageHintBitsStorage;

        // allocate new range
        HdBufferArrayRangeSharedPtr range
            = resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs, usageHint);

        // add sources to update queue
        resourceRegistry->AddSources(range, std::move(sources));
        rangeInstance.SetValue(range);
    }

    HdBufferArrayRangeSharedPtr const& newRange = rangeInstance.GetValue();

    HdStUpdateDrawItemBAR(
        newRange,
        drawItem->GetDrawingCoord()->GetTopologyIndex(),
        &_sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));
}

HdDirtyBits 
HdStSphere::GetInitialDirtyBitsMask() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyInstancer
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyCullStyle
        ;

    return mask;
}

void
HdStSphere::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    if (!_sphereDefaultRepr) {
        _sphereDefaultRepr = std::make_shared<HdRepr>();
        *dirtyBits |= HdChangeTracker::NewRepr;

        HdRepr::DrawItemUniquePtr drawItem =
            std::make_unique<HdStDrawItem>(&_sharedData);
        HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
        _sphereDefaultRepr->AddDrawItem(std::move(drawItem));

        // Set up drawing coord instance primvars.
        drawingCoord->SetInstancePrimvarBaseIndex(
            HdStSphere::InstancePrimvar);
    }
     
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        it = _reprs.insert(_reprs.end(),
                std::make_pair(reprToken, _sphereDefaultRepr));
    }
}

HdDirtyBits
HdStSphere::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

PXR_NAMESPACE_CLOSE_SCOPE

