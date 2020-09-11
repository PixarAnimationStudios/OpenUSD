//
// Copyright 2018 Pixar
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
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/flatNormals.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgi/shaderProgram.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

static HgiResourceBindingsSharedPtr
_CreateResourceBindings(
    Hgi* hgi,
    HgiBufferHandle const& points,
    HgiBufferHandle const& normals,
    HgiBufferHandle const& indices,
    HgiBufferHandle const& primitiveParam)
{
    // Begin the resource set
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "FlatNormals";

    if (points) {
        HgiBufferBindDesc bufBind0;
        bufBind0.bindingIndex = 0;
        bufBind0.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind0.stageUsage = HgiShaderStageCompute;
        bufBind0.offsets.push_back(0);
        bufBind0.buffers.push_back(points);
        resourceDesc.buffers.push_back(std::move(bufBind0));
    }

    if (normals) {
        HgiBufferBindDesc bufBind1;
        bufBind1.bindingIndex = 1;
        bufBind1.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind1.stageUsage = HgiShaderStageCompute;
        bufBind1.offsets.push_back(0);
        bufBind1.buffers.push_back(normals);
        resourceDesc.buffers.push_back(std::move(bufBind1));
    }

    if (indices) {
        HgiBufferBindDesc bufBind2;
        bufBind2.bindingIndex = 2;
        bufBind2.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind2.stageUsage = HgiShaderStageCompute;
        bufBind2.offsets.push_back(0);
        bufBind2.buffers.push_back(indices);
        resourceDesc.buffers.push_back(std::move(bufBind2));
    }

    if (primitiveParam) {
        HgiBufferBindDesc bufBind3;
        bufBind3.bindingIndex = 3;
        bufBind3.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind3.stageUsage = HgiShaderStageCompute;
        bufBind3.offsets.push_back(0);
        bufBind3.buffers.push_back(primitiveParam);
        resourceDesc.buffers.push_back(std::move(bufBind3));
    }

    return std::make_shared<HgiResourceBindingsHandle>(
        hgi->CreateResourceBindings(resourceDesc));
}

static HgiComputePipelineSharedPtr
_CreatePipeline(
    Hgi* hgi,
    uint32_t constantValuesSize,
    HgiShaderProgramHandle const& program)
{
    HgiComputePipelineDesc desc;
    desc.debugName = "FlatNormals";
    desc.shaderProgram = program;
    desc.shaderConstantsDesc.byteSize = constantValuesSize;
    return std::make_shared<HgiComputePipelineHandle>(
        hgi->CreateComputePipeline(desc));
}

HdSt_FlatNormalsComputationGPU::HdSt_FlatNormalsComputationGPU(
    HdBufferArrayRangeSharedPtr const &topologyRange,
    HdBufferArrayRangeSharedPtr const &vertexRange,
    int numFaces, TfToken const &srcName, TfToken const &dstName,
    HdType srcDataType, bool packed)
    : _topologyRange(topologyRange), _vertexRange(vertexRange)
    , _numFaces(numFaces), _srcName(srcName), _dstName(dstName)
    , _srcDataType(srcDataType)
{
    if (srcDataType != HdTypeFloatVec3 && srcDataType != HdTypeDoubleVec3) {
        TF_CODING_ERROR(
            "Unsupported points type %s for computing flat normals",
            TfEnum::GetName(srcDataType).c_str());
        _srcDataType = HdTypeInvalid;
    }
    _dstDataType = packed ? HdTypeInt32_2_10_10_10_REV : _srcDataType;
}

int
HdSt_FlatNormalsComputationGPU::GetNumOutputElements() const
{
    return _numFaces;
}

