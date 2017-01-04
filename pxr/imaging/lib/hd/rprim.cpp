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
#include "pxr/imaging/hd/rprim.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

HdRprim::HdRprim(HdSceneDelegate* delegate, SdfPath const& id,
                 SdfPath const& instancerID)
    : _delegate(delegate)
    , _id(id)
    , _instancerID(instancerID)
    , _surfaceShaderID()
    , _sharedData(HdDrawingCoord::DefaultNumSlots,
                  /*hasInstancer=*/(!instancerID.IsEmpty()),
                  /*visible=*/true)
{
    _sharedData.rprimID = id;
}

HdRprim::~HdRprim()
{
    /*NOTHING*/
}

std::vector<HdDrawItem>* 
HdRprim::GetDrawItems(TfToken const &defaultReprName, bool forced)
{
    // note: GetDrawItems is called at execute phase.
    // All required dirtyBits should have cleaned at this point.
    HdChangeTracker::DirtyBits dirtyBits(HdChangeTracker::Clean);
    TfToken reprName = _GetReprName(defaultReprName, forced, &dirtyBits);
    return _GetRepr(reprName, &dirtyBits)->GetDrawItems();
}


void
HdRprim::Sync(TfToken const &defaultReprName, bool forced,
              HdChangeTracker::DirtyBits *dirtyBits)
{
    // Check if the rprim has a new surface shader associated to it,
    // if so, we will request the binding from the delegate and set it up in
    // this rprim.
    if(*dirtyBits & HdChangeTracker::DirtySurfaceShader) {
        VtValue shaderBinding = 
            _delegate->Get(GetId(), HdShaderTokens->surfaceShader);

        if(shaderBinding.IsHolding<SdfPath>()){
            SetSurfaceShaderId(shaderBinding.Get<SdfPath>());
        } else {
            SetSurfaceShaderId(SdfPath());
        }

        *dirtyBits &= ~HdChangeTracker::DirtySurfaceShader;
    }

    TfToken reprName = _GetReprName(defaultReprName, forced, dirtyBits);
    _GetRepr(reprName, dirtyBits);

    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

TfToken
HdRprim::_GetReprName(TfToken const &defaultReprName, bool forced,
                      HdChangeTracker::DirtyBits *dirtyBits)
{
    // resolve reprName

    // if not forced, the prim's authored reprname wins.
    // otherewise we respect defaultReprName (used for shadowmap drawing etc)
    if (!forced) {
        SdfPath const& id = GetId();
        if (HdChangeTracker::IsReprDirty(*dirtyBits, id)) {
            _authoredReprName = _delegate->GetReprName(id);
        }
        if (!_authoredReprName.IsEmpty()) {
            return _authoredReprName;
        }
    }
    return defaultReprName;
}

GfRange3d
HdRprim::GetExtent()
{
    return _delegate->GetExtent(GetId());
}

void
HdRprim::_UpdateVisibility(HdChangeTracker::DirtyBits *dirtyBits)
{
    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, GetId())) {
        _sharedData.visible = _delegate->GetVisible(GetId());
    }
}

bool 
HdRprim::IsInCollection(TfToken const& collectionName) const
{
    return _delegate->IsInCollection(_id, collectionName);
}

void 
HdRprim::SetSurfaceShaderId(SdfPath const& surfaceShaderId) 
{
    if (_surfaceShaderID != surfaceShaderId)
    {
        _surfaceShaderID = surfaceShaderId;

        // The batches need to be verified and rebuilt if necessary.
        _GetChangeTracker().MarkShaderBindingsDirty();
    }
}

bool
HdRprim::IsDirty()
{
    return _delegate->GetRenderIndex().GetChangeTracker().IsRprimDirty(_id);
}

void
HdRprim::SetPrimId(int32_t primId)
{
    _primId = primId;
    // Don't set DirtyPrimID here, to avoid undesired variability tracking.
}

HdSceneDelegate*
HdRprim::GetDelegate() const {
    return _delegate;
}

