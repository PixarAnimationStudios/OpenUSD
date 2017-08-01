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

#include "pxr/imaging/hdSt/points.h"
#include "pxr/imaging/hdSt/pointsShaderKey.h"
#include "pxr/imaging/hdSt/instancer.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
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
HdStPoints::Sync(HdSceneDelegate* delegate,
                 HdRenderParam*   renderParam,
                 HdDirtyBits*     dirtyBits,
                 TfToken const&   reprName,
                 bool             forcedRepr)
{
    TF_UNUSED(renderParam);

    HdRprim::_Sync(delegate,
                  reprName,
                  forcedRepr,
                  dirtyBits);

    TfToken calcReprName = _GetReprName(delegate, reprName,
                                        forcedRepr, dirtyBits);
    _GetRepr(delegate, calcReprName, dirtyBits);

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdStPoints::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                            HdDrawItem *drawItem,
                            HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    /* VISIBILITY */
    _UpdateVisibility(sceneDelegate, dirtyBits);

    /* CONSTANT PRIMVARS, TRANSFORM AND EXTENT */
    _PopulateConstantPrimVars(sceneDelegate, drawItem, dirtyBits);

    /* INSTANCE PRIMVARS */
    if (!GetInstancerId().IsEmpty()) {
        HdStInstancer *instancer = static_cast<HdStInstancer*>(
            sceneDelegate->GetRenderIndex().GetInstancer(GetInstancerId()));
        if (TF_VERIFY(instancer)) {
            instancer->PopulateDrawItem(drawItem, &_sharedData,
                dirtyBits, InstancePrimVar);
        }
    }

    HdSt_PointsShaderKey shaderKey;
    drawItem->SetGeometricShader(
        Hd_GeometricShader::Create(
            shaderKey, 
            sceneDelegate->GetRenderIndex().GetResourceRegistry()));

    /* PRIMVAR */
    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        _PopulateVertexPrimVars(sceneDelegate, drawItem, dirtyBits);
    }

    // VertexPrimVar may be null, if there are no points in the prim.

    TF_VERIFY(drawItem->GetConstantPrimVarRange());
}

HdReprSharedPtr const &
HdStPoints::_GetRepr(HdSceneDelegate *sceneDelegate,
                     TfToken const &reprName,
                     HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _PointsReprConfig::DescArray descs = _GetReprDesc(reprName);

    _ReprVector::iterator it = _reprs.begin();
    if (_reprs.empty()) {
        // Hydra should have called _InitRepr earlier in sync when
        // before sending dirty bits to the delegate.
        TF_CODING_ERROR("_InitRepr() should be called for repr %s.",
                        reprName.GetText());

        static const HdReprSharedPtr ERROR_RETURN;

        return ERROR_RETURN;
    }

    // points don't have multiple draw items (for now)
    if (HdChangeTracker::IsDirty(*dirtyBits)) {
        if (descs[0].geomStyle != HdPointsGeomStyleInvalid) {
            _UpdateDrawItem(sceneDelegate,
                            _reprs[0].second->GetDrawItem(0),
                            dirtyBits);
        }

        *dirtyBits &= ~DirtyNewRepr;
    }

    return it->second;
}

void
HdStPoints::_PopulateVertexPrimVars(HdSceneDelegate *sceneDelegate,
                                    HdDrawItem *drawItem,
                                    HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdResourceRegistrySharedPtr const &resourceRegistry = 
        sceneDelegate->GetRenderIndex().GetResourceRegistry();

    // The "points" attribute is expected to be in this list.
    TfTokenVector primVarNames = GetPrimVarVertexNames(sceneDelegate);
    TfTokenVector const& vars = GetPrimVarVaryingNames(sceneDelegate);
    primVarNames.insert(primVarNames.end(), vars.begin(), vars.end());

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());

    int pointsIndexInSourceArray = -1;

    TF_FOR_ALL(nameIt, primVarNames) {
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt))
            continue;

        // TODO: We don't need to pull primvar metadata every time a value
        // changes, but we need support from the delegate.

        //assert name not in range.bufferArray.GetResources()
        VtValue value = GetPrimVar(sceneDelegate, *nameIt);

        if (!value.IsEmpty()) {
            // Store where the points will be stored in the source array
            // we need this later to figure out if the number of points is changing
            // and we need to force a garbage collection to resize the buffer
            if (*nameIt == HdTokens->points) {
                pointsIndexInSourceArray = sources.size();
            }

            // XXX: do we need special treatment for width as basicCurves?

            HdBufferSourceSharedPtr source(new HdVtBufferSource(*nameIt, value));
            sources.push_back(source);
        }
    }

    // return before allocation if it's empty.
    if (sources.empty())
        return;

    if (!drawItem->GetVertexPrimVarRange() ||
        !drawItem->GetVertexPrimVarRange()->IsValid()) {
        // initialize buffer array
        HdBufferSpecVector bufferSpecs;
        TF_FOR_ALL(it, sources) {
            (*it)->AddBufferSpecs(&bufferSpecs);
        }

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(), range);

    } else if (pointsIndexInSourceArray >=0) {

        int previousRange = drawItem->GetVertexPrimVarRange()->GetNumElements();
        int newRange = sources[pointsIndexInSourceArray]->GetNumElements();

        // Check if the range is different and if so force a garbage collection
        // which will make sure the points are up to date
        if(previousRange != newRange) {
            sceneDelegate->GetRenderIndex().GetChangeTracker().SetGarbageCollectionNeeded();
        }
    }

    // add sources to update queue
    resourceRegistry->AddSources(drawItem->GetVertexPrimVarRange(),
                                 sources);
}

HdDirtyBits 
HdStPoints::_GetInitialDirtyBits() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyInstanceIndex
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimVar
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtySurfaceShader
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
HdStPoints::_InitRepr(TfToken const &reprName,
                      HdDirtyBits *dirtyBits)
{
    _PointsReprConfig::DescArray const &descs = _GetReprDesc(reprName);

    if (_reprs.empty()) {
        _reprs.emplace_back(reprName, boost::make_shared<HdRepr>());

        HdReprSharedPtr &repr = _reprs.back().second;

        // allocate all draw items
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdPointsReprDesc &desc = descs[descIdx];

            if (desc.geomStyle != HdPointsGeomStyleInvalid) {
                repr->AddDrawItem(&_sharedData);
            }
        }

        *dirtyBits |= DirtyNewRepr;
    }

}


PXR_NAMESPACE_CLOSE_SCOPE