void
HdSt_FlatNormalsComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const &range_,
    HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_srcDataType == HdTypeInvalid)
        return;

    HdStBufferArrayRangeSharedPtr range =
        std::static_pointer_cast<HdStBufferArrayRange> (range_);
    HdStBufferArrayRangeSharedPtr vertexRange =
        std::static_pointer_cast<HdStBufferArrayRange> (_vertexRange);
    HdStBufferArrayRangeSharedPtr topologyRange =
        std::static_pointer_cast<HdStBufferArrayRange> (_topologyRange);

    // buffer resources for GPU computation
    HdStBufferResourceSharedPtr points = vertexRange->GetResource(_srcName);
    HdStBufferResourceSharedPtr normals = range->GetResource(_dstName);
    HdStBufferResourceSharedPtr indices = topologyRange->GetResource(
        HdTokens->indices);
    HdStBufferResourceSharedPtr primitiveParam = topologyRange->GetResource(
        HdTokens->primitiveParam);

    // select shader by datatype
    TfToken shaderToken;
    int indexArity = HdGetComponentCount(indices->GetTupleType().type);
    if (indexArity == 3) {
        if (_srcDataType == HdTypeFloatVec3) {
            if (_dstDataType == HdTypeFloatVec3) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsTriFloatToFloat;
            } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsTriFloatToPacked;
            }
        } else if (_srcDataType == HdTypeDoubleVec3) {
            if (_dstDataType == HdTypeDoubleVec3) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsTriDoubleToDouble;
            } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsTriDoubleToPacked;
            }
        }
    } else if (indexArity == 4) {
        if (_srcDataType == HdTypeFloatVec3) {
            if (_dstDataType == HdTypeFloatVec3) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsQuadFloatToFloat;
            } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsQuadFloatToPacked;
            }
        } else if (_srcDataType == HdTypeDoubleVec3) {
            if (_dstDataType == HdTypeDoubleVec3) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsQuadDoubleToDouble;
            } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
                shaderToken = HdStGLSLProgramTokens->flatNormalsQuadDoubleToPacked;
            }
        }
    }
    if (!TF_VERIFY(!shaderToken.IsEmpty())) return;
    
    HdStResourceRegistry* hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);
    HdStGLSLProgramSharedPtr computeProgram
        = HdStGLSLProgram::GetComputeProgram(shaderToken, hdStResourceRegistry);
    if (!computeProgram) return;

    // prepare uniform buffer for GPU computation
    struct Uniform {
        int vertexOffset;
        int elementOffset;
        int topologyOffset;
        int pointsOffset;
        int pointsStride;
        int normalsOffset;
        int normalsStride;
        int indexOffset;
        int indexStride;
        int pParamOffset;
        int pParamStride;
    } uniform;

    // coherent vertex offset in aggregated buffer array
    uniform.vertexOffset = vertexRange->GetElementOffset();
    // coherent element offset in aggregated buffer array
    uniform.elementOffset = range->GetElementOffset();
    // coherent topology offset in aggregated buffer array
    uniform.topologyOffset = topologyRange->GetElementOffset();
    // interleaved offset/stride to points
    // note: this code (and the glsl flat normal compute shader) assumes
    // components in interleaved vertex array are always same data type.
    // i.e. it can't handle an interleaved array which interleaves
    // float/double, float/int etc.
    //
    // The offset and stride values we pass to the shader are in terms
    // of indexes, not bytes, so we must convert the HdBufferResource
    // offset/stride (which are in bytes) to counts of float[]/double[]
    // entries.
    const size_t pointComponentSize =
        HdDataSizeOfType(HdGetComponentType(points->GetTupleType().type));
    uniform.pointsOffset = points->GetOffset() / pointComponentSize;
    uniform.pointsStride = points->GetStride() / pointComponentSize;
    // interleaved offset/stride to normals
    const size_t normalComponentSize =
        HdDataSizeOfType(HdGetComponentType(normals->GetTupleType().type));
    uniform.normalsOffset = normals->GetOffset() / normalComponentSize;
    uniform.normalsStride = normals->GetStride() / normalComponentSize;

    const size_t indexComponentSize =
        HdDataSizeOfType(HdGetComponentType(indices->GetTupleType().type));
    uniform.indexOffset = indices->GetOffset() / indexComponentSize;
    uniform.indexStride = indices->GetStride() / indexComponentSize;

    const size_t pParamComponentSize =
        HdDataSizeOfType(HdGetComponentType(primitiveParam->GetTupleType().type));
    uniform.pParamOffset = primitiveParam->GetOffset() / pParamComponentSize;
    uniform.pParamStride = primitiveParam->GetStride() / pParamComponentSize;

    Hgi* hgi = hdStResourceRegistry->GetHgi();
    
    // Generate hash for resource bindings and pipeline.
    // XXX Needs fingerprint hash to avoid collisions
    uint64_t rbHash = (uint64_t) TfHash::Combine(
        points->GetId().Get(),
        normals->GetId().Get(),
        indices->GetId().Get(),
        primitiveParam->GetId().Get());

    uint64_t pHash = (uint64_t) TfHash::Combine(
        computeProgram->GetProgram().Get(),
        sizeof(uniform));

    // Get or add resource bindings in registry.
    HdInstance<HgiResourceBindingsSharedPtr> resourceBindingsInstance =
        hdStResourceRegistry->RegisterResourceBindings(rbHash);
    if (resourceBindingsInstance.IsFirstInstance()) {
        HgiResourceBindingsSharedPtr rb = _CreateResourceBindings(
            hgi, points->GetId(), normals->GetId(), indices->GetId(),
            primitiveParam->GetId());
        resourceBindingsInstance.SetValue(rb);
    }

    HgiResourceBindingsSharedPtr const& resourceBindindsPtr =
        resourceBindingsInstance.GetValue();
    HgiResourceBindingsHandle resourceBindings = *resourceBindindsPtr.get();

    // Get or add pipeline in registry.
    HdInstance<HgiComputePipelineSharedPtr> computePipelineInstance =
        hdStResourceRegistry->RegisterComputePipeline(pHash);
    if (computePipelineInstance.IsFirstInstance()) {
        HgiComputePipelineSharedPtr pipe = _CreatePipeline(
            hgi, sizeof(uniform), computeProgram->GetProgram());
        computePipelineInstance.SetValue(pipe);
    }

    HgiComputePipelineSharedPtr const& pipelinePtr =
        computePipelineInstance.GetValue();
    HgiComputePipelineHandle pipeline = *pipelinePtr.get();

    HgiComputeCmds* computeCmds = hdStResourceRegistry->GetGlobalComputeCmds();
    computeCmds->PushDebugGroup("Flat Normals Cmds");
    computeCmds->BindResources(resourceBindings);
    computeCmds->BindPipeline(pipeline);

    // Queue transfer uniform buffer
    computeCmds->SetConstantValues(pipeline, 0, sizeof(uniform), &uniform);

    // Queue compute work
    int numPrims = topologyRange->GetNumElements();
    computeCmds->Dispatch(numPrims, 1);

    computeCmds->PopDebugGroup();
}

void
HdSt_FlatNormalsComputationGPU::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->emplace_back(_dstName, HdTupleType {_dstDataType, 1});
}


PXR_NAMESPACE_CLOSE_SCOPE
