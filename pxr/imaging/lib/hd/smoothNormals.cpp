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

#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


Hd_SmoothNormalsComputation::Hd_SmoothNormalsComputation(
    Hd_VertexAdjacency const *adjacency,
    HdBufferSourceSharedPtr const &points,
    TfToken const &dstName,
    HdBufferSourceSharedPtr const &adjacencyBuilder,
    bool packed)
    : _adjacency(adjacency), _points(points), _dstName(dstName),
      _adjacencyBuilder(adjacencyBuilder), _packed(packed)
{
}

void
Hd_SmoothNormalsComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // the datatype of normals is same as points (float or double).
    specs->push_back(HdBufferSpec(_dstName,
                                  GetGLComponentDataType(),
                                  (_packed ? 1 : 3)));
}

TfToken const &
Hd_SmoothNormalsComputation::GetName() const
{
    return _dstName;
}

int
Hd_SmoothNormalsComputation::GetGLComponentDataType() const
{
    if (_packed) {
        return GL_INT_2_10_10_10_REV;
    } else {
        return _points->GetGLComponentDataType();
    }
}

bool
Hd_SmoothNormalsComputation::Resolve()
{
    // dependency check first
    if (_adjacencyBuilder) {
        if (!_adjacencyBuilder->IsResolved()) return false;
    }
    if (_points) {
        if (!_points->IsResolved()) return false;
    }
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_adjacency)) return true;

    int numPoints = _points->GetNumElements();

    HdBufferSourceSharedPtr normals;

    switch (_points->GetGLElementDataType()) {
    case GL_FLOAT_VEC3:
        if (_packed) {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        _adjacency->ComputeSmoothNormalsPacked(
                            numPoints,
                            static_cast<const GfVec3f*>(_points->GetData())))));
        } else {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        _adjacency->ComputeSmoothNormals(
                            numPoints,
                            static_cast<const GfVec3f*>(_points->GetData())))));
        }
        break;
    case GL_DOUBLE_VEC3:
        if (_packed) {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        _adjacency->ComputeSmoothNormalsPacked(
                            numPoints,
                            static_cast<const GfVec3d*>(_points->GetData())))));
        } else {
            normals = HdBufferSourceSharedPtr(
                new HdVtBufferSource(
                    _dstName, VtValue(
                        _adjacency->ComputeSmoothNormals(
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
Hd_SmoothNormalsComputation::_CheckValid() const
{
    bool valid = _points ? _points->IsValid() : false;

    // _adjacencyBuilder is an optional source
    valid &= _adjacencyBuilder ? _adjacencyBuilder->IsValid() : true;

    return valid;
}
// ---------------------------------------------------------------------------

Hd_SmoothNormalsComputationGPU::Hd_SmoothNormalsComputationGPU(
    Hd_VertexAdjacency const *adjacency,
    TfToken const &srcName, TfToken const &dstName,
    GLenum srcDataType, GLenum dstDataType)
    : _adjacency(adjacency), _srcName(srcName), _dstName(dstName)
    , _srcDataType(srcDataType), _dstDataType(dstDataType)
{
    if (srcDataType != GL_FLOAT && srcDataType != GL_DOUBLE) {
        TF_CODING_ERROR(
            "Unsupported points type %x for computing smooth normals",
            srcDataType);
        srcDataType = GL_FLOAT;
    }
    if (dstDataType != GL_FLOAT && dstDataType != GL_DOUBLE &&
        dstDataType != GL_INT_2_10_10_10_REV) {
        TF_CODING_ERROR(
            "Unsupported normals type %x for computing smooth normals",
            dstDataType);
        dstDataType = GL_FLOAT;
    }
}

void
Hd_SmoothNormalsComputationGPU::Execute(
    HdBufferArrayRangeSharedPtr const &range_,
    HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!glDispatchCompute)
        return;

    TF_VERIFY(_adjacency);
    HdBufferArrayRangeSharedPtr const &adjacencyRange_ = 
        _adjacency->GetAdjacencyRange();
    TF_VERIFY(adjacencyRange_);

    HdBufferArrayRangeGLSharedPtr adjacencyRange =
        boost::static_pointer_cast<HdBufferArrayRangeGL> (adjacencyRange_);

    // select shader by datatype
    TfToken shaderToken;
    if (_srcDataType == GL_FLOAT) {
        if (_dstDataType == GL_FLOAT) {
            shaderToken = HdGLSLProgramTokens->smoothNormalsFloatToFloat;
        } else if (_dstDataType == GL_DOUBLE) {
            shaderToken = HdGLSLProgramTokens->smoothNormalsFloatToDouble;
        } else if (_dstDataType == GL_INT_2_10_10_10_REV) {
            shaderToken = HdGLSLProgramTokens->smoothNormalsFloatToPacked;
        }
    } else if (_srcDataType == GL_DOUBLE) {
        if (_dstDataType == GL_FLOAT) {
            shaderToken = HdGLSLProgramTokens->smoothNormalsDoubleToFloat;
        } else if (_dstDataType == GL_DOUBLE) {
            shaderToken = HdGLSLProgramTokens->smoothNormalsDoubleToDouble;
        } else if (_dstDataType == GL_INT_2_10_10_10_REV) {
            shaderToken = HdGLSLProgramTokens->smoothNormalsDoubleToPacked;
        }
    }
    if (!TF_VERIFY(!shaderToken.IsEmpty())) return;

    HdGLSLProgramSharedPtr computeProgram
        = HdGLSLProgram::GetComputeProgram(shaderToken, resourceRegistry);
    if (!computeProgram) return;

    GLuint program = computeProgram->GetProgram().GetId();

    HdBufferArrayRangeGLSharedPtr range =
        boost::static_pointer_cast<HdBufferArrayRangeGL> (range_);

    // buffer resources for GPU computation
    HdBufferResourceGLSharedPtr points = range->GetResource(_srcName);
    HdBufferResourceGLSharedPtr normals = range->GetResource(_dstName);
    HdBufferResourceGLSharedPtr adjacency = adjacencyRange->GetResource();

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
    uniform.pointsOffset = points->GetOffset() / points->GetComponentSize();
    uniform.pointsStride = points->GetStride() / points->GetComponentSize();
    // interleaved offset/stride to normals
    uniform.normalsOffset = normals->GetOffset() / normals->GetComponentSize();
    uniform.normalsStride = normals->GetStride() / normals->GetComponentSize();

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
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
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
Hd_SmoothNormalsComputationGPU::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    if (_dstDataType == GL_INT_2_10_10_10_REV) {
        specs->push_back(HdBufferSpec(_dstName, _dstDataType, 1));
    } else {
        specs->push_back(HdBufferSpec(_dstName, _dstDataType, 3));
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

