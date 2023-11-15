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
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/meshletSplit.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"


#include "pxr/base/gf/vec3i.h"

PXR_NAMESPACE_OPEN_SCOPE

constexpr uint32_t max_primitives = 512;
constexpr uint32_t max_vertices = 256;

struct VertexInfo {
    uint32_t vertexId;
    uint32_t indexId; // this is a promise that of the primitive associated, this is the index row associated
    uint32_t GetDisplacementIndex() {
        return indexId % 3;
    }
    uint32_t GetPrimitiveID() {
        return indexId / 3;
    }
};

struct Meshlet {
    uint32_t vertexCount;
    uint32_t primitiveCount;
    std::vector<VertexInfo> vertexInfo;
    std::vector<uint32_t> remappedIndices;
    std::vector<uint32_t> remappedPrimIDs;
};

struct MeshletCoord
{
    uint32_t meshlet_coord;
    uint32_t numMeshlets;
};
//has the coords for the meshlet at the start of this buffer
void flattenMeshlets(std::vector<uint32_t> &flattenInto, const std::vector<std::vector<Meshlet>> &meshlets) {
    uint32_t currentOffset = 0;
    uint32_t lastOffset = 0;
    for(int i = 0; i < meshlets.size(); i++) {
        int localOffset = 0;
        const std::vector<Meshlet> &meshletsInMesh = meshlets[i];
        flattenInto.push_back(meshletsInMesh.size());
        currentOffset++;
        localOffset++;
        for(int l = 0; l < meshletsInMesh.size(); l++) {
            flattenInto.push_back(meshletsInMesh.size());
            currentOffset++;
            localOffset++;
        }
        for(int j = 0; j < meshletsInMesh.size(); j++) {
            const Meshlet &m = meshletsInMesh[j];
            flattenInto.push_back(m.vertexCount);
            flattenInto.push_back(m.primitiveCount);
            currentOffset += 2;
            localOffset += 2;
            for (uint32_t k = 0; k < m.vertexInfo.size(); k++) {
                auto vertexInfo = m.vertexInfo[k];
                flattenInto.push_back(vertexInfo.vertexId);
                flattenInto.push_back(vertexInfo.indexId);
                currentOffset += 2;
                localOffset += 2;
            }
            
            for (int k = 0; k < m.remappedIndices.size(); k++) {
                flattenInto.push_back(m.remappedIndices[k]);
                flattenInto.push_back(m.remappedPrimIDs[k]);

                currentOffset += 2;
                localOffset += 2;
            }
            if (j < (meshletsInMesh.size()-1)) {
                flattenInto[lastOffset+(j+1)] = localOffset;
            }
        }
    }
}

//process mesh at a time
std::vector<Meshlet> processIndices(const uint32_t* indices, int indexCount, uint32_t meshStartLocation = 0, uint32_t meshEndLocation = 0) {
    int numPrimsProcessed = 0;
    int numVerticesProcessed = 0;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m;
    std::vector<Meshlet> meshlets;
    std::unordered_map<uint32_t, uint32_t> globalToLocalVertex;
    bool maxVertsReached = false;
    bool maxPrimsReached = false;
    Meshlet meshlet;
    int startPrimitive = meshStartLocation / 3;
    int endPrimitive = meshStartLocation / 3;
    uint primId = 0;
    for(uint32_t i = meshStartLocation; i < meshEndLocation + 1; i += 3) {
        if (maxVertsReached || maxPrimsReached || i >= meshEndLocation) {
            endPrimitive = numPrimsProcessed + startPrimitive;
            int count = 0;
            meshlet.vertexInfo.resize(m.size());
            for(auto it = m.begin(); it != m.end(); ++it) {
                VertexInfo vertexInfo;
                vertexInfo.vertexId = it->first;
                vertexInfo.indexId = it->second[0];
                meshlet.vertexInfo[count] = vertexInfo;
                globalToLocalVertex[it->first] = count;
                count++;
            }
            
            for(uint32_t n = startPrimitive; n < (endPrimitive); n++) {
                uint ind0 = globalToLocalVertex[indices[n*3]];
                uint ind1 = globalToLocalVertex[indices[n*3+1]];
                uint ind2 = globalToLocalVertex[indices[n*3+2]];
                uint packed = ind0;
                packed |= ind1 << 8;
                packed |= ind2 << 16;
                meshlet.remappedIndices.push_back(packed);
                meshlet.remappedPrimIDs.push_back(primId);
                primId++;
            }
            meshlet.vertexCount = meshlet.vertexInfo.size();
            meshlet.primitiveCount = numPrimsProcessed;

            maxVertsReached = false;
            maxPrimsReached = false;
            numPrimsProcessed = 0;
            numVerticesProcessed = 0;
            m = std::unordered_map<uint32_t, std::vector<uint32_t>>();
            globalToLocalVertex = std::unordered_map<uint32_t, uint32_t>();
            meshlets.push_back(meshlet);
            if(i >= meshEndLocation) {
                continue;
            }
            meshlet = Meshlet();
            startPrimitive = endPrimitive;
        }
        
        //TODO see if we can be smarter
        auto it1 = m.find(i);
        auto it2 = m.find(i+1);
        auto it3 = m.find(i+2);
        if((m.size() + 4) > max_vertices) {
            int space = max_vertices - m.size();
            int wantedSpace = 0;
            wantedSpace += it1 == m.end();
            wantedSpace += it2 == m.end();
            wantedSpace += it3 == m.end();
            if (space < wantedSpace) {
                maxVertsReached = true;
                i = i-3;
                continue;
            }
        }
        
        if (it1 == m.end()) {
            m[indices[i]] = {i};
            numVerticesProcessed++;
        } else {
            it1->second.push_back(i);
        }
        if (it2 == m.end()) {
            m[indices[i+1]] = {i+1};
            numVerticesProcessed++;
        } else {
            it2->second.push_back(i+1);
        }
        if (it3 == m.end()) {
            m[indices[i+2]] = {i+2};
            numVerticesProcessed++;
        } else {
            it3->second.push_back(i+2);
        }
        numPrimsProcessed++;
        if(numPrimsProcessed >= (max_primitives)) {
            maxPrimsReached = true;
            continue;
        }
    }
    return meshlets;
}

