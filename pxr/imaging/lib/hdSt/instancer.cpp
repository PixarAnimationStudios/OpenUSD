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
#include "pxr/imaging/hdSt/instancer.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStInstancer::HdStInstancer(HdSceneDelegate* delegate,
                             SdfPath const &id,
                             SdfPath const &parentId)
    : HdInstancer(delegate, id, parentId)
    , _numInstancePrimVars(0)
{
}

void
HdStInstancer::PopulateDrawItem(HdDrawItem *drawItem, HdRprimSharedData *sharedData,
                                HdDirtyBits *dirtyBits, int instancePrimVarSlot)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdRenderIndex &renderIndex = GetDelegate()->GetRenderIndex();
    HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();

    /* INSTANCE PRIMVARS */
    // Populate all instance primvars by backtracing hierarachy.
    // GetInstancePrimVars() will update instance primvars if necessary.
    // Update INSTANCE PRIMVARS first so that GetInstanceIndices() can
    // detect inconsistency between indices and the size of primvar arrays.
    int level = 0;
    HdStInstancer *currentInstancer = this;
    while (currentInstancer) {
        // allocate instance primvar slot in the drawing coordinate.
        drawingCoord->SetInstancePrimVarIndex(level,
                                              instancePrimVarSlot + level);
        sharedData->barContainer.Set(
            drawingCoord->GetInstancePrimVarIndex(level),
            currentInstancer->GetInstancePrimVars());

        // next
        currentInstancer = static_cast<HdStInstancer*>(
            renderIndex.GetInstancer(currentInstancer->GetParentId()));
        ++level;
    }

    /* INSTANCE INDICES */
    if (HdChangeTracker::IsInstanceIndexDirty(*dirtyBits, sharedData->rprimID)) {
        sharedData->barContainer.Set(
            drawingCoord->GetInstanceIndexIndex(),
            GetInstanceIndices(sharedData->rprimID));
    }

    TF_VERIFY(drawItem->GetInstanceIndexRange());
}

HdBufferArrayRangeSharedPtr
HdStInstancer::GetInstancePrimVars()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdChangeTracker &changeTracker = 
        GetDelegate()->GetRenderIndex().GetChangeTracker();
    SdfPath const& instancerId = GetId();

    // Two RPrim's might be trying to update the same instancer at once.
    // do a quick unguarded check to see if it is dirty.
    int dirtyBits = changeTracker.GetInstancerDirtyBits(instancerId);
    if (HdChangeTracker::IsAnyPrimVarDirty(dirtyBits, instancerId)) {
        std::lock_guard<std::mutex> lock(_instanceLock);
  
        // Now we have the lock, we need to check again, as another thread might
        // have beat us to updating this instance.
        dirtyBits = changeTracker.GetInstancerDirtyBits(instancerId);

        // check the dirtyBits of this instancer so that the instance primvar will
        // be updated just once even if there're multiple prototypes.
        if (HdChangeTracker::IsAnyPrimVarDirty(dirtyBits, instancerId)) {
            HdStResourceRegistrySharedPtr const& resourceRegistry = 
                boost::static_pointer_cast<HdStResourceRegistry>(
                GetDelegate()->GetRenderIndex().GetResourceRegistry());

            TfTokenVector primVarNames;
            primVarNames = GetDelegate()->GetPrimVarInstanceNames(instancerId);

            // for all instance primvars
            HdBufferSourceVector sources;
            sources.reserve(primVarNames.size());

            // Always reset numInstancePrimVars, for the case the number of
            // instances are varying.
            // XXX: This might overlook the error that only a subset of
            // instance primvars are varying.
            _numInstancePrimVars = 0;

            TF_FOR_ALL(nameIt, primVarNames) {
                if (HdChangeTracker::IsPrimVarDirty(dirtyBits, instancerId, 
                                                    *nameIt)) {
                    VtValue value = GetDelegate()->Get(instancerId, *nameIt);
                    if (!value.IsEmpty()) {
                        HdBufferSourceSharedPtr source;
                        if (*nameIt == HdTokens->instanceTransform &&
                            TF_VERIFY(value.IsHolding<VtArray<GfMatrix4d> >())) {
                            // Explicitly invoke the c'tor taking a
                            // VtArray<GfMatrix4d> to ensure we properly convert to
                            // the appropriate floating-point matrix type.
                            source.reset(new HdVtBufferSource(*nameIt,
                                                              value.UncheckedGet<VtArray<GfMatrix4d> >()));
                        }
                        else {
                            source.reset(new HdVtBufferSource(*nameIt, value));
                        }

                        // This is a defensive check, but ideally we would not be sent
                        // empty arrays from the client. Once UsdImaging can fulfill
                        // this contract efficiently, this check should emit a coding
                        // error.
                        if (source->GetNumElements() == 0)
                            continue;

                        // Latch onto the first numElements we see.
                        int numElements = source->GetNumElements();
                        if (_numInstancePrimVars == 0) {
                            _numInstancePrimVars = numElements;
                        }

                        if (numElements != _numInstancePrimVars) {
                            // This Rprim is now potentially in a bad state.
                            // to prevent crashes, trim down _numInstancePrimVars.
                            //
                            // Also note that this will not catch time-varying update
                            // errors.
                            TF_WARN("Inconsistent number of '%s' values "
                                    "(%d vs %d) for <%s>.",
                                    nameIt->GetText(),
                                    source->GetNumElements(),
                                    _numInstancePrimVars,
                                    instancerId.GetText());
                            _numInstancePrimVars
                                = std::min(numElements, _numInstancePrimVars);
                        }

                        sources.push_back(source);
                    }
                }
            }

            if (!sources.empty()) {
                // if the instance BAR has not been allocated, create new one
                if (!_instancePrimVarRange) {
                    HdBufferSpecVector bufferSpecs;
                    HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);

                    _instancePrimVarRange =
                        resourceRegistry->AllocateNonUniformBufferArrayRange(
                            HdTokens->primVar, bufferSpecs);
                }
                TF_VERIFY(_instancePrimVarRange->IsValid());

                // schedule to sync gpu
                resourceRegistry->AddSources(_instancePrimVarRange, sources);
            }

            // Clear the dirtyBits of this instancer since we just scheduled to
            // update them and we don't want to do again for other prototypes
            // sharing same instancer. This is slightly inconsistent with how we
            // clear the dirtyBits of Rprims in HdRenderIndex, which takes the mask
            // of renderPass into account.
            // We can add another explicit pass for instancer update into
            // HdRenderIndex to be more consistent, if we like instead.
            changeTracker.MarkInstancerClean(instancerId);
        }
    }

    return _instancePrimVarRange;
}

