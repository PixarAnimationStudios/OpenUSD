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
#include "pxr/imaging/hdSt/smoothNormals.h"

#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/vertexAdjacency.h"

#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_SmoothNormalsComputationCPU::HdSt_SmoothNormalsComputationCPU(
    Hd_VertexAdjacency const *adjacency,
    HdBufferSourceSharedPtr const &points,
    TfToken const &dstName,
    HdBufferSourceSharedPtr const &adjacencyBuilder,
    bool packed)
    : _adjacency(adjacency)
    , _points(points)
    , _dstName(dstName)
    , _adjacencyBuilder(adjacencyBuilder)
    , _packed(packed)
{
}

void
HdSt_SmoothNormalsComputationCPU::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    // The datatype of normals is the same as that of points,
    // unless the packed format was requested.
    specs->emplace_back(_dstName,
        _packed
            ? HdTupleType { HdTypeInt32_2_10_10_10_REV, 1 }
            : _points->GetTupleType() );
}

TfToken const &
HdSt_SmoothNormalsComputationCPU::GetName() const
{
    return _dstName;
}

bool
HdSt_SmoothNormalsComputationCPU::Resolve()
{
    // dependency check first
    if (_adjacencyBuilder) {
        if (!_adjacencyBuilder->IsResolved()) return false;
    }
    if (!_points->IsResolved()) return false;
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_adjacency)) return true;

    size_t numPoints = _points->GetNumElements();

    HdBufferSourceSharedPtr normals;

    switch (_points->GetTupleType().type) {
    case HdTypeFloatVec3:
        if (_packed) {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormalsPacked(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3f*>(_points->GetData())))));
        } else {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormals(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3f*>(_points->GetData())))));
        }
        break;
    case HdTypeDoubleVec3:
        if (_packed) {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormalsPacked(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3d*>(_points->GetData())))));
        } else {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        Hd_SmoothNormals::ComputeSmoothNormals(
                            _adjacency,
                            numPoints,
                            static_cast<const GfVec3d*>(_points->GetData())))));
        }
        break;
    default:
        TF_CODING_ERROR("Unsupported points type for computing smooth normals");
        break;
    }

    _SetResult(normals);

    // call base class to mark as resolved.
    _SetResolved();
    return true;
}

bool
HdSt_SmoothNormalsComputationCPU::_CheckValid() const
{
    bool valid = _points ? _points->IsValid() : false;

    // _adjacencyBuilder is an optional source
    valid &= _adjacencyBuilder ? _adjacencyBuilder->IsValid() : true;

    return valid;
}

namespace {

enum {
    BufferBinding_Uniforms,
    BufferBinding_Points,
    BufferBinding_Normals,
    BufferBinding_Adjacency,
};

HgiResourceBindingsSharedPtr
_CreateResourceBindings(
    Hgi* hgi,
    HgiBufferHandle const& points,
    HgiBufferHandle const& normals,
    HgiBufferHandle const& adjacency)
{
    // Begin the resource set
    HgiResourceBindingsDesc resourceDesc;
    resourceDesc.debugName = "SmoothNormals";

    if (points) {
        HgiBufferBindDesc bufBind0;
        bufBind0.bindingIndex = BufferBinding_Points;
        bufBind0.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind0.stageUsage = HgiShaderStageCompute;
        bufBind0.writable = false;
        bufBind0.offsets.push_back(0);
        bufBind0.buffers.push_back(points);
        resourceDesc.buffers.push_back(std::move(bufBind0));
    }

    if (normals) {
        HgiBufferBindDesc bufBind1;
        bufBind1.bindingIndex = BufferBinding_Normals;
        bufBind1.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind1.stageUsage = HgiShaderStageCompute;
        bufBind1.writable = true;
        bufBind1.offsets.push_back(0);
        bufBind1.buffers.push_back(normals);
        resourceDesc.buffers.push_back(std::move(bufBind1));
    }

    if (adjacency) {
        HgiBufferBindDesc bufBind2;
        bufBind2.bindingIndex = BufferBinding_Adjacency;
        bufBind2.resourceType = HgiBindResourceTypeStorageBuffer;
        bufBind2.stageUsage = HgiShaderStageCompute;
        bufBind2.writable = false;
        bufBind2.offsets.push_back(0);
        bufBind2.buffers.push_back(adjacency);
        resourceDesc.buffers.push_back(std::move(bufBind2));
    }

    return std::make_shared<HgiResourceBindingsHandle>(
        hgi->CreateResourceBindings(resourceDesc));
}

HgiComputePipelineSharedPtr
_CreatePipeline(
    Hgi* hgi,
    uint32_t constantValuesSize,
    HgiShaderProgramHandle const& program)
{
    HgiComputePipelineDesc desc;
    desc.debugName = "SmoothNormals";
    desc.shaderProgram = program;
    desc.shaderConstantsDesc.byteSize = constantValuesSize;
    return std::make_shared<HgiComputePipelineHandle>(
        hgi->CreateComputePipeline(desc));
}

} // Anonymous namespace