HdRenderIndex& 
HdRprim::_GetRenderIndex() {
    return _delegate->GetRenderIndex();
}
HdRenderIndex const& 
HdRprim::_GetRenderIndex() const {
    return _delegate->GetRenderIndex();
}

int
HdRprim::GetInitialDirtyBitsMask() const
{
    return _GetInitialDirtyBits();
}

HdChangeTracker& 
HdRprim::_GetChangeTracker() {
    return _GetRenderIndex().GetChangeTracker();
}

void
HdRprim::_PopulateConstantPrimVars(HdDrawItem *drawItem,
                                   HdChangeTracker::DirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdSceneDelegate* delegate = GetDelegate();
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    // XXX: this should be in a different method
    _sharedData.surfaceShader = _GetRenderIndex().GetShader(_surfaceShaderID);

    // update uniforms
    HdBufferSourceVector sources;
    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        GfMatrix4d transform = delegate->GetTransform(id);
        _sharedData.bounds.SetMatrix(transform); // for CPU frustum culling

        HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                           HdTokens->transform,
                                           transform));
        sources.push_back(source);
        source.reset(new HdVtBufferSource(HdTokens->transformInverse,
                                          transform.GetInverse()));
        sources.push_back(source);

        // if this is a prototype (has instancer),
        // also push the instancer transform separately.
        if (!_instancerID.IsEmpty()) {
            // gather all instancer transforms in the instancing hierarchy
            VtMatrix4dArray rootTransforms = _GetInstancerTransforms();
            VtMatrix4dArray rootInverseTransforms(rootTransforms.size());
            bool leftHanded = transform.IsLeftHanded();
            for (size_t i = 0; i < rootTransforms.size(); ++i) {
                rootInverseTransforms[i] = rootTransforms[i].GetInverse();
                // flip the handedness if necessary
                leftHanded ^= rootTransforms[i].IsLeftHanded();
            }

            source.reset(new HdVtBufferSource(
                             HdTokens->instancerTransform,
                             rootTransforms, /*staticArray=*/true));
            sources.push_back(source);
            source.reset(new HdVtBufferSource(
                             HdTokens->instancerTransformInverse,
                             rootInverseTransforms, /*staticArray=*/true));
            sources.push_back(source);

            // XXX: It might be worth to consider to have isFlipped
            // for non-instanced prims as well. It can improve
            // the drawing performance on older-GPUs by reducing
            // fragment shader cost, although it needs more GPU memory.

            // set as int (GLSL needs 32-bit align for bool)
            source.reset(new HdVtBufferSource(
                             HdTokens->isFlipped, VtValue(int(leftHanded))));
            sources.push_back(source);
        }
    }
    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        _sharedData.bounds.SetRange(GetExtent());

        GfVec3d const & localMin = drawItem->GetBounds().GetBox().GetMin();
        HdBufferSourceSharedPtr sourceMin(new HdVtBufferSource(
                                           HdTokens->bboxLocalMin,
                                           VtValue(GfVec4f(
                                               localMin[0],
                                               localMin[1],
                                               localMin[2], 0))));
        sources.push_back(sourceMin);

        GfVec3d const & localMax = drawItem->GetBounds().GetBox().GetMax();
        HdBufferSourceSharedPtr sourceMax(new HdVtBufferSource(
                                           HdTokens->bboxLocalMax,
                                           VtValue(GfVec4f(
                                               localMax[0],
                                               localMax[1],
                                               localMax[2], 0))));
        sources.push_back(sourceMax);
    }

    if (HdChangeTracker::IsPrimIdDirty(*dirtyBits, id)) {
        GfVec4f primIdColor;
        int32_t primId = GetPrimId();
        HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                           HdTokens->primID,
                                           VtValue(primId)));
        sources.push_back(source);
    }

    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        TfTokenVector primVarNames = delegate->GetPrimVarConstantNames(id);
        sources.reserve(sources.size()+primVarNames.size());
        TF_FOR_ALL(nameIt, primVarNames) {
            if (HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, *nameIt)) {
                VtValue value = delegate->Get(id, *nameIt);

                // XXX Hydra doesn't support string primvar yet
                if (value.IsHolding<std::string>()) continue;

                if (!value.IsEmpty()) {
                    HdBufferSourceSharedPtr source(
                        new HdVtBufferSource(*nameIt, value));

                    // if it's an unacceptable type, skip it (e.g. std::string)
                    if (source->GetNumComponents() > 0) {
                        sources.push_back(source);
                    }
                }
            }
        }
    }

    // return before allocation if it's empty.
    if (sources.empty())
        return;

    // Allocate a new uniform buffer if not exists.
    if (!drawItem->GetConstantPrimVarRange()) {
        // establish a buffer range
        HdBufferSpecVector bufferSpecs;
        TF_FOR_ALL(srcIt, sources) {
            (*srcIt)->AddBufferSpecs(&bufferSpecs);
        }

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateShaderStorageBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        TF_VERIFY(range->IsValid());

        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetConstantPrimVarIndex(), range);
    }
    TF_VERIFY(drawItem->GetConstantPrimVarRange()->IsValid());

    resourceRegistry->AddSources(
        drawItem->GetConstantPrimVarRange(), sources);
}

