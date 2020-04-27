//
// Copyright 2020 benmalartre
//
// Original code from 
// https://prideout.net/blog/old/blog/index.html@p=54.html
// 
//
#include "pxr/imaging/plugin/LoFi/halfedge.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

void LoFiAdjacency::Compute(const VtArray<GfVec3i>& samples)
{
  size_t numTriangles = samples.size() / 3;
  std::vector<LoFiHalfEdge> halfEdges;
  halfEdges.resize(numTriangles * 3);
  _adjacency.resize(numTriangles * 6);
  TfHashMap<uint64_t, LoFiHalfEdge*, TfHash> halfEdgesMap;

  const GfVec3i* sample = &samples[0];
  LoFiHalfEdge* halfEdge = &halfEdges[0];
  for (int triIndex = 0; triIndex < numTriangles; ++triIndex)
  {
      uint64_t A = sample[0][0];
      uint32_t sA = triIndex*3;sample++;
      uint64_t B = sample[0][0];
      uint32_t sB = triIndex*3+1;sample++;
      uint64_t C = sample[0][0];
      uint32_t sC = triIndex*3+2;sample++;

      // create the half-edge that goes from C to A:
      halfEdgesMap[C | (A << 32)] = halfEdge;
      halfEdge->vertex = A;
      halfEdge->sample = sA;
      halfEdge->next = 1 + halfEdge;
      ++halfEdge;

      // create the half-edge that goes from A to B:
      halfEdgesMap[A | (B << 32)] = halfEdge;
      halfEdge->vertex = B;
      halfEdge->sample = sB;
      halfEdge->next = 1 + halfEdge;
      ++halfEdge;

      // create the half-edge that goes from B to C:
      halfEdgesMap[B | (C << 32)] = halfEdge;
      halfEdge->vertex = C;
      halfEdge->sample = sC;
      halfEdge->next = halfEdge - 2;
      ++halfEdge;
  }

  // verify that the mesh is clean:
  size_t numEntries = halfEdgesMap.size();
  if(numEntries != numTriangles * 3)
  {
    std::cout << "Bad Mesh: Duplicated edges or inconsistent winding!!!" << std::endl;
    _valid = false;
    return;
  }

  // populate the twin pointers by iterating over the hash map:
  uint64_t edgeIndex; 
  size_t boundaryCount = 0;
  for(auto& halfEdge: halfEdgesMap)
  {
    edgeIndex = halfEdge.first;
    uint64_t twinIndex = ((edgeIndex & 0xffffffff) << 32) | (edgeIndex >> 32);
    auto& twinEdge = halfEdgesMap.find(twinIndex);
    if(twinEdge != halfEdgesMap.end())
    {
      twinEdge->second->twin = halfEdge.second;
      halfEdge.second->twin = twinEdge->second;
    }
    else ++boundaryCount;
  }

  // now that we have a half-edge structure, it's easy to create adjacency info for OpenGL:
  if (boundaryCount > 0)
  {
      int* adjacency = &_adjacency[0];
      LoFiHalfEdge* halfEdge = &halfEdges[0];
      for (int triIndex = 0; triIndex < numTriangles; ++triIndex, halfEdge += 3, adjacency += 6)
      {
          adjacency[0] = halfEdge[2].sample;
          adjacency[1] = halfEdge[0].twin ? (halfEdge[0].twin->next->sample) : adjacency[0];
          adjacency[2] = halfEdge[0].sample;
          adjacency[3] = halfEdge[1].twin ? (halfEdge[1].twin->next->sample) : adjacency[1];
          adjacency[4] = halfEdge[1].sample;
          adjacency[5] = halfEdge[2].twin ? (halfEdge[2].twin->next->sample) : adjacency[2];
      }
  }
  else
  {
      int* adjacency = &_adjacency[0];
      LoFiHalfEdge* halfEdge = &halfEdges[0];
      for (int triIndex = 0; triIndex < numTriangles; ++triIndex, halfEdge += 3, adjacency += 6)
      {
          adjacency[0] = halfEdge[2].sample;
          adjacency[1] = halfEdge[0].twin->next->sample;
          adjacency[2] = halfEdge[0].sample;
          adjacency[3] = halfEdge[1].twin->next->sample;
          adjacency[4] = halfEdge[1].sample;
          adjacency[5] = halfEdge[2].twin->next->sample;
      }
  }
  _valid = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