void
HdStInstancer::_GetInstanceIndices(SdfPath const &prototypeId,
                                   std::vector<VtIntArray> *instanceIndicesArray)
{
    SdfPath const &instancerId = GetId();
    VtIntArray instanceIndices
        = GetDelegate()->GetInstanceIndices(instancerId, prototypeId);

    // quick sanity check
    // instance indices should not exceed the size of instance primvars.
    TF_FOR_ALL (it, instanceIndices) {
        if (*it >= _numInstancePrimVars) {
            TF_WARN("Instance index exceeds the number of instance primvars"
                    " (%d >= %d) for <%s>",
                    *it, _numInstancePrimVars, instancerId.GetText());
            instanceIndices.clear();
            // insert 0-th index as placeholder (0th should always exist, since
            // we don't populate instance primvars with numElements == 0).
            instanceIndices.push_back(0);
            break;
        }
    }

    instanceIndicesArray->push_back(instanceIndices);

    if (TfDebug::IsEnabled(HD_INSTANCER_UPDATED)) {
        std::stringstream ss;
        ss << instanceIndices;
        TF_DEBUG(HD_INSTANCER_UPDATED).Msg("GetInstanceIndices for proto <%s> "
            "instancer <%s> (parent: <%s>): %s\n",
            prototypeId.GetText(),
            instancerId.GetText(),
            GetParentId().GetText(),
            ss.str().c_str());
    }

    // backtrace the instancer hierarchy to gather all instance indices.
    if (!GetParentId().IsEmpty()) {
        HdInstancer *parentInstancer =
            GetDelegate()->GetRenderIndex().GetInstancer(GetParentId());
        if (TF_VERIFY(parentInstancer)) {
            static_cast<HdStInstancer*>(parentInstancer)->
                _GetInstanceIndices(instancerId, instanceIndicesArray);
        }
    }
}

