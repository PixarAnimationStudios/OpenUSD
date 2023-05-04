//
// Copyright 2023 Pixar
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
#include "pxr/imaging/hdSt/vertexAdjacency.h"

#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_VertexAdjacencyBuilder::HdSt_VertexAdjacencyBuilder()
    : _vertexAdjacency()
    , _vertexAdjacencyRange()
    , _sharedVertexAdjacencyBuilder()
{
}

HdSt_VertexAdjacencyBuilder::~HdSt_VertexAdjacencyBuilder() = default;

HdBufferSourceSharedPtr
HdSt_VertexAdjacencyBuilder::GetSharedVertexAdjacencyBuilderComputation(
    HdMeshTopology const *topology)
{
    // if there's a already requested (and unresolved) adjacency computation,
    // just returns it to make a dependency.
    if (HdSt_VertexAdjacencyBuilderComputationSharedPtr builder =
        _sharedVertexAdjacencyBuilder.lock()) {

        return builder;
    }

    // if cpu adjacency table exists, no need to compute again
    if (!(_vertexAdjacency.GetAdjacencyTable().empty())) {
        return nullptr;
    }

    HdSt_VertexAdjacencyBuilderComputationSharedPtr builder =
        std::make_shared<HdSt_VertexAdjacencyBuilderComputation>(
            &_vertexAdjacency, topology);

    // store the computation as weak ptr so that it can be referenced
    // by another computation.
    _sharedVertexAdjacencyBuilder = builder;

    return builder;
}

// ---------------------------------------------------------------------------

HdSt_VertexAdjacencyBuilderComputation::HdSt_VertexAdjacencyBuilderComputation(
    Hd_VertexAdjacency *vertexAdjacency,
    HdMeshTopology const *topology)
    : _vertexAdjacency(vertexAdjacency)
    , _topology(topology)
{
}

bool
HdSt_VertexAdjacencyBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _vertexAdjacency->BuildAdjacencyTable(_topology);

    // call base class to mark as resolved.
    _SetResolved();
    return true;
}

bool
HdSt_VertexAdjacencyBuilderComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------

HdSt_VertexAdjacencyBufferSource::HdSt_VertexAdjacencyBufferSource(
    Hd_VertexAdjacency const *vertexAdjacency,
    HdBufferSourceSharedPtr const &vertexAdjacencyBuilder)
    : _vertexAdjacency(vertexAdjacency)
    , _vertexAdjacencyBuilder(vertexAdjacencyBuilder)
{
}

bool
HdSt_VertexAdjacencyBufferSource::Resolve()
{
    if (!_vertexAdjacencyBuilder->IsResolved()) return false;
    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // prepare buffer source to be transferred.
    VtIntArray const &vertexAdjacency = _vertexAdjacency->GetAdjacencyTable();
    _SetResult(std::make_shared<HdVtBufferSource>(
                        HdTokens->adjacency, VtValue(vertexAdjacency)));
    _SetResolved();
    return true;
}

void
HdSt_VertexAdjacencyBufferSource::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->emplace_back(HdTokens->adjacency, HdTupleType{HdTypeInt32, 1});
}

bool
HdSt_VertexAdjacencyBufferSource::_CheckValid() const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

