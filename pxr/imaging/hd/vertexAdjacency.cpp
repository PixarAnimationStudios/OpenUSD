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
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE


Hd_VertexAdjacency::Hd_VertexAdjacency()
    : _numPoints(0)
    , _adjacencyTable()
{
}

Hd_VertexAdjacency::~Hd_VertexAdjacency()
{
    HD_PERF_COUNTER_SUBTRACT(HdPerfTokens->adjacencyBufSize,
                             _adjacencyTable.size() * sizeof(int));
}

void
Hd_VertexAdjacency::BuildAdjacencyTable(HdMeshTopology const *topology)
{
    // compute adjacency

    int const * numVertsPtr = topology->GetFaceVertexCounts().cdata();
    int const * vertsPtr = topology->GetFaceVertexIndices().cdata();
    const int numFaces = topology->GetFaceVertexCounts().size();
    bool flip = (topology->GetOrientation() != HdTokens->rightHanded);

    if (numFaces > 0 && !vertsPtr) {
        TF_WARN("Topology missing face vertex indices.");
        _numPoints = 0;
        _adjacencyTable.clear();
        return;
    }

    // compute numPoints from topology indices
    _numPoints = topology->GetNumPoints();

    // Track the number of entries needed the adjacency table.
    // We start by needing 2 per point (offset and valence).
    size_t numEntries = _numPoints * 2;


    // Compute the size of each entry, so we can work out the offsets.
    std::vector<int> vertexValence(_numPoints);


    int vertIndex = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        for (int j=0; j<nv; ++j) {
            int index = vertsPtr[vertIndex++];
            if (index < 0 || index >= _numPoints) {
                TF_WARN("vertex index out of range "
                        "index: %d numPoints: %d", index, _numPoints);
                _numPoints = 0;
                _adjacencyTable.clear();
                return;
            }
            ++vertexValence[index];
        }

        // Increase the number of entries needed by 2 (prev & next index).
        // for every vertex in the face.
        numEntries += 2 * nv;
    }


    // Each entry is a count followed by pairs of adjacent vertex indices.
    // We use a uniform entry size for all vertices, this allows faster
    // lookups at the cost of some additional memory.
    HD_PERF_COUNTER_SUBTRACT(HdPerfTokens->adjacencyBufSize,
                                          _adjacencyTable.size() * sizeof(int));


    _adjacencyTable.clear();
    _adjacencyTable.resize(numEntries);
    HD_PERF_COUNTER_ADD(HdPerfTokens->adjacencyBufSize,
                                                      numEntries * sizeof(int));

    // Fill out first part of buffer with offsets.  Even though we
    // know counts, don't fill them out now, so we know how many indices
    // we've written so far.
    int currentOffset = _numPoints * 2;
    for (int pointNum = 0; pointNum < _numPoints; ++pointNum) {
        _adjacencyTable[pointNum * 2] = currentOffset;
        currentOffset += 2*vertexValence[pointNum];
    }

    vertIndex = 0;
    for (int i=0; i<numFaces; ++i) {
        int nv = numVertsPtr[i];
        for (int j=0; j<nv; ++j) {
            int prev = vertsPtr[vertIndex+(j+nv-1)%nv];
            int curr = vertsPtr[vertIndex+j];
            int next = vertsPtr[vertIndex+(j+1)%nv];
            if (flip) std::swap(prev, next);

            int entryOffset = _adjacencyTable[curr * 2 + 0];
            int &entryCount = _adjacencyTable[curr * 2 + 1];

            int pairOffset = entryOffset + entryCount * 2;
            ++entryCount;

            _adjacencyTable[pairOffset + 0] = prev;
            _adjacencyTable[pairOffset + 1] = next;
        }
        vertIndex += nv;
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

