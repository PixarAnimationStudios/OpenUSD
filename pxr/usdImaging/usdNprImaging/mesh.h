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
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/meshTopology.h"

#include <vector>
#include <memory>


PXR_NAMESPACE_OPEN_SCOPE

struct UsdNprStrokeNode;
struct UsdNprStrokeParams;
struct UsdNprEdgeClassification;
  

enum UsdHalfEdgeMeshVaryingBits {
  VARYING_TOPOLOGY = 1,
  VARYING_DEFORM = 2,
  VARYING_TRANSFORM = 4,
  VARYING_VISIBILITY = 8
};

struct UsdNprHalfEdge
{
  uint32_t                index;     // half edge index
  uint32_t                vertex;    // vertex index
  struct UsdNprHalfEdge*  twin;      // opposite half-edge
  struct UsdNprHalfEdge*  next;      // next half-edge

  UsdNprHalfEdge():vertex(0),twin(NULL),next(NULL){};
  inline size_t GetTriangleIndex() const {return index / 3;};
  void GetTriangleNormal(const GfVec3f* positions, GfVec3f& normal) const;
  void GetVertexNormal(const GfVec3f* normals, GfVec3f& normal) const;
  bool GetFacing(const GfVec3f* positions, const GfVec3f& v, float* weight) const;
  bool GetFacing(const GfVec3f* positions, const GfVec3f* normals,
    const GfVec3f& v, float* weight) const;
  bool GetVertexFacing(const GfVec3f* positions, const GfVec3f* normals,
    const GfVec3f& v, float* weight) const;
  float GetDot(const GfVec3f* positions, const GfVec3f* normals,
    const GfVec3f& v) const;
  short GetFlags(const GfVec3f* positions, const GfVec3f* normals, 
    const GfVec3f& v, float creaseValue, float* weight) const;
  void GetWeightedPositionAndNormal(const GfVec3f* positions, 
    const GfVec3f* normals, float weight, GfVec3f& position, GfVec3f& normal);
};

/// \class UsdNprHalfEdgeMesh
///
class UsdNprHalfEdgeMesh
{
public:
  UsdNprHalfEdgeMesh(const SdfPath& path, const HdDirtyBits& varyingBits);
  void Init(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode);
  void Update(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode);
  const std::vector<UsdNprHalfEdge>& GetHalfEdges() const {return _halfEdges;};

  const std::vector<UsdNprHalfEdge>& GetHalfEdges(){return _halfEdges;};
  const UsdNprHalfEdge* GetHalfEdgesPtr() const {return &_halfEdges[0];};
  const GfVec3f* GetPositionsPtr() const {return &_positions[0];};
  const GfVec3f* GetNormalsPtr() const {return &_normals[0];};
  size_t GetNumPoints() const {return _positions.size();};
  size_t GetNumTriangles() const {return _numTriangles;};
  size_t GetNumHalfEdges() const {return _halfEdges.size();};

  // object
  const SdfPath& GetPath(){return _sdfPath;};

  // xform
  void SetMatrix(const GfMatrix4d& m){_xform = GfMatrix4f(m);};
  const GfMatrix4f& GetMatrix() const {return _xform;};

  // varying
  bool IsVarying() const {return _varyingBits != 0;};
  bool IsTopoVarying() const {return _varyingBits & VARYING_TOPOLOGY;};
  bool IsDeformVarying() const {return _varyingBits & VARYING_DEFORM;};
  bool IsTransformVarying() const {return _varyingBits & VARYING_TRANSFORM;};
  bool IsVisibilityVarying() const {return _varyingBits & VARYING_VISIBILITY;};
  char GetVaryingBits() const {return _varyingBits;};

  // time
  void SetLastTime(double time){_lastTime = time;};
  double GetLastTime(){return _lastTime;};

  // mutex
  std::mutex& GetMutex(){return _mutex;};

private:
  SdfPath                     _sdfPath;
  GfMatrix4f                  _xform;
  size_t                      _numTriangles;
  std::vector<UsdNprHalfEdge> _halfEdges; 
  VtArray<GfVec3f>            _positions;
  VtArray<GfVec3f>            _normals;
  char                        _varyingBits;
  double                      _lastTime;
  mutable std::mutex          _mutex;

};

typedef std::shared_ptr<UsdNprHalfEdgeMesh> UsdNprHalfEdgeMeshSharedPtr;

/*
class UsdNprEdgeBuffer {
public:
  void Init()
private:
  TfHashMap<size_t, uint64_t, TfHash> buffer;
};
*/

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_NPR_IMAGING_HALFEDGE_H
