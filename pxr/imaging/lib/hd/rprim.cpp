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

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/extCompPrimvarBufferSource.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/tf/envSetting.h"

#include "pxr/base/arch/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_ENABLE_SHARED_VERTEX_PRIMVAR, 1,
                      "Enable sharing of vertex primvar");

HdRprim::HdRprim(SdfPath const& id,
                 SdfPath const& instancerId)
    : _id(id)
    , _instancerId(instancerId)
    , _materialId()
    , _sharedData(HdDrawingCoord::DefaultNumSlots,
                  /*hasInstancer=*/(!instancerId.IsEmpty()),
                  /*visible=*/true)
{
    _sharedData.rprimID = id;
}

HdRprim::~HdRprim()
{
    /*NOTHING*/
}

void
HdRprim::Finalize(HdRenderParam *renderParam)
{
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
    HdReprSharedPtr repr = _GetRepr(delegate, reprName, &dirtyBits);

    if (repr) {
        return repr->GetDrawItems();
    } else {
        return nullptr;
    }
}


void
HdRprim::_Sync(HdSceneDelegate* delegate,
              TfToken const &defaultReprName,
              bool forced,
              HdDirtyBits *dirtyBits)
{
    HdRenderIndex   &renderIndex   = delegate->GetRenderIndex();
    HdChangeTracker &changeTracker = renderIndex.GetChangeTracker();

    // Check if the rprim has a new material binding associated to it,
    // if so, we will request the binding from the delegate and set it up in
    // this rprim.
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        VtValue materialId = 
            delegate->Get(GetId(), HdShaderTokens->surfaceShader);

        if (materialId.IsHolding<SdfPath>()){
            _SetMaterialId(changeTracker, materialId.Get<SdfPath>());
        } else {
            _SetMaterialId(changeTracker, SdfPath());
        }

        *dirtyBits &= ~HdChangeTracker::DirtyMaterialId;
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

bool
HdRprim::CanSkipDirtyBitPropagationAndSync(HdDirtyBits bits) const
{
    // For invisible prims, we'd like to avoid syncing data, which involves:
    // (a) the scene delegate pulling data post dirty-bit propagation 
    // (b) the rprim processing its dirty bits and
    // (c) the rprim committing resource updates to the GPU
    // 
    // However, the current design adds a draw item for a repr during repr 
    // initialization (see _InitRepr) even if a prim may be invisible, which
    // requires us go through the sync process to avoid tripping other checks.
    // 
    // XXX: We may want to avoid this altogether, or rethink how we approach
    // the two workflow scenarios:
    // ( i) objects that are always invisible (i.e., never loaded by the user or
    // scene)
    // (ii) vis-invis'ing objects
    //  
    // For now, we take the hit of first repr initialization (+ sync) and avoid
    // time-varying updates to the invisible prim.
    // 
    // Note: If the sync is skipped, the dirty bits in the change tracker
    // remain the same.
    bool skip = false;

    HdDirtyBits mask = (HdChangeTracker::DirtyVisibility |
                        HdChangeTracker::NewRepr);

    if (!IsVisible() && !(bits & mask)) {
        // By setting the propagated dirty bits to Clean, we effectively 
        // disable delegate and rprim sync
        skip = true;
        HD_PERF_COUNTER_INCR(HdPerfTokens->skipInvisibleRprimSync);
    }

    return skip;
}

HdDirtyBits
HdRprim::PropagateRprimDirtyBits(HdDirtyBits bits)
{
    // If the dependent computations changed - assume all
    // primvars are dirty
    if (bits & HdChangeTracker::DirtyComputationPrimvarDesc) {
        bits |= (HdChangeTracker::DirtyPoints  |
                 HdChangeTracker::DirtyNormals |
                 HdChangeTracker::DirtyWidths  |
                 HdChangeTracker::DirtyPrimVar);
    }

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

    // Let subclasses propagate bits
    return _PropagateDirtyBits(bits);
}

void
HdRprim::InitRepr(HdSceneDelegate* delegate,
                  TfToken const &defaultReprName,
                  bool forced,
                  HdDirtyBits *dirtyBits)
{
    TfToken reprName = _GetReprName(delegate, defaultReprName,
                                    forced, dirtyBits);

    _InitRepr(reprName, dirtyBits);

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
HdRprim::GetRenderTag(HdSceneDelegate* delegate, TfToken const& reprName) const
{
    return delegate->GetRenderTag(_id, reprName);
}

void 
HdRprim::_SetMaterialId(HdChangeTracker &changeTracker,
                        SdfPath const& materialId)
{
    if (_materialId != materialId) {
        _materialId = materialId;

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

HdShaderCodeSharedPtr
HdRprim::_GetShaderCode(HdSceneDelegate *delegate, HdShader const *shader) const
{
    return shader->GetShaderCode();
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
    HdResourceRegistrySharedPtr const &resourceRegistry = 
        renderIndex.GetResourceRegistry();

    // XXX: this should be in a different method
    // XXX: This should be in HdSt getting the HdSt Shader
    const HdShader *shader = static_cast<const HdShader *>(
                                  renderIndex.GetSprim(HdPrimTypeTokens->shader,
                                                       _materialId));

    if (shader == nullptr) {
        shader = static_cast<const HdShader *>(
                        renderIndex.GetFallbackSprim(HdPrimTypeTokens->shader));
    }

    _sharedData.material = _GetShaderCode(delegate, shader);


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
        if (!_instancerId.IsEmpty()) {
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

                        // Check that source data isn't an array.
                        // (Note: we would have to pass true to the
                        // staticArray param of the HdVtBufferSource in
                        // order to accept an array here)
                        if (source->GetNumElements() == 1) {
                            // Ok: Everything is Good.  Add the source.
                            sources.push_back(source);
                        } else {
                            TF_WARN(
                              "Expected non-array value for "
                              "constant primvar %s on Rprim %s",
                              nameIt->GetText(), id.GetText());
                        }
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

VtMatrix4dArray
HdRprim::_GetInstancerTransforms(HdSceneDelegate* delegate)
{
    SdfPath const& id = GetId();
    SdfPath instancerId = _instancerId;
    VtMatrix4dArray transforms;

    HdRenderIndex &renderIndex = delegate->GetRenderIndex();

    while (!instancerId.IsEmpty()) {
        transforms.push_back(delegate->GetInstancerTransform(instancerId, id));
        HdInstancer *instancer = renderIndex.GetInstancer(instancerId);
        if (instancer) {
            instancerId = instancer->GetParentId();
        } else {
            instancerId = SdfPath();
        }
    }
    return transforms;
}

//
// De-duplicating and sharing immutable primvar data.
// 
// Primvar data is identified using a hash computed from the
// sources of the primvar data, of which there are generally
// two kinds:
//   - data provided by the scene delegate
//   - data produced by computations
// 
// Immutable and mutable buffer data is managed using distinct
// heaps in the resource registry. Aggregation of buffer array
// ranges within each heap is managed separately.
// 
// We attempt to balance the benefits of sharing vs efficient
// varying update using the following simple strategy:
//
//  - When populating the first repr for an rprim, allocate
//    the primvar range from the immutable heap and attempt
//    to deduplicate the data by looking up the primvarId
//    in the primvar instance registry.
//
//  - When populating an additional repr for an rprim using
//    an existing immutable primvar range, compute an updated
//    primvarId and allocate from the immutable heap, again
//    attempting to deduplicate.
//
//  - Otherwise, migrate the primvar data to the mutable heap
//    and abandon further attempts to deduplicate.
//
//  - The computation of the primvarId for an rprim is cumulative
//    and includes the new sources of data being committed
//    during each successive update.
//
//  - Once we have migrated a primvar allocation to the mutable
//    heap we will no longer spend time computing a primvarId.
//

/* static */
bool
HdRprim::_IsEnabledSharedVertexPrimvar()
{
    static bool enabled =
        (TfGetEnvSetting(HD_ENABLE_SHARED_VERTEX_PRIMVAR) == 1);
    return enabled;
}

uint64_t
HdRprim::_ComputeSharedPrimvarId(uint64_t baseId,
                                 HdBufferSourceVector const &sources,
                                 HdComputationVector const &computations) const
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

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::AddBufferSpecs(&bufferSpecs, computations);
    for (HdBufferSpec const &bufferSpec : bufferSpecs) {
        boost::hash_combine(primvarId, bufferSpec.name);
        boost::hash_combine(primvarId, bufferSpec.glDataType);
        boost::hash_combine(primvarId, bufferSpec.numComponents);
        boost::hash_combine(primvarId, bufferSpec.arraySize);
    }

    return primvarId;
}

HdBufferArrayRangeSharedPtr
HdRprim::_GetSharedPrimvarRange(uint64_t primvarId,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayRangeSharedPtr const &existing,
    bool * isFirstInstance,
    HdResourceRegistrySharedPtr const &resourceRegistry) const
{
    HdInstance<uint64_t, HdBufferArrayRangeSharedPtr> barInstance;
    std::unique_lock<std::mutex> regLock = 
        resourceRegistry->RegisterPrimvarRange(primvarId, &barInstance);

    HdBufferArrayRangeSharedPtr range;

    if (barInstance.IsFirstInstance()) {
        if (existing) {
            range = resourceRegistry->
                MergeNonUniformImmutableBufferArrayRange(
                    HdTokens->primVar, bufferSpecs, existing);
        } else {
            range = resourceRegistry->
                AllocateNonUniformImmutableBufferArrayRange(
                    HdTokens->primVar, bufferSpecs);
        }
        barInstance.SetValue(range);
    } else {
        range = barInstance.GetValue();
    }

    if (isFirstInstance) {
        *isFirstInstance = barInstance.IsFirstInstance();
    }
    return range;
}

void
HdRprim::_GetExtComputationPrimVarsComputations(
                                              HdSceneDelegate *sceneDelegate,
                                              HdInterpolation interpolationMode,
                                              HdDirtyBits dirtyBits,
                                              HdBufferSourceVector *sources)
{
    const SdfPath &id = GetId();

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    TfTokenVector compPrimVars =
            sceneDelegate->GetExtComputationPrimVarNames(id, interpolationMode);

    TF_FOR_ALL(compPrimVarIt, compPrimVars) {
        const TfToken &compPrimVarName =  *compPrimVarIt;

        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, compPrimVarName)) {
            HdExtComputationPrimVarDesc primVarDesc =
                   sceneDelegate->GetExtComputationPrimVarDesc(id,
                                                               compPrimVarName);


            HdExtComputation *sourceComp;
            HdSceneDelegate *sourceCompSceneDelegate;

            renderIndex.GetExtComputationInfo(primVarDesc.computationId,
                                              &sourceComp,
                                              &sourceCompSceneDelegate);
            if (sourceComp != nullptr) {
                HdExtCompCpuComputationSharedPtr cpuComputation =
                            sourceComp->GetComputation(sourceCompSceneDelegate);


                HdBufferSourceSharedPtr primVarBufferSource(
                    new HdExtCompPrimvarBufferSource(
                                              compPrimVarName,
                                              cpuComputation,
                                              primVarDesc.computationOutputName,
                                              primVarDesc.defaultValue));


                sources->push_back(primVarBufferSource);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
