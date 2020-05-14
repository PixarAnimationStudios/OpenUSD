//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_USD_IMAGING_USD_NPR_IMAGING_STROKE_H
#define PXR_USD_IMAGING_USD_NPR_IMAGING_STROKE_H

#include "pxr/pxr.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include <vector>
#include <memory>


PXR_NAMESPACE_OPEN_SCOPE

struct UsdNprHalfEdge;
class UsdNprHalfEdgeMesh;

enum UsdNprEdgeFlags : short {
  EDGE_BOUNDARY   = 1,
  EDGE_CREASE     = 2,
  EDGE_SILHOUETTE = 4,
  EDGE_TWIN       = 8,
  EDGE_VISITED    = 16,
  EDGE_CHAINED    = 32
};

struct UsdNprEdgeClassification {
  std::vector<const UsdNprHalfEdge*> silhouettes;
  std::vector<float>                 weights;
  std::vector<const UsdNprHalfEdge*> creases;
  std::vector<const UsdNprHalfEdge*> boundaries;
  std::vector<short>                 allFlags;
};

struct UsdNprStrokeParams {
  bool findSilhouettes;
  bool findCreases;
  bool findBoundaries;
  float creaseValue;
  float silhouetteWidth;
  float creaseWidth;
  float boundaryWidth;

  UsdNprStrokeParams()
    : findSilhouettes(true)
    , findCreases(true)
    , findBoundaries(true)
    , creaseValue(1.0)
    , silhouetteWidth(0.2)
    , creaseWidth(0.15)
    , boundaryWidth(0.4){};
};

struct UsdNprStrokeNode {
  UsdNprHalfEdge* edge;
  float width;
  float weight;

  GfVec3f position;
  GfVec3f color;

  UsdNprStrokeNode(UsdNprHalfEdge* e, float w):
    edge(e), width(w), weight(0.5){};
};

typedef std::vector<UsdNprStrokeNode> UsdNprStrokeNodeList;

class UsdNprStrokeChain {
public:
  void Init(UsdNprHalfEdge* edge, short type, float width);
  void Build(std::vector<short>& edgeClassification, short type);
  
  void ComputeOutputPoints( const UsdNprHalfEdgeMesh* mesh, 
    const GfVec3f& viewPoint, GfVec3f* points) const;

  size_t GetNumNodes() const {return _nodes.size();};
  const UsdNprStrokeNodeList GetNodes() const {return _nodes;};

private:
  void _ComputePoint(const GfVec3f* positions, const GfMatrix4f& xform,
    size_t index, const GfVec3f& V, float width, GfVec3f* p1, GfVec3f* p2) const;

  UsdNprHalfEdge* _start;
  UsdNprStrokeNodeList _nodes;
  float _width;
  short _type;
};

typedef std::vector<UsdNprStrokeChain> UsdNprStrokeChainList;

class UsdNprStrokeGraph {
public:
  void Init(UsdNprHalfEdgeMesh* mesh, const GfMatrix4f& view, const GfMatrix4f& proj);
  void Prepare(const UsdNprStrokeParams& params);
  void ResetChainedFlag(const std::vector<const UsdNprHalfEdge*>& edges);
  void ClearStrokeChains();
  void BuildStrokeChains(short edgeType);

  const UsdNprStrokeChainList& GetStrokes() const {return _strokes;};
  UsdNprHalfEdgeMesh* GetMesh() {return _mesh;};
  const UsdNprHalfEdgeMesh* GetMesh() const {return _mesh;};
  const std::vector<const UsdNprHalfEdge*>& GetSilhouettes(){return _silhouettes;};

  size_t GetNumStrokes() const {return _strokes.size();};
  size_t GetNumNodes() const;
  GfVec3f GetViewPoint() const;

private:
  GfMatrix4f                         _viewMatrix;
  GfMatrix4f                         _projectionMatrix;
  UsdNprHalfEdgeMesh*                _mesh;

  UsdNprStrokeChainList              _strokes;

  std::vector<const UsdNprHalfEdge*> _silhouettes;
  std::vector<const UsdNprHalfEdge*> _boundaries;
  std::vector<const UsdNprHalfEdge*> _creases;
  std::vector<short>                 _allFlags;
};

typedef std::vector<UsdNprStrokeGraph> UsdNprStrokeGraphList;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_NPR_IMAGING_STROKE_H
