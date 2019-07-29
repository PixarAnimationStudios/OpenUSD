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
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/rprimUtils.h"

#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/rprimSharedData.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

void
HdStPopulateConstantPrimvars(
    HdRprim* prim,
    HdRprimSharedData *sharedData,
    HdSceneDelegate* delegate,
    HdDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdPrimvarDescriptorVector const& constantPrimvars)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = prim->GetId();
    SdfPath const& instancerId = prim->GetInstancerId();

    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdResourceRegistrySharedPtr const &resourceRegistry = 
        renderIndex.GetResourceRegistry();

    // Update uniforms
    HdBufferSourceVector sources;
    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        GfMatrix4d transform = delegate->GetTransform(id);
        sharedData->bounds.SetMatrix(transform); // for CPU frustum culling

        HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                           HdTokens->transform,
                                           transform));
        sources.push_back(source);
        source.reset(new HdVtBufferSource(HdTokens->transformInverse,
                                          transform.GetInverse()));
        sources.push_back(source);

        // If this is a prototype (has instancer),
        // also push the instancer transform separately.
        if (!instancerId.IsEmpty()) {
            // Gather all instancer transforms in the instancing hierarchy
            VtMatrix4dArray rootTransforms = 
                prim->GetInstancerTransforms(delegate);
            VtMatrix4dArray rootInverseTransforms(rootTransforms.size());
            bool leftHanded = transform.IsLeftHanded();
            for (size_t i = 0; i < rootTransforms.size(); ++i) {
                rootInverseTransforms[i] = rootTransforms[i].GetInverse();
                // Flip the handedness if necessary
                leftHanded ^= rootTransforms[i].IsLeftHanded();
            }

            source.reset(new HdVtBufferSource(
                             HdTokens->instancerTransform,
                             rootTransforms,
                             rootTransforms.size()));
            sources.push_back(source);
            source.reset(new HdVtBufferSource(
                             HdTokens->instancerTransformInverse,
                             rootInverseTransforms,
                             rootInverseTransforms.size()));
            sources.push_back(source);

            // XXX: It might be worth to consider to have isFlipped
            // for non-instanced prims as well. It can improve
            // the drawing performance on older-GPUs by reducing
            // fragment shader cost, although it needs more GPU memory.

            // Set as int (GLSL needs 32-bit align for bool)
            source.reset(new HdVtBufferSource(
                             HdTokens->isFlipped, VtValue(int(leftHanded))));
            sources.push_back(source);
        }
    }
    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        sharedData->bounds.SetRange(prim->GetExtent(delegate));

        GfVec3d const & localMin = drawItem->GetBounds().GetBox().GetMin();
        HdBufferSourceSharedPtr sourceMin(new HdVtBufferSource(
                                           HdTokens->bboxLocalMin,
                                           VtValue(GfVec4f(
                                               localMin[0],
                                               localMin[1],
                                               localMin[2],
                                               1.0f))));
        sources.push_back(sourceMin);

        GfVec3d const & localMax = drawItem->GetBounds().GetBox().GetMax();
        HdBufferSourceSharedPtr sourceMax(new HdVtBufferSource(
                                           HdTokens->bboxLocalMax,
                                           VtValue(GfVec4f(
                                               localMax[0],
                                               localMax[1],
                                               localMax[2],
                                               1.0f))));
        sources.push_back(sourceMax);
    }

    if (HdChangeTracker::IsPrimIdDirty(*dirtyBits, id)) {
        int32_t primId = prim->GetPrimId();
        HdBufferSourceSharedPtr source(new HdVtBufferSource(
                                           HdTokens->primID,
                                           VtValue(primId)));
        sources.push_back(source);
    }

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        sources.reserve(sources.size()+constantPrimvars.size());
        for (const HdPrimvarDescriptor& pv: constantPrimvars) {
            if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, pv.name)) {
                VtValue value = delegate->Get(id, pv.name);

                // XXX Storm doesn't support string primvars yet
                if (value.IsHolding<std::string>()) {
                    continue;
                }

                if (value.IsArrayValued() && value.GetArraySize() == 0) {
                    // A value holding an empty array does not count as an
                    // empty value. Catch that case here.
                    TF_WARN("Empty array value for constant primvar %s "
                            "on Rprim %s", pv.name.GetText(), id.GetText());
                } else if (!value.IsEmpty()) {
                    // Given that this is a constant primvar, if it is
                    // holding VtArray then use that as a single array
                    // value rather than as one value per element.
                    HdBufferSourceSharedPtr source(
                        new HdVtBufferSource(pv.name, value,
                            value.IsArrayValued() ? value.GetArraySize() : 1));

                    TF_VERIFY(source->GetTupleType().type != HdTypeInvalid);
                    TF_VERIFY(source->GetTupleType().count > 0);
                    sources.push_back(source);
                }
            }
        }
    }

    // If no sources are found no need to allocate,
    // we can early out.
    if (sources.empty()){
        return;
    }

    // Allocate a new uniform buffer if not exists.
    if (!drawItem->GetConstantPrimvarRange()) {
        // establish a buffer range
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateShaderStorageBufferArrayRange(
                HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
        TF_VERIFY(range->IsValid());

        sharedData->barContainer.Set(
            drawItem->GetDrawingCoord()->GetConstantPrimvarIndex(), range);
    }
    TF_VERIFY(drawItem->GetConstantPrimvarRange()->IsValid());

    resourceRegistry->AddSources(
        drawItem->GetConstantPrimvarRange(), sources);
}

PXR_NAMESPACE_CLOSE_SCOPE
