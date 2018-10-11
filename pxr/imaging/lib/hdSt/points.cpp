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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/points.h"
#include "pxr/imaging/hdSt/pointsShaderKey.h"
#include "pxr/imaging/hdSt/instancer.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStPoints::HdStPoints(SdfPath const& id,
                       SdfPath const& instancerId)
  : HdPoints(id, instancerId)
{
    /*NOTHING*/
}

HdStPoints::~HdStPoints()
{
    /*NOTHING*/
}

void
HdStPoints::Sync(HdSceneDelegate      *delegate,
                 HdRenderParam        *renderParam,
                 HdDirtyBits          *dirtyBits,
                 HdReprSelector const &reprSelector,
                 bool                  forcedRepr)
{
    TF_UNUSED(renderParam);

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(),
                       delegate->GetMaterialId(GetId()));
    }

    HdReprSelector calcReprSelector = _GetReprSelector(
            reprSelector, forcedRepr);
    _UpdateRepr(delegate, calcReprSelector, dirtyBits);

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdStPoints::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                            HdStDrawItem *drawItem,
                            HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    /* VISIBILITY */
    _UpdateVisibility(sceneDelegate, dirtyBits);

    /* CONSTANT PRIMVARS, TRANSFORM AND EXTENT */
    HdPrimvarDescriptorVector constantPrimvars =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationConstant);
    _PopulateConstantPrimvars(sceneDelegate, drawItem, dirtyBits,
            constantPrimvars);

    /* MATERIAL SHADER */
    drawItem->SetMaterialShaderFromRenderIndex(
        sceneDelegate->GetRenderIndex(), GetMaterialId());

    /* INSTANCE PRIMVARS */
    if (!GetInstancerId().IsEmpty()) {
        HdStInstancer *instancer = static_cast<HdStInstancer*>(
            sceneDelegate->GetRenderIndex().GetInstancer(GetInstancerId()));
        if (TF_VERIFY(instancer)) {
            instancer->PopulateDrawItem(drawItem, &_sharedData,
                dirtyBits, InstancePrimvar);
        }
    }

    HdSt_PointsShaderKey shaderKey;
    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());
    drawItem->SetGeometricShader(
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry));

    /* PRIMVAR */
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _PopulateVertexPrimvars(sceneDelegate, drawItem, dirtyBits);
    }

    // VertexPrimvar may be null, if there are no points in the prim.

    TF_VERIFY(drawItem->GetConstantPrimvarRange());
}

void
HdStPoints::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                        HdReprSelector const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX: We only support smoothHull for now
    _PointsReprConfig::DescArray descs = _GetReprDesc(
            HdReprSelector(HdReprTokens->smoothHull));
    HdReprSharedPtr const &curRepr = _smoothHullRepr;

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        std::cout << "HdStPoints::_UpdateRepr " << GetId()
                  << " Repr = " << HdReprTokens->smoothHull << "\n";
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
        const HdPointsReprDesc &desc = descs[descIdx];

        if (desc.geomStyle != HdPointsGeomStyleInvalid) {
            HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                curRepr->GetDrawItem(drawItemIndex++));
            if (HdChangeTracker::IsDirty(*dirtyBits)) {
                _UpdateDrawItem(sceneDelegate, drawItem, dirtyBits);
            }
        }
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

void
HdStPoints::_PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                    HdStDrawItem *drawItem,
                                    HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // The "points" attribute is expected to be in this list.
    HdPrimvarDescriptorVector primvars =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationVertex);

    // Add varying primvars so we can process them together, below.
    HdPrimvarDescriptorVector varyingPvs =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationVarying);
    primvars.insert(primvars.end(), varyingPvs.begin(), varyingPvs.end());

    HdBufferSourceVector sources;
    sources.reserve(primvars.size());

    int pointsIndexInSourceArray = -1;

    for (HdPrimvarDescriptor const& primvar: primvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name))
            continue;

        // TODO: We don't need to pull primvar metadata every time a value
        // changes, but we need support from the delegate.

        //assert name not in range.bufferArray.GetResources()
        VtValue value = GetPrimvar(sceneDelegate, primvar.name);

        if (!value.IsEmpty()) {
            // Store where the points will be stored in the source array
            // we need this later to figure out if the number of points is
            // changing and we need to force a garbage collection to resize
            // the buffer
            if (primvar.name == HdTokens->points) {
                pointsIndexInSourceArray = sources.size();
            }

            // XXX: do we need special treatment for width as basicCurves?

            HdBufferSourceSharedPtr source(
                new HdVtBufferSource(primvar.name, value));
            sources.push_back(source);
        }
    }

    // return before allocation if it's empty.
    if (sources.empty())
        return;

    if (!drawItem->GetVertexPrimvarRange() ||
        !drawItem->GetVertexPrimvarRange()->IsValid()) {
        // initialize buffer array
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(), range);

    } else if (pointsIndexInSourceArray >=0) {

        int previousRange = drawItem->GetVertexPrimvarRange()->GetNumElements();
        int newRange = sources[pointsIndexInSourceArray]->GetNumElements();

        // Check if the range is different and if so force a garbage collection
        // which will make sure the points are up to date; also mark batches
        // dirty.
        if(previousRange != newRange) {
            sceneDelegate->GetRenderIndex().GetChangeTracker().
                SetGarbageCollectionNeeded();
            sceneDelegate->GetRenderIndex().GetChangeTracker().
                MarkBatchesDirty();
        }
    }

    // add sources to update queue
    resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
                                 sources);
}

HdDirtyBits 
HdStPoints::GetInitialDirtyBitsMask() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyInstanceIndex
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyWidths
        ;

    return mask;
}

HdDirtyBits
HdStPoints::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdStPoints::_InitRepr(HdReprSelector const &reprToken,
                      HdDirtyBits *dirtyBits)
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
                HdDrawItem *drawItem = new HdStDrawItem(&_sharedData);
                _smoothHullRepr->AddDrawItem(drawItem);
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

