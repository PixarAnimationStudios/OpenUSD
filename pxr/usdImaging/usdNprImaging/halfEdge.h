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

struct UsdNprHalfEdge
{
    uint32_t              mesh;      // mesh index
    uint32_t              vertex;    // vertex index
    uint32_t              sample;    // sample index
    uint32_t              triangle;  // triangle index
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
    void Compute(const UsdGeomMesh& mesh, size_t meshIndex);
    void Update(const UsdGeomMesh& mesh);
    const std::vector<UsdNprHalfEdge>& GetHalfEdges() const {return _halfEdges;};

    const GfVec3f* GetPositionsPtr(){return &_positions[0];};
    const GfVec3f* GetNormalsPtr(){return &_normals[0];};
    size_t GetNumPoints(){return _positions.size();};
    size_t GetNumTriangles(){return _numTriangles;};

private:
    std::vector<UsdNprHalfEdge> _halfEdges; 
    VtArray<GfVec3f>            _positions;
    VtArray<GfVec3f>            _normals;
    size_t                      _numTriangles;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_NPR_IMAGING_HALFEDGE_H
