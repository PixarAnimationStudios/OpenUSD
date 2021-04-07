//
// Copyright 2020 benmalartre
//
// Original code from 
// https://prideout.net/blog/old/blog/index.html@p=54.html
// 
//
#include "pxr/usdImaging/usdNprImaging/mesh.h"
#include "pxr/usdImaging/usdNprImaging/utils.h"
#include "pxr/usdImaging/usdNprImaging/stroke.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// convert varying bits
static char _ConvertVaryingBits(const HdDirtyBits& varyingBits)
{
  char outVaryingBits = 0;
  if(varyingBits & HdChangeTracker::DirtyTopology)
  {
    outVaryingBits |= UsdHalfEdgeMeshVaryingBits::VARYING_TOPOLOGY;
  }
  if(varyingBits & HdChangeTracker::DirtyPoints)
  {
    outVaryingBits |= UsdHalfEdgeMeshVaryingBits::VARYING_DEFORM;
  }
  if(varyingBits & HdChangeTracker::DirtyTransform)
  {
    outVaryingBits |= UsdHalfEdgeMeshVaryingBits::VARYING_TRANSFORM;
  }
  if(varyingBits & HdChangeTracker::DirtyVisibility)
  {
    outVaryingBits |= UsdHalfEdgeMeshVaryingBits::VARYING_VISIBILITY;
  }
  return outVaryingBits;
}


void UsdNprHalfEdge::GetTriangleNormal(const GfVec3f* positions, GfVec3f& normal) const
{
  GfVec3f ab = positions[vertex] - positions[next->vertex];
  GfVec3f ac = positions[vertex] - positions[next->next->vertex];
  normal = (ab ^ ac).GetNormalized();
}

void UsdNprHalfEdge::GetVertexNormal(const GfVec3f* normals, GfVec3f& normal) const
{
  bool closed = false;
  normal = normals[GetTriangleIndex()];
  size_t numTriangles = 1;
  UsdNprHalfEdge* current = (UsdNprHalfEdge*)this;
  while(current->next->twin) {
    current = current->next->twin;
    if(current == this) {
      closed = true;
      break;
    }
    else {
      normal += normals[current->GetTriangleIndex()];
      numTriangles++;
    }
  }

  if(!closed && this->twin) {
    current = this->twin;
    normal += normals[current->GetTriangleIndex()];
    numTriangles++;
    while(current->next->next->twin) {
      current = current->next->next->twin;
      normal += normals[current->GetTriangleIndex()];
      numTriangles++;
    }
  }
  normal *= 1.f/(float)numTriangles;
}

bool UsdNprHalfEdge::GetVertexFacing(const GfVec3f* positions, 
  const GfVec3f* normals,const GfVec3f& v, float* weight) const
{
  GfVec3f vn;
  GetVertexNormal(normals, vn);
  GfVec3f dir = (positions[vertex] - v).GetNormalized();
  *weight = GfDot(vn, dir);
  return (*weight > 0.0);
}

bool UsdNprHalfEdge::GetFacing(const GfVec3f* positions, const GfVec3f& v,
  float* weight) const
{
  GfVec3f tn;
  GetTriangleNormal(positions, tn);
  GfVec3f dir = (positions[vertex] - v).GetNormalized();
  *weight = GfDot(tn, dir);
  return (*weight > 0.0);
}

bool UsdNprHalfEdge::GetFacing(const GfVec3f* positions, const GfVec3f* normals, 
  const GfVec3f& v, float* weight) const
{
  GfVec3f dir = (positions[vertex] - v).GetNormalized();
  *weight = GfDot(normals[GetTriangleIndex()], dir);
  return (*weight > 0.0);
}

float UsdNprHalfEdge::GetDot(const GfVec3f* positions, const GfVec3f* normals,
  const GfVec3f& v) const
{
  GfVec3f dir = (positions[vertex] - v).GetNormalized();
  return GfDot(normals[GetTriangleIndex()], dir);
}

/*
bool UsdNprHalfEdge::GetFacing(const GfVec3f* positions, const GfVec3f* normals,
  const GfVec3f& v) const
{
  GfVec3f dir = (positions[vertex] - v).GetNormalized();
  return GfDot(normals[vertex], dir) > 0.0;
}
*/

short UsdNprHalfEdge::GetFlags(const GfVec3f* positions, const GfVec3f* normals, 
  const GfVec3f& v, float creaseValue, float* weight) const
{
  short flags = 0;
  if(!twin)return flags | EDGE_BOUNDARY;

  if(twin->GetTriangleIndex() < GetTriangleIndex()) return flags | EDGE_TWIN;

  float weight1, weight2;
  bool s1 = GetVertexFacing(positions, normals, v, &weight1);
  bool s2 = twin->GetVertexFacing(positions, normals, v, &weight2);
  
  if(s1 != s2) {
    flags |= EDGE_SILHOUETTE;
    *weight = std::fabsf(weight1) / (std::fabsf(weight1) + std::fabsf(weight2));
  } else {
    *weight = 0.5f;
  }

  if(creaseValue >= 0.0) {
    GfVec3f tn1, tn2;
    if(GfAbs(GfDot(normals[GetTriangleIndex()], 
                   normals[twin->GetTriangleIndex()])) < creaseValue)
      flags |= EDGE_CREASE;
  }

  return flags;
}

