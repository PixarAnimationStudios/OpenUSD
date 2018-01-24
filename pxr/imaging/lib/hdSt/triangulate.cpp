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

#include "pxr/imaging/hdSt/triangulate.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"


#include "pxr/base/gf/vec3i.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_TriangleIndexBuilderComputation::HdSt_TriangleIndexBuilderComputation(
    HdSt_MeshTopology *topology, SdfPath const &id)
    : _id(id), _topology(topology)
{
}

void
HdSt_TriangleIndexBuilderComputation::AddBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->emplace_back(HdTokens->indices, HdTupleType{HdTypeInt32Vec3, 1});
    // triangles don't support ptex indexing (at least for now).
    specs->emplace_back(HdTokens->primitiveParam,
                        HdTupleType{HdTypeInt32, 1});
}

bool
HdSt_TriangleIndexBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();

    VtVec3iArray trianglesFaceVertexIndices;
    VtIntArray primitiveParam;
    HdMeshUtil meshUtil(_topology, _id);
    meshUtil.ComputeTriangleIndices(
            &trianglesFaceVertexIndices,
            &primitiveParam);

    _SetResult(HdBufferSourceSharedPtr(
                   new HdVtBufferSource(
                       HdTokens->indices,
                       VtValue(trianglesFaceVertexIndices))));

    _primitiveParam.reset(new HdVtBufferSource(
                              HdTokens->primitiveParam,
                              VtValue(primitiveParam)));

    _SetResolved();
    return true;
}

bool
HdSt_TriangleIndexBuilderComputation::HasChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtr
HdSt_TriangleIndexBuilderComputation::GetChainedBuffer() const
{
    return _primitiveParam;
}

bool
HdSt_TriangleIndexBuilderComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

HdSt_TriangulateFaceVaryingComputation::HdSt_TriangulateFaceVaryingComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source,
    SdfPath const &id)
    : _id(id), _topology(topology), _source(source)
{
}

bool
HdSt_TriangulateFaceVaryingComputation::Resolve()
{
    if (!TF_VERIFY(_source)) return false;
    if (!_source->IsResolved()) return false;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HD_PERF_COUNTER_INCR(HdPerfTokens->triangulateFaceVarying);

    VtValue result;
    HdMeshUtil meshUtil(_topology, _id);
    if(meshUtil.ComputeTriangulatedFaceVaryingPrimvar(
            _source->GetData(),
            _source->GetNumElements(),
            _source->GetTupleType().type,
            &result)) {
        _SetResult(HdBufferSourceSharedPtr(
                    new HdVtBufferSource(
                        _source->GetName(),
                        result)));
    } else {
        _SetResult(_source);
    }

    _SetResolved();
    return true;
}

void
HdSt_TriangulateFaceVaryingComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same spec buffer as source
    _source->AddBufferSpecs(specs);
}

bool
HdSt_TriangulateFaceVaryingComputation::_CheckValid() const
{
    return (_source->IsValid());
}

PXR_NAMESPACE_CLOSE_SCOPE

