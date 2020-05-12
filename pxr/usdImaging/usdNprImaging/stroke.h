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

enum UsdNprStrokeType : short {
  STROKE_BOUNDARY = 1,
  STROKE_CREASE = 2,
  STROKE_SILHOUETTE = 4,
  STROKE_TWIN = 8
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

  UsdNprStrokeNode(UsdNprHalfEdge* e, float w):
    edge(e), width(w), weight(0.5){};
};

typedef std::vector<UsdNprStrokeNode> UsdNprStrokeNodeList;

struct UsdNprStrokeClassification {
  std::vector<const UsdNprHalfEdge*> silhouettes;
  std::vector<const UsdNprHalfEdge*> creases;
  std::vector<const UsdNprHalfEdge*> boundaries;
  std::vector<short>                 allFlags;
};

class UsdNprStrokeChain {
public:
  void Init(UsdNprHalfEdge* edge, short type, float width);
  void Build(const std::vector<short>& edgeClassification, short type, std::vector<bool>& visited);

  size_t GetNumPoints() const {return _nodes.size();};
  const UsdNprStrokeNodeList GetNodes(){return _nodes;};
  //void FindPreviousNode();
private:
  UsdNprHalfEdge* _start;
  UsdNprStrokeNodeList _nodes;
  float _width;
  short _type;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_NPR_IMAGING_STROKE_H