HdBufferArrayRangeSharedPtr
HdStInstancer::GetInstanceIndices(SdfPath const &prototypeId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Note: this function is called from the prototype HdRprm only if
    // the prototype has DirtyInstanceIndex. There's no need to guard using
    // dirtyBits within this function.

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::static_pointer_cast<HdStResourceRegistry>(
        GetDelegate()->GetRenderIndex().GetResourceRegistry());

    // delegate provides sparse index array for prototypeId.
    std::vector<VtIntArray> instanceIndicesArray;
    _GetInstanceIndices(prototypeId, &instanceIndicesArray);
    int instancerNumLevels = (int)instanceIndicesArray.size();

    if (!TF_VERIFY(instancerNumLevels > 0)) {
        return HdBufferArrayRangeSharedPtr();
    }

    HdBufferArrayRangeSharedPtr indexRange;

    {
        std::lock_guard<std::mutex> lock(_instanceLock);
        if (!TfMapLookup(_instanceIndexRangeMap, prototypeId, &indexRange)) {
            TF_DEBUG(HD_INSTANCER_UPDATED).Msg("Allocating new instanceIndex "
                    "range for <%s>\n",
                    GetId().GetText());
            HdBufferSpecVector bufferSpecs;
            bufferSpecs.emplace_back(HdTokens->instanceIndices,
                                     HdTupleType {HdTypeInt32, 1});
            // for GPU frustum culling, we need a copy of instanceIndices.
            // see shader/frustumCull.glslfx
            bufferSpecs.emplace_back(HdTokens->culledInstanceIndices,
                                     HdTupleType {HdTypeInt32, 1});

            // allocate new one
            indexRange = resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs);
            _instanceIndexRangeMap[prototypeId] = indexRange;

            // XXX: reconsider the lifetime of instanceIndexRangeMap entities.
            // When a prototype is removed from index, it should be removed from
            // map. Although it's unlikely we remove prototypes without removing
            // instancer, presumably we'll still need some kind of garbage
            // collection.
        } else {
            TF_DEBUG(HD_INSTANCER_UPDATED).Msg("Pre-allocated instanceIndex "
                    "range for <%s>\n",
                    GetId().GetText());

        }
    }

    // create the cartesian product of instanceIndices array. Each tuple is
    // preceded by a global instance index <n>.
    // e.g.
    //   input   : [0,1] [3,4,5] [7,8]
    //   output  : [<0>,0,3,7,  <1>,1,3,7,  <2>,0,4,7,  <3>,1,4,7,
    //              <4>,0,5,7,  <5>,1,5,7,  <6>,0,3,8, ...]

    size_t nTotal = 1;
    for (int i = 0; i < instancerNumLevels; ++i) {
        nTotal *= instanceIndicesArray[i].size();
    }
    int instanceIndexWidth = 1 + instancerNumLevels;

    // if it's going to be too big, issue a warning and just draw the first
    // instance.
    if ((nTotal*instanceIndexWidth) > indexRange->GetMaxNumElements()) {
        TF_WARN("Can't draw prototype %s (too many instances) : "
                "nest level=%d, # of instances=%ld",
                prototypeId.GetText(), instancerNumLevels, nTotal);
        nTotal = 1;
    }

    VtIntArray instanceIndices(nTotal * instanceIndexWidth);
    std::vector<int> currents(instancerNumLevels);
    for (size_t j = 0; j < nTotal; ++j) {
        instanceIndices[j*instanceIndexWidth] = j; // global idx
        for (int i = 0; i < instancerNumLevels; ++i) {
            instanceIndices[j*instanceIndexWidth + i + 1] =
                instanceIndicesArray[i][currents[i]];
        }
        ++currents[0];
        for (int i = 0; i < instancerNumLevels-1; ++i) {
            if (static_cast<size_t>(currents[i]) >= instanceIndicesArray[i].size()) {
                ++currents[i+1];
                currents[i] = 0;
            }
        }
    }

    if (TfDebug::IsEnabled(HD_INSTANCER_UPDATED)) {
        std::stringstream ss;
        ss << instanceIndices;
        TF_DEBUG(HD_INSTANCER_UPDATED).Msg("Flattened indices <%s>: %s\n",
            prototypeId.GetText(),
            ss.str().c_str());
    }

    // update instance indices
    HdBufferSourceVector sources;
    HdBufferSourceSharedPtr source(
        new HdVtBufferSource(HdTokens->instanceIndices,
                             VtValue(instanceIndices)));
    sources.push_back(source);
    source.reset(new HdVtBufferSource(HdTokens->culledInstanceIndices,
                                      VtValue(instanceIndices)));
    sources.push_back(source);
    resourceRegistry->AddSources(indexRange, sources);

    return indexRange;
}

PXR_NAMESPACE_CLOSE_SCOPE

