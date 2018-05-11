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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/smoothNormals.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_SmoothNormalsComputationGPU::HdSt_SmoothNormalsComputationGPU(
    Hd_VertexAdjacency const *adjacency,
    TfToken const &srcName, TfToken const &dstName,
    HdType srcDataType, HdType dstDataType)
    : _adjacency(adjacency), _srcName(srcName), _dstName(dstName)
    , _srcDataType(srcDataType), _dstDataType(dstDataType)
{
    if (srcDataType != HdTypeFloatVec3 && srcDataType != HdTypeDoubleVec3) {
        TF_CODING_ERROR(
            "Unsupported points type %s for computing smooth normals",
            TfEnum::GetName(srcDataType).c_str());
        _srcDataType = HdTypeInvalid;
    }
    if (dstDataType != HdTypeFloatVec3 && dstDataType != HdTypeDoubleVec3 &&
        dstDataType != HdTypeInt32_2_10_10_10_REV) {
        TF_CODING_ERROR(
            "Unsupported normals type %s for computing smooth normals",
            TfEnum::GetName(dstDataType).c_str());
        _dstDataType = HdTypeInvalid;
    }
}

void
HdSt_SmoothNormalsComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const &range_,
    HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!glDispatchCompute)
        return;
    if (_srcDataType == HdTypeInvalid || _dstDataType == HdTypeInvalid)
        return;

    TF_VERIFY(_adjacency);
    HdBufferArrayRangeSharedPtr const &adjacencyRange_ = 
        _adjacency->GetAdjacencyRange();
    TF_VERIFY(adjacencyRange_);

    HdStBufferArrayRangeGLSharedPtr adjacencyRange =
        boost::static_pointer_cast<HdStBufferArrayRangeGL> (adjacencyRange_);

    // select shader by datatype
    TfToken shaderToken;
    if (_srcDataType == HdTypeFloatVec3) {
        if (_dstDataType == HdTypeFloatVec3) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsFloatToFloat;
        } else if (_dstDataType == HdTypeDoubleVec3) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsFloatToDouble;
        } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsFloatToPacked;
        }
    } else if (_srcDataType == HdTypeDoubleVec3) {
        if (_dstDataType == HdTypeFloatVec3) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsDoubleToFloat;
        } else if (_dstDataType == HdTypeDoubleVec3) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsDoubleToDouble;
        } else if (_dstDataType == HdTypeInt32_2_10_10_10_REV) {
            shaderToken = HdStGLSLProgramTokens->smoothNormalsDoubleToPacked;
        }
    }
    if (!TF_VERIFY(!shaderToken.IsEmpty())) return;

    HdStGLSLProgramSharedPtr computeProgram
        = HdStGLSLProgram::GetComputeProgram(shaderToken,
            static_cast<HdStResourceRegistry*>(resourceRegistry));
    if (!computeProgram) return;

    GLuint program = computeProgram->GetProgram().GetId();

    HdStBufferArrayRangeGLSharedPtr range =
        boost::static_pointer_cast<HdStBufferArrayRangeGL> (range_);

    // buffer resources for GPU computation
    HdStBufferResourceGLSharedPtr points = range->GetResource(_srcName);
    HdStBufferResourceGLSharedPtr normals = range->GetResource(_dstName);
    HdStBufferResourceGLSharedPtr adjacency = adjacencyRange->GetResource();

    // prepare uniform buffer for GPU computation
    struct Uniform {
        int vertexOffset;
        int adjacencyOffset;
        int pointsOffset;
        int pointsStride;
        int normalsOffset;
        int normalsStride;
    } uniform;

    // coherent vertex offset in aggregated buffer array
    uniform.vertexOffset = range->GetOffset();
    // adjacency offset/stride in aggregated adjacency table
    uniform.adjacencyOffset = adjacencyRange->GetOffset();
    // interleaved offset/stride to points
    // note: this code (and the glsl smooth normal compute shader) assumes
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

    // The number of points is based off the size of the output,
    // However, the number of points in the adjacency table
    // is computed based off the largest vertex indexed from
    // to topology (aka topology->ComputeNumPoints).
    //
    // Therefore, we need to clamp the number of points
    // to the number of entries in the adjancency table.
    int numDestPoints = range->GetNumElements();
    int numSrcPoints = _adjacency->GetNumPoints();

    int numPoints = std::min(numSrcPoints, numDestPoints);

    // transfer uniform buffer
    GLuint ubo = computeProgram->GetGlobalUniformBuffer().GetId();
    GlfContextCaps const &caps = GlfContextCaps::GetInstance();
    // XXX: workaround for 319.xx driver bug of glNamedBufferDataEXT on UBO
    // XXX: move this workaround to renderContextCaps
    if (false && caps.directStateAccessEnabled) {
        glNamedBufferDataEXT(ubo, sizeof(uniform), &uniform, GL_STATIC_DRAW);
    } else {
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(uniform), &uniform, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, points->GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, normals->GetId());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, adjacency->GetId());

    // dispatch compute kernel
    glUseProgram(program);

    glDispatchCompute(numPoints, 1, 1);

    glUseProgram(0);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
}

void
HdSt_SmoothNormalsComputationGPU::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    specs->emplace_back(_dstName, HdTupleType {_dstDataType, 1});
}


PXR_NAMESPACE_CLOSE_SCOPE

