//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_USD_IMAGING_USD_NPR_IMAGING_HALFEDGE_H
#define PXR_USD_IMAGING_USD_NPR_IMAGING_HALFEDGE_H

#include "pxr/pxr.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

enum UsdHalfEdgeMeshVaryingBits {
  VARYING_TOPOLOGY = 1,
  VARYING_DEFORM = 2,
  VARYING_TRANSFORM = 4,
  VARYING_VISIBILITY = 8
};

struct UsdNprHalfEdge
{
  uint32_t                vertex;    // vertex index
  uint32_t                triangle;  // triangle index
  struct UsdNprHalfEdge*  twin;      // opposite half-edge
  struct UsdNprHalfEdge*  next;      // next half-edge

  UsdNprHalfEdge():vertex(0),twin(NULL),next(NULL){};
  void GetTriangleNormal(const GfVec3f* positions, GfVec3f& normal) const;
};

/// \class UsdNprHalfEdgeMesh
///
class UsdNprHalfEdgeMesh
{
public:
  UsdNprHalfEdgeMesh(char varyingBits):_varyingBits(varyingBits){};
  void Compute(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode);
  void Update(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode);
  const std::vector<UsdNprHalfEdge>& GetHalfEdges() const {return _halfEdges;};

  const GfVec3f* GetPositionsPtr() const {return &_positions[0];};
  const GfVec3f* GetNormalsPtr() const {return &_normals[0];};
  size_t GetNumPoints() const {return _positions.size();};
  size_t GetNumTriangles() const {return _numTriangles;};
  size_t GetNumHalfEdges() const {return _halfEdges.size();};

  // varying
  bool IsVarying(){return _varyingBits != 0;};
  bool IsTopoVarying(){return _varyingBits & VARYING_TOPOLOGY;};
  bool IsDeformVarying(){return _varyingBits & VARYING_DEFORM;};
  bool IsTransformVarying(){return _varyingBits & VARYING_TRANSFORM;};
  bool IsVisibilityVarying(){return _varyingBits & VARYING_VISIBILITY;};
  char GetVaryingBits(){return _varyingBits;};

private:
  std::vector<UsdNprHalfEdge> _halfEdges; 
  VtArray<GfVec3f>            _positions;
  VtArray<GfVec3f>            _normals;
  size_t                      _numTriangles;
  char                        _varyingBits;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_NPR_IMAGING_HALFEDGE_H