HdSt_MeshletSplitBuilderComputation::HdSt_MeshletSplitBuilderComputation(
    HdSt_MeshTopology *topology, SdfPath const &id, HdBufferSourceSharedPtr indexBufferSource)
    : _id(id), _topology(topology), _indexBufferSource(indexBufferSource)
{
}

void
HdSt_MeshletSplitBuilderComputation::GetBufferSpecs(
    HdBufferSpecVector *specs) const
{
    specs->emplace_back(HdTokens->meshlets,
                        HdTupleType{HdTypeInt32, 1});
}

bool
HdSt_MeshletSplitBuilderComputation::Resolve()
{
    if (!_TryLock()) return false;
    //Make sure these sources are solved
    //Should be triangle index builder computation
    if (!_indexBufferSource->IsResolved()) return false;
    HD_TRACE_FUNCTION();
    const uint32_t* data = reinterpret_cast<const uint32_t*>(_indexBufferSource->GetData());
    auto meshlets = processIndices(data, _indexBufferSource->GetNumElements());
    std::vector<uint32_t> flattenInto;
    flattenMeshlets(flattenInto, {meshlets});
    VtIntArray meshletData;
    meshletData.resize(flattenInto.size());
    for (int i = 0; i < flattenInto.size(); i++) {
        meshletData[i] = flattenInto[i];
    }
    _SetResolved();
    return true;
}

bool
HdSt_MeshletSplitBuilderComputation::HasChainedBuffer() const
{
    return false;
}

HdBufferSourceSharedPtrVector
HdSt_MeshletSplitBuilderComputation::GetChainedBuffers() const
{
    return {};
    //return { _primitiveParam, _trianglesEdgeIndices };
}

bool
HdSt_MeshletSplitBuilderComputation::_CheckValid() const
{
    return true;
}

// ---------------------------------------------------------------------------
/*
HdSt_MeshletSplitComputation::HdSt_MeshletSplitComputation(
    HdSt_MeshTopology *topology,
    HdBufferSourceSharedPtr const &source,
    SdfPath const &id)
    : _id(id), _topology(topology), _source(source)
{
}

bool
HdSt_MeshletSplitComputation::Resolve()
{
    if (!TF_VERIFY(_source)) return false;
    if (!_source->IsResolved()) return false;

    if (!_TryLock()) return false;

    HD_TRACE_FUNCTION();
    //HD_PERF_COUNTER_INCR(HdPerfTokens->triangulateFaceVarying);

    VtValue result;
    HdMeshUtil meshUtil(_topology, _id);
    if(meshUtil.ComputeTriangulatedFaceVaryingPrimvar(
            _source->GetData(),
            _source->GetNumElements(),
            _source->GetTupleType().type,
            &result)) {
        _SetResult(std::make_shared<HdVtBufferSource>(
                        _source->GetName(),
                        result));
    } else {
        _SetResult(_source);
    }

    _SetResolved();
    return true;
}

void
HdSt_TriangulateFaceVaryingComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // produces same spec buffer as source
    _source->GetBufferSpecs(specs);
}

bool
HdSt_TriangulateFaceVaryingComputation::_CheckValid() const
{
    return (_source->IsValid());
}
*/
PXR_NAMESPACE_CLOSE_SCOPE

