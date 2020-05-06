//
// Copyright 2020 benmalartre
//
// Original code from 
// https://prideout.net/blog/old/blog/index.html@p=54.html
// 
//
#include "pxr/usdImaging/usdNprImaging/halfEdge.h"
#include "pxr/usdImaging/usdNprImaging/utils.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

void UsdNprHalfEdge::GetTriangleNormal(const GfVec3f* positions, GfVec3f& normal) const
{
  GfVec3f ab = positions[vertex] - positions[next->vertex];
  GfVec3f ac = positions[vertex] - positions[next->next->vertex];
  normal = (ab ^ ac).GetNormalized();
}

void UsdNprHalfEdgeMesh::Compute(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode)
{
  UsdAttribute pointsAttr = mesh.GetPointsAttr();
  UsdAttribute faceVertexCountsAttr = mesh.GetFaceVertexCountsAttr();
  UsdAttribute faceVertexIndicesAttr = mesh.GetFaceVertexIndicesAttr();

  pointsAttr.Get(&_positions, timeCode);
  VtArray<int> faceVertexCounts;
  VtArray<int> faceVertexIndices;
  faceVertexCountsAttr.Get(&faceVertexCounts, timeCode);
  faceVertexIndicesAttr.Get(&faceVertexIndices, timeCode);

  VtArray<GfVec3i> samples;
  UsdNprTriangulateMesh(faceVertexCounts, 
                        faceVertexIndices, 
                        samples);

  UsdNprComputeVertexNormals(
    _positions,
    faceVertexCounts,
    faceVertexIndices,
    samples,
    _normals
  );

  _numTriangles = samples.size() / 3;
  
  _halfEdges.resize(_numTriangles * 3);

  TfHashMap<uint64_t, UsdNprHalfEdge*, TfHash> halfEdgesMap;

  const GfVec3i* sample = &samples[0];
  UsdNprHalfEdge* halfEdge = &_halfEdges[0];
  for (int triIndex = 0; triIndex < _numTriangles; ++triIndex)
  {
      uint64_t A = sample[0][0];sample++;
      uint64_t B = sample[0][0];sample++;
      uint64_t C = sample[0][0];sample++;

      // create the half-edge that goes from C to A:
      halfEdgesMap[A | (C << 32)] = halfEdge;
      halfEdge->vertex = C;
      halfEdge->triangle = triIndex;
      halfEdge->next = 1 + halfEdge;
      ++halfEdge;

      // create the half-edge that goes from A to B:
      halfEdgesMap[B | (A << 32)] = halfEdge;
      halfEdge->vertex = A;
      halfEdge->triangle = triIndex;
      halfEdge->next = 1 + halfEdge;
      ++halfEdge;

      // create the half-edge that goes from B to C:
      halfEdgesMap[C | (B << 32)] = halfEdge;
      halfEdge->vertex = B;
      halfEdge->triangle = triIndex;
      halfEdge->next = halfEdge - 2;
      ++halfEdge;
  }

  // verify that the mesh is clean:
  size_t numEntries = halfEdgesMap.size();
  bool problematic = false;
  if(numEntries != _numTriangles * 3)problematic = true;

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
  std::cout << "BOUNDARIES : " << boundaryCount << std::endl;
}

void UsdNprHalfEdgeMesh::Update(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode) 
{
  UsdAttribute pointsAttr = mesh.GetPointsAttr();
  UsdAttribute faceVertexCountsAttr = mesh.GetFaceVertexCountsAttr();
  UsdAttribute faceVertexIndicesAttr = mesh.GetFaceVertexIndicesAttr();

  pointsAttr.Get(&_positions, timeCode);
  VtArray<int> faceVertexCounts;
  VtArray<int> faceVertexIndices;
  faceVertexCountsAttr.Get(&faceVertexCounts, timeCode);
  faceVertexIndicesAttr.Get(&faceVertexIndices, timeCode);

  VtArray<GfVec3i> samples;
  UsdNprTriangulateMesh(faceVertexCounts, 
                        faceVertexIndices, 
                        samples);

  UsdNprComputeVertexNormals(
    _positions,
    faceVertexCounts,
    faceVertexIndices,
    samples,
    _normals
  );
}

PXR_NAMESPACE_CLOSE_SCOPE