void UsdNprHalfEdge::GetWeightedPositionAndNormal(const GfVec3f* positions, 
  const GfVec3f* normals, float weight, GfVec3f& position, GfVec3f& normal)
{
  position = positions[vertex] * weight +
    positions[next->vertex] * (1.f - weight);
  GfVec3f n1, n2;
  GetVertexNormal(normals, n1);
  next->GetVertexNormal(normals, n2);
  normal = n1 * weight + n2  * (1.f - weight);
}

UsdNprHalfEdgeMesh::UsdNprHalfEdgeMesh(const SdfPath& path, 
  const HdDirtyBits& varyingBits)
  :_varyingBits(_ConvertVaryingBits(varyingBits))
  , _sdfPath(path)
{
};
    

void UsdNprHalfEdgeMesh::Init(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode)
{
  UsdAttribute pointsAttr = mesh.GetPointsAttr();
  UsdAttribute faceVertexCountsAttr = mesh.GetFaceVertexCountsAttr();
  UsdAttribute faceVertexIndicesAttr = mesh.GetFaceVertexIndicesAttr();

  pointsAttr.Get(&_positions, timeCode);
  VtArray<int> faceVertexCounts;
  VtArray<int> faceVertexIndices;
  faceVertexCountsAttr.Get(&faceVertexCounts, timeCode);
  faceVertexIndicesAttr.Get(&faceVertexIndices, timeCode);
  VtArray<int> samples;

  UsdNprTriangulateMesh(faceVertexCounts, 
                        faceVertexIndices, 
                        samples);

  UsdNprComputeTriangleNormals(_positions, samples, _normals);
  /*
  UsdNprComputeVertexNormals(
    _positions,
    faceVertexCounts,
    faceVertexIndices,
    samples,
    _normals
  );
  */

  _numTriangles = samples.size() / 3;
  
  _halfEdges.resize(_numTriangles * 3);

  TfHashMap<uint64_t, UsdNprHalfEdge*, TfHash> halfEdgesMap;

  const int* sample = &samples[0];
  UsdNprHalfEdge* halfEdge = &_halfEdges[0];
  for (int triIndex = 0; triIndex < _numTriangles; ++triIndex)
  {
      uint64_t A = sample[0];sample++;
      uint64_t B = sample[0];sample++;
      uint64_t C = sample[0];sample++;

      // create the half-edge that goes from C to A:
      halfEdgesMap[A | (C << 32)] = halfEdge;
      halfEdge->index = triIndex * 3;
      halfEdge->vertex = C;
      halfEdge->next = 1 + halfEdge;
      ++halfEdge;

      // create the half-edge that goes from A to B:
      halfEdgesMap[B | (A << 32)] = halfEdge;
      halfEdge->index = triIndex * 3 + 1;
      halfEdge->vertex = A;
      halfEdge->next = 1 + halfEdge;
      ++halfEdge;

      // create the half-edge that goes from B to C:
      halfEdgesMap[C | (B << 32)] = halfEdge;
      halfEdge->index = triIndex * 3 + 2;
      halfEdge->vertex = B;
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
    const auto& it = halfEdgesMap.find(twinIndex);
    if(it != halfEdgesMap.end())
    {
      UsdNprHalfEdge* twinEdge = (UsdNprHalfEdge*)it->second;
      twinEdge->twin = halfEdge.second;
      halfEdge.second->twin = twinEdge;
    }
    else ++boundaryCount;
  }
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
  VtArray<int> samples;

  UsdNprTriangulateMesh(faceVertexCounts, 
                        faceVertexIndices, 
                        samples);

  UsdNprComputeTriangleNormals(_positions, samples, _normals);
}


static GfVec3f _RandomOffset(float x)
{
  return GfVec3f(
    (float)rand()/(float)RAND_MAX * x,
    (float)rand()/(float)RAND_MAX * x,
    (float)rand()/(float)RAND_MAX * x
  );
}

inline static GfVec3f _ComputePoint(const GfVec3f& A, const GfVec3f& B, const GfVec3f& V,
  float width, short index)
{

  const GfVec3f tangent = (B-A).GetNormalized();
  const GfVec3f dir = ((A+B)*0.5 - V).GetNormalized();
  const GfVec3f normal = (tangent ^ dir).GetNormalized();

  switch(index) {
    case 0:
      return A - normal * width;
    case 1:
      return B - normal * width;
    case 2:
      return B + normal * width;
    case 3:
      return A + normal * width;
  };
}

PXR_NAMESPACE_CLOSE_SCOPE
