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
#include "pxr/imaging/hdSt/volume.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/rprimUtils.h"
#include "pxr/imaging/hdSt/volumeShaderKey.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStVolume::HdStVolume(SdfPath const& id, SdfPath const & instancerId)
    : HdVolume(id)
{
}

HdStVolume::~HdStVolume() = default;

HdDirtyBits 
HdStVolume::GetInitialDirtyBitsMask() const
{
    const int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyInstanceIndex
        ;

    return (HdDirtyBits)mask;
}

HdDirtyBits 
HdStVolume::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void 
HdStVolume::_InitRepr(TfToken const &reprToken, HdDirtyBits* dirtyBits)
{
    // All representations point to _volumeRepr.
    if (!_volumeRepr) {
        _volumeRepr = HdReprSharedPtr(new HdRepr());
        HdDrawItem * const drawItem = new HdStDrawItem(&_sharedData);
        _volumeRepr->AddDrawItem(drawItem);
        *dirtyBits |= HdChangeTracker::NewRepr;
    }
    
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        it = _reprs.insert(_reprs.end(),
                std::make_pair(reprToken, _volumeRepr));
    }
}

void
HdStVolume::Sync(HdSceneDelegate *delegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits,
                 TfToken const   &reprToken)
{
    TF_UNUSED(renderParam);

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(),
                       delegate->GetMaterialId(GetId()));

        _sharedData.materialTag = _GetMaterialTag(delegate->GetRenderIndex());
    }

    _UpdateRepr(delegate, reprToken, dirtyBits);

    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

const TfToken&
HdStVolume::_GetMaterialTag(const HdRenderIndex &renderIndex) const
{
    // Returning "translucent" to indicate that volumes use OIT
    // (order independent translucency).
    //
    // HdxMaterialTagTokens->translucent is defined in hdx, so we just
    // explicitly say "translucent" here to avoid dependency problems.
    //
    // Note that this is only a temporary solution as we might designate
    // a separate render task for volumes all together so volumes will have
    // their own material tag.

    static TfToken result("translucent");

    return result;
}

void
HdStVolume::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                        TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSharedPtr const &curRepr = _volumeRepr;

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    HdStDrawItem * const drawItem = static_cast<HdStDrawItem*>(
        curRepr->GetDrawItem(0));

    if (HdChangeTracker::IsDirty(*dirtyBits)) {
        _UpdateDrawItem(sceneDelegate, drawItem, dirtyBits);
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

static
const VtValue &
_GetCubeVertices()
{
    static const VtValue result(
        VtVec3fArray{
                GfVec3f(0, 0, 0),
                GfVec3f(0, 0, 1),
                GfVec3f(0, 1, 0),
                GfVec3f(0, 1, 1),
                GfVec3f(1, 0, 0),
                GfVec3f(1, 0, 1),
                GfVec3f(1, 1, 0),
                GfVec3f(1, 1, 1)});
    
    return result;
}

static
const VtValue &
_GetCubeTriangleIndices()
{
    static const VtValue result(
        VtVec3iArray{
                GfVec3i(2,3,1),
                GfVec3i(2,1,0),
                    
                GfVec3i(4,5,7),
                GfVec3i(4,7,6),

                GfVec3i(0,1,5),
                GfVec3i(0,5,4),
        
                GfVec3i(6,7,3),
                GfVec3i(6,3,2),

                GfVec3i(4,6,2),
                GfVec3i(4,2,0),

                GfVec3i(1,3,7),
                GfVec3i(1,7,5)});
    
    return result;
}

void
HdStVolume::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                            HdStDrawItem *drawItem,
                            HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    /* VISIBILITY */
    _UpdateVisibility(sceneDelegate, dirtyBits);

    /* CONSTANT PRIMVARS, TRANSFORM AND EXTENT */
    HdPrimvarDescriptorVector constantPrimvars =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationConstant);
    HdStPopulateConstantPrimvars(this, &_sharedData, sceneDelegate, drawItem, 
        dirtyBits, constantPrimvars);

    /* MATERIAL SHADER */
    drawItem->SetMaterialShaderFromRenderIndex(
        sceneDelegate->GetRenderIndex(), GetMaterialId());

    HdSt_VolumeShaderKey shaderKey;
    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());
    drawItem->SetGeometricShader(
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry));

    /* VERTICES */
    {
        // XXX:
        // Always the same vertices, should they be allocated only
        // once and shared across all volumes?
        HdBufferSourceSharedPtr source(
            new HdVtBufferSource(HdTokens->points, _GetCubeVertices()));

        HdBufferSourceVector sources = { source };

        if (!drawItem->GetVertexPrimvarRange() ||
            !drawItem->GetVertexPrimvarRange()->IsValid()) {
            HdBufferSpecVector bufferSpecs;
            HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
            
            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
            _sharedData.barContainer.Set(
                drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(), range);
        }
        
        resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
                                     sources);
    }

    /* TRIANGLE INDICES */
    {
        // XXX:
        // Always the same triangle indices, should they be allocated only
        // once and shared across all volumes?
        HdBufferSourceSharedPtr source(
            new HdVtBufferSource(HdTokens->indices, _GetCubeTriangleIndices()));

        HdBufferSourceVector sources = { source };

        if (!drawItem->GetTopologyRange() ||
            !drawItem->GetTopologyRange()->IsValid()) {
            HdBufferSpecVector bufferSpecs;
            HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
            
            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
            _sharedData.barContainer.Set(
                drawItem->GetDrawingCoord()->GetTopologyIndex(), range);
        }
        
        resourceRegistry->AddSources(drawItem->GetTopologyRange(),
                                     sources);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