void
HdRprim::_PopulateInstancePrimVars(HdDrawItem *drawItem,
                                   HdChangeTracker::DirtyBits *dirtyBits,
                                   int instancePrimVarSlot)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    if (_instancerID.IsEmpty()) {
        return;
    }

    HdInstancerSharedPtr instancer = _GetRenderIndex().GetInstancer(_instancerID);
    if (!TF_VERIFY(instancer)) return;

    HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();

    // populate INSTANCE PRIVARS first so that we can detect inconsistency
    // between the number of instances and instance primvars.

    /* INSTANCE PRIMVARS */
    // we always call GetInstancePrimVars so that HdInstancer updates
    // instance primvars if needed.
    // populate all instance primvars by backtracing hierarachy
    int level = 0;
    HdInstancerSharedPtr currentInstancer = instancer;
    while (currentInstancer) {
        // allocate instance primvar slot in the drawing coordinate.
        drawingCoord->SetInstancePrimVarIndex(level,
                                              instancePrimVarSlot + level);
        _sharedData.barContainer.Set(
            drawingCoord->GetInstancePrimVarIndex(level),
            currentInstancer->GetInstancePrimVars(level));

        // next
        currentInstancer
            = _GetRenderIndex().GetInstancer(currentInstancer->GetParentId());
        ++level;
    }

    /* INSTANCE INDICES */
    if (HdChangeTracker::IsInstanceIndexDirty(*dirtyBits, id)) {
        _sharedData.barContainer.Set(
            drawingCoord->GetInstanceIndexIndex(),
            instancer->GetInstanceIndices(id));
    }

    TF_VERIFY(drawItem->GetInstanceIndexRange());
}

VtMatrix4dArray
HdRprim::_GetInstancerTransforms()
{
    SdfPath const& id = GetId();
    SdfPath instancerID = _instancerID;
    VtMatrix4dArray transforms;
    HdSceneDelegate* delegate = GetDelegate();

    while (!instancerID.IsEmpty()) {
        transforms.push_back(delegate->GetInstancerTransform(instancerID, id));
        HdInstancerSharedPtr instancer
            = _GetRenderIndex().GetInstancer(instancerID);
        if (instancer) {
            instancerID = instancer->GetParentId();
        } else {
            instancerID = SdfPath();
        }
    }
    return transforms;
}

int
HdRprim::GetDirtyBitsMask(TfToken const &reprName)
{
    int mask =
          HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyInstancer
        | HdChangeTracker::DirtyInstanceIndex
        | HdChangeTracker::DirtySurfaceShader;
    return mask;
}