HdSt_SmoothNormalsComputationGPU::HdSt_SmoothNormalsComputationGPU(
    HdSt_VertexAdjacencyBuilder const *vertexAdjacencyBuilder,
    TfToken const &srcName, TfToken const &dstName,
    HdType srcDataType, bool packed)
    : _vertexAdjacencyBuilder(vertexAdjacencyBuilder)
    , _srcName(srcName)
    , _dstName(dstName)
    , _srcDataType(srcDataType)
{
    if (srcDataType != HdTypeFloatVec3 && srcDataType != HdTypeDoubleVec3) {
        TF_CODING_ERROR(
            "Unsupported points type %s for computing smooth normals",
            TfEnum::GetName(srcDataType).c_str());
        _srcDataType = HdTypeInvalid;
    }
    _dstDataType = packed ? HdTypeInt32_2_10_10_10_REV : _srcDataType;
}

void
HdSt_SmoothNormalsComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const &range_,
    HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_srcDataType == HdTypeInvalid)
        return;

    TF_VERIFY(_vertexAdjacencyBuilder);
    HdBufferArrayRangeSharedPtr const &adjacencyRange_ =
        _vertexAdjacencyBuilder->GetVertexAdjacencyRange();
    TF_VERIFY(adjacencyRange_);

    HdStBufferArrayRangeSharedPtr adjacencyRange =
        std::static_pointer_cast<HdStBufferArrayRange>(adjacencyRange_);

    // select shader by datatype
    TfToken shaderToken;
    if (_srcDataType == HdTypeFloatVec3) {
        if (_dstDataType == HdTypeFloatVec3) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsFloatToFloat;
        } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsFloatToPacked;
        }
    } else if (_srcDataType == HdTypeDoubleVec3) {
        if (_dstDataType == HdTypeDoubleVec3) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsDoubleToDouble;
        } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsDoubleToPacked;
        }
    }
    if (!TF_VERIFY(!shaderToken.IsEmpty())) return;

    struct Uniform {
        int vertexOffset;
        int adjacencyOffset;
        int pointsOffset;
        int pointsStride;
        int normalsOffset;
        int normalsStride;
        int indexEnd;
    } uniform;

    HdStResourceRegistry* hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);
    HdStGLSLProgramSharedPtr computeProgram
        = HdStGLSLProgram::GetComputeProgram(shaderToken, hdStResourceRegistry,
          [&](HgiShaderFunctionDesc &computeDesc) {
            computeDesc.debugName = shaderToken.GetString();
            computeDesc.shaderStage = HgiShaderStageCompute;
            computeDesc.computeDescriptor.localSize = GfVec3i(64, 1, 1);

            TfToken srcType;
            TfToken dstType;
            if (_srcDataType == HdTypeFloatVec3) {
                srcType = HdStTokens->_float;
            } else {
                srcType = HdStTokens->_double;
            }

            if (_dstDataType == HdTypeFloatVec3) {
                dstType = HdStTokens->_float;
            } else if (_dstDataType == HdTypeDoubleVec3) {
                dstType = HdStTokens->_double;
            } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
                dstType = HdStTokens->_int;
            }
            HgiShaderFunctionAddBuffer(&computeDesc,
                "points", srcType,
                BufferBinding_Points, HgiBindingTypePointer);
            HgiShaderFunctionAddWritableBuffer(&computeDesc,
                "normals", dstType,
                BufferBinding_Normals);
            HgiShaderFunctionAddBuffer(&computeDesc,
                "entry", HdStTokens->_int,
                BufferBinding_Adjacency, HgiBindingTypePointer);

            static const std::string params[] = {
                "vertexOffset",       // offset in aggregated buffer
                "adjacencyOffset",    // offset in aggregated buffer
                "pointsOffset",       // interleave offset
                "pointsStride",       // interleave stride
                "normalsOffset",      // interleave offset
                "normalsStride",      // interleave stride
                "indexEnd"
            };
            static_assert((sizeof(Uniform) / sizeof(int)) ==
                          (sizeof(params) / sizeof(params[0])), "");
            for (std::string const & param : params) {
                HgiShaderFunctionAddConstantParam(
                    &computeDesc, param, HdStTokens->_int);
            }
            HgiShaderFunctionAddStageInput(
                &computeDesc, "hd_GlobalInvocationID", "uvec3",
                HgiShaderKeywordTokens->hdGlobalInvocationID);
        });

    if (!computeProgram) return;

    HdStBufferArrayRangeSharedPtr range =
        std::static_pointer_cast<HdStBufferArrayRange> (range_);

    // buffer resources for GPU computation
    HdStBufferResourceSharedPtr points = range->GetResource(_srcName);
    HdStBufferResourceSharedPtr normals = range->GetResource(_dstName);
    HdStBufferResourceSharedPtr adjacency = adjacencyRange->GetResource();

    // prepare uniform buffer for GPU computation
    // coherent vertex offset in aggregated buffer array
    uniform.vertexOffset = range->GetElementOffset();
    // adjacency offset/stride in aggregated adjacency table
    uniform.adjacencyOffset = adjacencyRange->GetElementOffset();
    // interleaved offset/stride to points
    // note: this code (and the glsl smooth normal compute shader) assumes
    // components in interleaved vertex array are always same data type.
    // i.e. it can't handle an interleaved array which interleaves
    // float/double, float/int etc.
    //
    // The offset and stride values we pass to the shader are in terms
    // of indexes, not bytes, so we must convert the HdStBufferResource
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
    // The number of points is based off the size of the output,
    // However, the number of points in the adjacency table
    // is computed based off the largest vertex indexed from
    // to topology (aka topology->ComputeNumPoints).
    //
    // Therefore, we need to clamp the number of points
    // to the number of entries in the adjancency table.
    const int numDestPoints = range->GetNumElements();
    const int numSrcPoints =
        _vertexAdjacencyBuilder->GetVertexAdjacency()->GetNumPoints();

    const int numPoints = std::min(numSrcPoints, numDestPoints);
    uniform.indexEnd = numPoints;

    Hgi* hgi = hdStResourceRegistry->GetHgi();

    // Generate hash for resource bindings and pipeline.
    // XXX Needs fingerprint hash to avoid collisions
    uint64_t rbHash = (uint64_t) TfHash::Combine(
        points->GetHandle().GetId(),
        normals->GetHandle().GetId(),
        adjacency->GetHandle().GetId());

    uint64_t pHash = (uint64_t) TfHash::Combine(
        computeProgram->GetProgram().Get(),
        sizeof(uniform));

    // Get or add resource bindings in registry.
    HdInstance<HgiResourceBindingsSharedPtr> resourceBindingsInstance =
        hdStResourceRegistry->RegisterResourceBindings(rbHash);
    if (resourceBindingsInstance.IsFirstInstance()) {
        HgiResourceBindingsSharedPtr rb =
            _CreateResourceBindings(hgi,
                                    points->GetHandle(),
                                    normals->GetHandle(),
                                    adjacency->GetHandle());
        resourceBindingsInstance.SetValue(rb);
    }

    HgiResourceBindingsSharedPtr const& resourceBindingsPtr =
        resourceBindingsInstance.GetValue();
    HgiResourceBindingsHandle resourceBindings = *resourceBindingsPtr.get();

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
    computeCmds->PushDebugGroup("Smooth Normals Cmds");
    computeCmds->BindResources(resourceBindings);
    computeCmds->BindPipeline(pipeline);

    // transfer uniform buffer
    computeCmds->SetConstantValues(
        pipeline, BufferBinding_Uniforms, sizeof(uniform), &uniform);

    // dispatch compute kernel
    computeCmds->Dispatch(numPoints, 1);

    // submit the work
    computeCmds->PopDebugGroup();
}

void
HdSt_SmoothNormalsComputationGPU::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->emplace_back(_dstName, HdTupleType {_dstDataType, 1});
}


PXR_NAMESPACE_CLOSE_SCOPE

