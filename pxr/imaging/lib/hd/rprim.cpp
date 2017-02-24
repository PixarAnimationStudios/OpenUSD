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
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE


HdRprim::HdRprim(SdfPath const& id,
                 SdfPath const& instancerID)
    : _id(id)
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
HdRprim::GetDrawItems(HdSceneDelegate* delegate,
                      TfToken const &defaultReprName, bool forced)
{
    // note: GetDrawItems is called at execute phase.
    // All required dirtyBits should have cleaned at this point.
    HdDirtyBits dirtyBits(HdChangeTracker::Clean);
    TfToken reprName = _GetReprName(delegate, defaultReprName,
                                    forced, &dirtyBits);
    return _GetRepr(delegate, reprName, &dirtyBits)->GetDrawItems();
}


void
HdRprim::_Sync(HdSceneDelegate* delegate,
              TfToken const &defaultReprName,
              bool forced,
              HdDirtyBits *dirtyBits)
{
    HdRenderIndex   &renderIndex   = delegate->GetRenderIndex();
    HdChangeTracker &changeTracker = renderIndex.GetChangeTracker();

    // Check if the rprim has a new surface shader associated to it,
    // if so, we will request the binding from the delegate and set it up in
    // this rprim.
    if(*dirtyBits & HdChangeTracker::DirtySurfaceShader) {
        VtValue shaderBinding = 
            delegate->Get(GetId(), HdShaderTokens->surfaceShader);

        if(shaderBinding.IsHolding<SdfPath>()){
            _SetSurfaceShaderId(changeTracker, shaderBinding.Get<SdfPath>());
        } else {
            _SetSurfaceShaderId(changeTracker, SdfPath());
        }

        *dirtyBits &= ~HdChangeTracker::DirtySurfaceShader;
    }
}

TfToken
HdRprim::_GetReprName(HdSceneDelegate* delegate,
                      TfToken const &defaultReprName,
                      bool forced,
                      HdDirtyBits *dirtyBits)
{
    // resolve reprName

    // if not forced, the prim's authored reprname wins.
    // otherewise we respect defaultReprName (used for shadowmap drawing etc)
    if (!forced) {
        SdfPath const& id = GetId();
        if (HdChangeTracker::IsReprDirty(*dirtyBits, id)) {
            _authoredReprName = delegate->GetReprName(id);
        }
        if (!_authoredReprName.IsEmpty()) {
            return _authoredReprName;
        }
    }
    return defaultReprName;
}

// Static
HdDirtyBits
HdRprim::_PropagateRprimDirtyBits(HdDirtyBits bits)
{
    // propagate point dirtiness to normal
    bits |= (bits & HdChangeTracker::DirtyPoints) ?
                                              HdChangeTracker::DirtyNormals : 0;

    // when refine level changes, topology becomes dirty.
    // XXX: can we remove DirtyRefineLevel then?
    if (bits & HdChangeTracker::DirtyRefineLevel) {
        bits |=  HdChangeTracker::DirtyTopology;
    }

    // if topology changes, all dependent bits become dirty.
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= (HdChangeTracker::DirtyPoints  |
                 HdChangeTracker::DirtyNormals |
                 HdChangeTracker::DirtyPrimVar);
    }
    return bits;
}

void
HdRprim::_UpdateVisibility(HdSceneDelegate* delegate,
                           HdDirtyBits *dirtyBits)
{
    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, GetId())) {
        _sharedData.visible = delegate->GetVisible(GetId());
    }
}


TfToken
HdRprim::GetRenderTag(HdSceneDelegate* delegate) const
{
    return delegate->GetRenderTag(_id);
}

void 
HdRprim::_SetSurfaceShaderId(HdChangeTracker &changeTracker,
                             SdfPath const& surfaceShaderId)
{
    if (_surfaceShaderID != surfaceShaderId)
    {
        _surfaceShaderID = surfaceShaderId;

        // The batches need to be verified and rebuilt if necessary.
        changeTracker.MarkShaderBindingsDirty();
    }
}

bool
HdRprim::IsDirty(HdChangeTracker &changeTracker)
{
    return changeTracker.IsRprimDirty(GetId());
}

void
HdRprim::SetPrimId(int32_t primId)
{
    _primId = primId;
    // Don't set DirtyPrimID here, to avoid undesired variability tracking.
}

HdDirtyBits
HdRprim::GetInitialDirtyBitsMask() const
{
    return _GetInitialDirtyBits();
}

void
HdRprim::_PopulateConstantPrimVars(HdSceneDelegate* delegate,
                                   HdDrawItem *drawItem,
                                   HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();


    // XXX: this should be in a different method
    // XXX: This should be in HdSt getting the HdSt Shader
    const HdShader *shader = static_cast<const HdShader *>(
                                  renderIndex.GetSprim(HdPrimTypeTokens->shader,
                                                       _surfaceShaderID));

    if (shader == nullptr) {
        shader = static_cast<const HdShader *>(
                        renderIndex.GetFallbackSprim(HdPrimTypeTokens->shader));
    }

    _sharedData.surfaceShader = shader->GetShaderCode();


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
            VtMatrix4dArray rootTransforms = _GetInstancerTransforms(delegate);
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
        _sharedData.bounds.SetRange(GetExtent(delegate));

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
HdRprim::_PopulateInstancePrimVars(HdSceneDelegate* delegate,
                                   HdDrawItem *drawItem,
                                   HdDirtyBits *dirtyBits,
                                   int instancePrimVarSlot)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    if (_instancerID.IsEmpty()) {
        return;
    }

    HdRenderIndex &renderIndex = delegate->GetRenderIndex();

    HdInstancerSharedPtr instancer = renderIndex.GetInstancer(_instancerID);
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
            = renderIndex.GetInstancer(currentInstancer->GetParentId());
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
HdRprim::_GetInstancerTransforms(HdSceneDelegate* delegate)
{
    SdfPath const& id = GetId();
    SdfPath instancerID = _instancerID;
    VtMatrix4dArray transforms;

    HdRenderIndex &renderIndex = delegate->GetRenderIndex();

    while (!instancerID.IsEmpty()) {
        transforms.push_back(delegate->GetInstancerTransform(instancerID, id));
        HdInstancerSharedPtr instancer = renderIndex.GetInstancer(instancerID);
        if (instancer) {
            instancerID = instancer->GetParentId();
        } else {
            instancerID = SdfPath();
        }
    }
    return transforms;
}

PXR_NAMESPACE_CLOSE_SCOPE

