//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_USD_IMAGING_USD_NPR_IMAGING_DUALMESH_H
#define PXR_USD_IMAGING_USD_NPR_IMAGING_DUALMESH_H

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/meshTopology.h"
#include <vector>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class UsdNprOctree;

struct UsdNprHalfEdge;
class UsdNprHalfEdgeMesh;

static const int NPR_OCTREE_MAX_EDGE_NUMBER = 64;

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

class UsdNprDualEdge {
public:
  // pos1 and pos2 should be projected to the same surface of the 4D cube
  UsdNprDualEdge(const UsdNprHalfEdge* halfEdge, bool facing,
	   int tp, const GfVec4f& pos1, const GfVec4f& pos2);

  ~UsdNprDualEdge() { };

  // mesh index
  int32_t GetMesh() const;
  // index in mesh
  int32_t GetTriangle(short i) const;
  int32_t GetPoint(short i) const;
  const UsdNprHalfEdge* GetEdge() const {return _halfEdge;};

  // dual points
  GfVec3f GetDualPoint(short i){return _points[i];};

  // front facing or back facing
  bool IsFacing() const { return _facing; };

  // silhouette checked tag
  bool IsChecked() const { return _checked; };
  void Check()   { _checked = true;};
  void Uncheck() { _checked = false;};

  // touch a box
  bool Touch(const GfVec3f& minp, const GfVec3f& maxp) const;

private:
  const UsdNprHalfEdge* _halfEdge;
  bool                _facing;
  bool                _checked;
  GfVec3f             _points[2];
};

// octree node
class UsdNprOctree {
public:
  UsdNprOctree() : _depth(0), _min(-1, -1, -1), _max(1, 1, 1), _isLeaf(true) { 
    for (int i=0; i<8; i++) _children[i] = NULL; 
  };
  UsdNprOctree( const GfVec3f& minp, const GfVec3f& maxp, int depth = 0, size_t n = 0) :
    _depth(depth), _min(minp), _max(maxp), _isLeaf(true) { 
      for (int i=0; i<8; i++) _children[i] = NULL; 
      _dualEdges.reserve(n);
    };
  ~UsdNprOctree();

  // depth in octree
  int GetDepth() const { return _depth; };

  // bounding box
  const GfVec3f& GetBBoxMin() const { return _min; };
  const GfVec3f& GetBBoxMax() const { return _max; };

  // leaf
  bool IsLeaf() const { return _isLeaf; };

  // edges
  int GetNumDualEdges() const { return _dualEdges.size(); };
  std::vector<UsdNprDualEdge*>& GetDualEdges() { return _dualEdges; };
  void UncheckAllEdges();

  // insert dual edge
  void InsertEdge(UsdNprDualEdge* e) { _dualEdges.push_back(e); };

  // split into 8
  void Split();

  // silhouettes
  void FindSilhouettes(const GfVec3f& n, float d, 
    std::vector<const UsdNprHalfEdge*>& silhouettes);
  void FindSilhouettes(const GfVec3f& n, float d, size_t* count);

  void Log();
  void CountDualEdges(size_t* count);

protected:

  // depth in octree
  int _depth;

  // bounding box
  GfVec3f _min, _max;

  // leaf ?
  bool _isLeaf;

  // children
  UsdNprOctree* _children[8];

  // edges
  std::vector<UsdNprDualEdge*> _dualEdges;
  
  // touch the camera plane
  bool _TouchPlane(const GfVec3f& n, float d);
};

class UsdNprDualMesh : public UsdNprOctree {
public:
  UsdNprDualMesh():UsdNprOctree(){};
  ~UsdNprDualMesh();

  // clear
  void Clear();

  // mesh
  void InitMesh(const UsdGeomMesh& mesh, HdDirtyBits varyingBits);
  void UpdateMesh(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode, 
    bool recomputeAdjacencency);
  const UsdNprHalfEdgeMesh* GetMesh() const {
    return _halfEdgeMesh;
  };
  char GetMeshVaryingBits();

  // xform
  void SetMatrix(const GfMatrix4d& m){_meshXform = GfMatrix4f(m);};

  // build the tree
  void Build();

  // silhouettes
  void FindSilhouettes(const GfMatrix4d& viewMatrix, std::vector<const UsdNprHalfEdge*>& silhouettes);


  // project edges to dual space
  void ProjectEdge(const UsdNprHalfEdge* halfEdge);

  // output
  void ComputeOutputGeometry(std::vector<const UsdNprHalfEdge*>& silhouettes,
    const GfVec3f& viewPoint, VtArray<GfVec3f>& points, VtArray<int>& faceVertexCounts,
    VtArray<int>& faceVertexIndices);

  // time
  void SetLastTime(const UsdTimeCode& timeCode){_lastTime = timeCode;};
  const UsdTimeCode& GetLastTime(){return _lastTime;};

private:      
  // mesh
  UsdNprHalfEdgeMesh*               _halfEdgeMesh;
  GfMatrix4f                        _meshXform;    

  std::vector<const UsdNprHalfEdge*> _boundaries;
  std::vector<const UsdNprHalfEdge*> _creases;

  UsdTimeCode                        _lastTime;
};

typedef std::shared_ptr<UsdNprDualMesh> UsdNprDualMeshSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXR_USD_IMAGING_USD_NPR_IMAGING_DUALMESH_H
