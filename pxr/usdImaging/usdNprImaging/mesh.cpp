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

void UsdNprHalfEdge::GetPolygonNormal(const GfVec3f* positions, 
  GfVec3f& normal) const
{
  normal = GfVec3f(0.f);
  UsdNprHalfEdge* currentEdge = (UsdNprHalfEdge*)this;
  UsdNprHalfEdge* nextEdge = (UsdNprHalfEdge*)this->next;
  size_t numEdges = 0;
  while(nextEdge != this) {
    GfVec3f ab = positions[currentEdge->vertex] - positions[nextEdge->vertex];
    GfVec3f ac = positions[currentEdge->vertex] - positions[nextEdge->next->vertex];
    normal += (ab ^ ac).GetNormalized();
    numEdges ++;
    currentEdge = nextEdge;
    nextEdge = currentEdge->next;
  }
  normal *= 1.f / static_cast<float>(numEdges);
}

bool UsdNprHalfEdge::GetVertexFacing(const GfVec3f* positions, 
  const GfVec3f* vertexNormals,const GfVec3f& viewPoint, float* weight) const
{
  GfVec3f dir = (positions[vertex] - viewPoint).GetNormalized();
  *weight = GfDot(vertexNormals[vertex], dir);
  return (*weight > 0.0);
}

bool UsdNprHalfEdge::GetFacing(const GfVec3f* positions, 
  const GfVec3f* polygonNormals, const GfVec3f& viewPoint, float* weight) const
{
  GfVec3f dir = (positions[vertex] - viewPoint).GetNormalized();
  *weight = GfDot(polygonNormals[GetPolygonIndex()], dir);
  return (*weight > 0.0);
}

float UsdNprHalfEdge::GetDot(const GfVec3f* positions, const GfVec3f* normals,
  const GfVec3f& viewPoint) const
{
  GfVec3f dir = (positions[vertex] - viewPoint).GetNormalized();
  return GfDot(normals[GetPolygonIndex()], dir);
}

short UsdNprHalfEdge::GetFlags(const GfVec3f* positions, 
  const GfVec3f* vertexNormals, const GfVec3f* polygonNormals, 
  const GfVec3f& viewPoint, float creaseValue, float* weight) const
{
  short flags = 0;
  if(!twin)return flags | EDGE_BOUNDARY;

  if(twin->GetPolygonIndex() < GetPolygonIndex()) return flags | EDGE_TWIN;

  float weight1, weight2;
  bool s1 = GetVertexFacing(positions, vertexNormals, viewPoint, &weight1);
  bool s2 = twin->GetVertexFacing(positions, vertexNormals, viewPoint, &weight2);

  if(s1 != s2) {
    flags |= EDGE_SILHOUETTE;
    *weight = 1.f - (
      std::fabsf(weight1) / (std::fabsf(weight1) + std::fabsf(weight2))
    );
  }
  
  if(creaseValue >= 0.0) {
    if(GfAbs(GfDot(polygonNormals[GetPolygonIndex()], 
                   polygonNormals[twin->GetPolygonIndex()])) < 1.f - creaseValue)
      flags |= EDGE_CREASE;
  }
  return flags;
}

void UsdNprHalfEdge::GetWeightedPositionAndNormal(const GfVec3f* positions, 
  const GfVec3f* vertexNormals, float weight, GfVec3f& position, GfVec3f& normal)
{
  position = positions[vertex] * weight +
    positions[next->vertex] * (1.f - weight);
  normal = vertexNormals[vertex] * weight + 
    vertexNormals[next->vertex]  * (1.f - weight);
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
  VtArray<int> triangles;

  UsdNprTriangulateMesh(faceVertexCounts, 
                        faceVertexIndices, 
                        triangles);

  // compute vertex and polygon normals  
  UsdNprComputeNormals(
    _positions,
    faceVertexCounts,
    faceVertexIndices,
    triangles,
    _polygonNormals,
    _vertexNormals
  );

  _numTriangles = triangles.size() / 3;
  _numPolygons = faceVertexCounts.size();
  
  _halfEdges.resize(faceVertexIndices.size());

  TfHashMap<uint64_t, UsdNprHalfEdge*, TfHash> halfEdgesMap;

  UsdNprHalfEdge* halfEdge = &_halfEdges[0];
  const int* indices = &faceVertexIndices[0];
  size_t halfEdgeIndex = 0;

  // for each face build half-edges and insert in map
  for(int faceIndex = 0; faceIndex < _numPolygons; ++ faceIndex) {
    size_t numFaceVertices = faceVertexCounts[faceIndex];
    for(int faceVertexIndex = 0; faceVertexIndex < numFaceVertices; ++faceVertexIndex) {
      uint64_t p0 = indices[faceVertexIndex];
      uint64_t p1 = indices[(faceVertexIndex + 1) % numFaceVertices];
      halfEdgesMap[p1 | (p0 << 32)] = halfEdge;
      halfEdge->index = halfEdgeIndex++;
      halfEdge->vertex = p0;
      halfEdge->polygon = faceIndex;
      
      halfEdge->next = faceVertexIndex < (numFaceVertices - 1) ? 
          halfEdge + 1 : halfEdge - (numFaceVertices - 1);

      ++halfEdge;
    }

    indices += numFaceVertices;
  }

  // verify that the mesh is clean:
  size_t numEntries = halfEdgesMap.size();
  bool problematic = false;
  if(numEntries != faceVertexIndices.size())problematic = true;

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
  VtArray<int> triangles;

  UsdNprTriangulateMesh(faceVertexCounts, 
                        faceVertexIndices, 
                        triangles);

  //UsdNprComputeTriangleNormals(_positions, samples, _normals);
  UsdNprComputeNormals(
    _positions,
    faceVertexCounts,
    faceVertexIndices,
    triangles,
    _polygonNormals,
    _vertexNormals
  );
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
