//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_DUALMESH_H
#define PXR_IMAGING_PLUGIN_LOFI_DUALMESH_H

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/vt/array.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiOctree;
class LoFiMesh;
class LoFiDualMesh;
struct LoFiHalfEdge;

static const int LOFI_OCTREE_MAX_EDGE_NUMBER = 12;

// eight surfaces of the 4D cube
enum { 
  PX = 0, 
  PY = 1,
  PZ = 2, 
  PW = 3, 
  NX = 4, 
  NY = 5, 
  NZ = 6, 
  NW = 7
};

class LoFiDualEdge {
public:
  // pos1 and pos2 should be projected to the same surface of the 4D cube
  LoFiDualEdge(const LoFiHalfEdge* halfEdge, bool facing,
	   int tp, const GfVec4f& pos1, const GfVec4f& pos2);

  ~LoFiDualEdge() { };

  // index in mesh
  int32_t GetTriangle(short i) const;
  int32_t GetPoint(short i) const;
  const LoFiHalfEdge* GetEdge() const {return _halfEdge;};

  // dual points
  GfVec3f GetDualPoint(short i){return _points[i];};

  // front facing or back facing
  bool IsFacing() const { return _facing; };

  // silhouette checked tag
  bool IsChecked() const { return _checked; };
  void Check()   { _checked = true;};
  void Uncheck() { _checked = false; };

  // touch a box
  bool Touch(const GfVec3f& minp, const GfVec3f& maxp) const;

private:
  const LoFiHalfEdge* _halfEdge;
  bool                _facing;
  bool                _checked;
  GfVec3f             _points[2];
};

// octree node
class LoFiOctree {
public:
  LoFiOctree() : _depth(0), _min(-1, -1, -1), _max(1, 1, 1), _isLeaf(true)
    { for (int i=0; i<8; i++) _children[i] = NULL; };
  LoFiOctree( const GfVec3f& minp, const GfVec3f& maxp, int depth = 0) :
    _depth(depth), _min(minp), _max(maxp), _isLeaf(true)
    { for (int i=0; i<8; i++) _children[i] = NULL; };
  ~LoFiOctree();

  // depth in octree
  int GetDepth() const { return _depth; };

  // bounding box
  const GfVec3f& GetBBoxMin() const { return _min; };
  const GfVec3f& GetBBoxMax() const { return _max; };

  // leaf
  bool IsLeaf() const { return _isLeaf; };

  // edges
  int GetNumDualEdges() const { return _dualEdges.size(); };
  std::vector<LoFiDualEdge*>& GetDualEdges() { return _dualEdges; };

  // insert dual edge
  void InsertEdge(LoFiDualEdge* e) { _dualEdges.push_back(e); };

  // split into 8
  void Split();

  // silhouettes
  void FindSilhouettes(const GfVec3f& n, float d, std::vector<const LoFiHalfEdge*>& silhouettes);

  void Log();

protected:

  // depth in octree
  int _depth;

  // bounding box
  GfVec3f _min, _max;

  // leaf ?
  bool _isLeaf;

  // children
  LoFiOctree* _children[8];

  // edges
  std::vector<LoFiDualEdge*> _dualEdges;
  
  // touch the camera plane
  bool _TouchPlane(const GfVec3f& n, float d);
};

class LoFiDualMesh : public LoFiOctree{
public:
  ~LoFiDualMesh();
  // build the tree
  void Build(LoFiMesh* mesh);

  // mesh
  const LoFiMesh* GetMesh() const {return _mesh;};

  // silhouettes
  void ClearSilhouettes();
  void FindSilhouettes(const GfMatrix4d& viewMatrix);
  void UncheckAllEdges();

  // project points to dual space
  //void ProjectPoints();

  // project edges to dual space
  void ProjectEdge(const LoFiHalfEdge* halfEdge);

private:      
  // mesh
  LoFiMesh* _mesh;    
  //VtArray<GfVec4f> _dualPoints;

  std::vector<const LoFiHalfEdge*> _boundaries;
  std::vector<const LoFiHalfEdge*> _silhouettes;
};

PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXR_IMAGING_PLUGIN_LOFI_DUALMESH_H
