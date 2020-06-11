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

bool UsdNprHalfEdge::GetFacing(const GfVec3f* positions, const GfVec3f& v) const
{
  GfVec3f tn;
  GetTriangleNormal(positions, tn);
  GfVec3f dir = (positions[vertex] - v).GetNormalized();
  return GfDot(tn, dir) > 0.0;
}

bool UsdNprHalfEdge::GetFacing(const GfVec3f* positions, const GfVec3f* normals, 
  const GfVec3f& v) const
{
  GfVec3f dir = (positions[vertex] - v).GetNormalized();
  return GfDot(normals[GetTriangleIndex()], dir) > 0.0;
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
  const GfVec3f& v, float creaseValue) const
{
  short flags = 0;
  if(!twin)return flags | EDGE_BOUNDARY;

  if(twin->GetTriangleIndex() < GetTriangleIndex()) return flags | EDGE_TWIN;

  bool s1 = GetFacing(positions, normals, v);
  bool s2 = twin->GetFacing(positions, normals, v);
  if(s1 != s2) flags |= EDGE_SILHOUETTE;

  if(creaseValue >= 0.0) {
    GfVec3f tn1, tn2;
    /*
    GetTriangleNormal(positions, tn1);
    twin->GetTriangleNormal(positions, tn2);
    */
    if(GfAbs(GfDot(normals[GetTriangleIndex()], normals[twin->GetTriangleIndex()])) < creaseValue)
      flags |= EDGE_CREASE;
  }

  return flags;
}

UsdNprHalfEdgeMesh::UsdNprHalfEdgeMesh(const SdfPath& path, char varyingBits)
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
  /*
  UsdNprComputeVertexNormals(
    _positions,
    faceVertexCounts,
    faceVertexIndices,
    samples,
    _normals
  );
  */
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
      return (A * 0.99 + V * 0.01) - normal * width;
    case 1:
      return (B * 0.99 + V * 0.01) - normal * width;
    case 2:
      return (B * 0.99 + V * 0.01) + normal * width;
    case 3:
      return (A * 0.99 + V * 0.01) + normal * width;
  };
}

void UsdNprHalfEdgeMesh::ComputeOutputGeometry(const std::vector<const UsdNprHalfEdge*>& silhouettes,
  const GfVec3f& viewPoint, VtArray<GfVec3f>& points, VtArray<int>& faceVertexCounts,
  VtArray<int>& faceVertexIndices)
{
  size_t numEdges = silhouettes.size();// + _creases.size() + _boundaries.size();
  size_t numPoints = numEdges * 4;

  // topology
  faceVertexCounts.resize(numEdges);
  for(int i=0;i<numEdges;++i)faceVertexCounts[i] = 4;
  faceVertexIndices.resize(numPoints);
  for(int i=0;i<numPoints;++i)faceVertexIndices[i] = i;

  // points
  points.resize(numPoints);
  size_t index = 0;
  float width = 0.04;
  
  for(const auto& halfEdge: silhouettes)
  {
    const GfVec3f* positions = &_positions[0];
    const GfVec3f& A = _xform.Transform(positions[halfEdge->vertex]);
    const GfVec3f& B = _xform.Transform(positions[halfEdge->twin->vertex]);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 0);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 1);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 2);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 3);
  }
  
  /*
  for(const auto& halfEdge: _boundaries)
  {
    const GfVec3f* positions = _halfEdgeMesh->GetPositionsPtr();
    const GfVec3f& A = _meshXform.Transform(positions[halfEdge->vertex]);
    const GfVec3f& B = _meshXform.Transform(positions[halfEdge->next->vertex]);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 0);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 1);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 2);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 3);
  }

  for(const auto& halfEdge: _creases)
  {
    const GfVec3f* positions = _halfEdgeMesh->GetPositionsPtr();
    const GfVec3f& A = _meshXform.Transform(positions[halfEdge->vertex]);
    const GfVec3f& B = _meshXform.Transform(positions[halfEdge->twin->vertex]);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 0);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 1);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 2);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 3);
  }
  */
}

PXR_NAMESPACE_CLOSE_SCOPE
