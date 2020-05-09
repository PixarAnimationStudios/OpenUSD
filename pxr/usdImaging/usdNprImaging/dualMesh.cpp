
#include "pxr/usdImaging/usdNprImaging/dualMesh.h"
#include "pxr/usdImaging/usdNprImaging/halfEdge.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


int32_t UsdNprDualEdge::GetTriangle(short i) const 
{
  if(i==0)return _halfEdge->triangle;
  else
  {
    if(_halfEdge->twin)return _halfEdge->twin->triangle;
    else return -1;
  }
}

int32_t UsdNprDualEdge::GetPoint(short i) const 
{
  if(i == 0)return _halfEdge->vertex;
  else return _halfEdge->next->vertex;
}

UsdNprDualEdge::UsdNprDualEdge(const UsdNprHalfEdge* halfEdge, bool facing,
		   int tp, const GfVec4d& pos1, const GfVec4d& pos2, int32_t index)
  : _halfEdge(halfEdge)
  , _facing(facing)
  , _index(index)
{

  float d1, d2;
  switch (tp) {
  case PX:
  case NX:
    d1 = GfMax(DBL_EPSILON, GfAbs(pos1[0]));
    d2 = GfMax(DBL_EPSILON, GfAbs(pos2[0]));
    _points[0] = GfVec3d(pos1[1]/d1, pos1[2]/d1, pos1[3]/d1);
    _points[1] = GfVec3d(pos2[1]/d2, pos2[2]/d2, pos2[3]/d2);
    break;
  case PY:
  case NY:
    d1 = GfMax(DBL_EPSILON, GfAbs(pos1[1]));
    d2 = GfMax(DBL_EPSILON, GfAbs(pos2[1]));
    _points[0] = GfVec3d(pos1[2]/d1, pos1[3]/d1, pos1[0]/d1);
    _points[1] = GfVec3d(pos2[2]/d2, pos2[3]/d2, pos2[0]/d2);
    break;
  case PZ:
  case NZ:
    d1 = GfMax(DBL_EPSILON, GfAbs(pos1[2]));
    d2 = GfMax(DBL_EPSILON, GfAbs(pos2[2]));
    _points[0] = GfVec3d(pos1[3]/d1, pos1[0]/d1, pos1[1]/d1);
    _points[1] = GfVec3d(pos2[3]/d2, pos2[0]/d2, pos2[1]/d2);
    break;
  case PW:
  case NW:
    d1 = GfMax(DBL_EPSILON, GfAbs(pos1[3]));
    d2 = GfMax(DBL_EPSILON, GfAbs(pos2[3]));
    _points[0] = GfVec3d(pos1[0]/d1, pos1[1]/d1, pos1[2]/d1);
    _points[1] = GfVec3d(pos2[0]/d2, pos2[1]/d2, pos2[2]/d2);
  }
}

// intersect a box
bool UsdNprDualEdge::Touch(const GfVec3d& minp, const GfVec3d& maxp) const {
  int m = 4;
  GfVec3d step = (_points[1] - _points[0]) / m;
  GfVec3d A = _points[0], B = _points[0] + step;
  for (int i = 0; i < m; i++) 
  {
    GfVec3d bmin(GfMin(A[0], B[0]), GfMin(A[1], B[1]), GfMin(A[2], B[2]));
    GfVec3d bmax(GfMax(A[0], B[0]), GfMax(A[1], B[1]), GfMax(A[2], B[2]));
    if (bmin[0] <= maxp[0] && minp[0] <= bmax[0] &&
        bmin[1] <= maxp[1] && minp[1] <= bmax[1] &&
        bmin[2] <= maxp[2] && minp[2] <= bmax[2]) return true;
    A = B;
    B += step;
  }
  return false;
}

UsdNprOctree::~UsdNprOctree() 
{
  for (int i=0; i<8; i++) 
  {
    if (_children[i]) delete _children[i];
    _children[i] = NULL;
  }

  _dualEdges.clear();
}

static UsdNprOctree* _CreateOctreeCell(const GfVec3d& bmin, const GfVec3d& bmax,
  size_t depth, const std::vector<UsdNprDualEdge*>& dualEdges)
{
  UsdNprOctree* cell = new UsdNprOctree( bmin, bmax, depth, dualEdges.size());

  for(const auto& dualEdge: dualEdges)
    if(dualEdge->Touch(bmin, bmax))
      cell->InsertEdge(dualEdge);

  if(!cell->GetNumDualEdges())
  {
    delete cell;
    return NULL;
  }
  return cell;
}

void UsdNprOctree::Split() 
{
  int esz = _dualEdges.size();

  if (esz <= NPR_OCTREE_MAX_EDGE_NUMBER || 
      (esz <= 2* NPR_OCTREE_MAX_EDGE_NUMBER && _depth > 3) ||
      (esz <= 3* NPR_OCTREE_MAX_EDGE_NUMBER && _depth > 4) ||
      _depth > 5 ) {
    _isLeaf = true;
    return;
  }

  _isLeaf = false;

  double xx[] = {_min[0], 0.5f*(_min[0]+_max[0]), _max[0]};
  double yy[] = {_min[1], 0.5f*(_min[1]+_max[1]), _max[1]};
  double zz[] = {_min[2], 0.5f*(_min[2]+_max[2]), _max[2]};

  _children[0] = _CreateOctreeCell(GfVec3d(xx[0], yy[0], zz[0]), 
    GfVec3d(xx[1], yy[1], zz[1]), _depth+1, _dualEdges);
  _children[1] = _CreateOctreeCell(GfVec3d(xx[0], yy[0], zz[1]), 
    GfVec3d(xx[1], yy[1], zz[2]), _depth+1, _dualEdges);
  _children[2] = _CreateOctreeCell(GfVec3d(xx[0], yy[1], zz[0]), 
    GfVec3d(xx[1], yy[2], zz[1]), _depth+1, _dualEdges);
  _children[3] = _CreateOctreeCell(GfVec3d(xx[0], yy[1], zz[1]), 
    GfVec3d(xx[1], yy[2], zz[2]), _depth+1, _dualEdges);
  _children[4] = _CreateOctreeCell(GfVec3d(xx[1], yy[0], zz[0]), 
    GfVec3d(xx[2], yy[1], zz[1]), _depth+1, _dualEdges);
  _children[5] = _CreateOctreeCell(GfVec3d(xx[1], yy[0], zz[1]), 
    GfVec3d(xx[2], yy[1], zz[2]), _depth+1, _dualEdges);
  _children[6] = _CreateOctreeCell(GfVec3d(xx[1], yy[1], zz[0]), 
    GfVec3d(xx[2], yy[2], zz[1]), _depth+1, _dualEdges);
  _children[7] = _CreateOctreeCell(GfVec3d(xx[1], yy[1], zz[1]), 
    GfVec3d(xx[2], yy[2], zz[2]), _depth+1, _dualEdges);

  for(int j=0;j<8;++j)
    if(_children[j])_children[j]->Split();

  _dualEdges.clear();
}

// intersect a plane
bool UsdNprOctree::_TouchPlane(const GfVec3d& n, double d) {
  bool sa = n[0]>=0, sb = n[1]>=0, sc = n[2]>=0;
  double p1x = _min[0], p1y, p1z, p2x = _max[0], p2y, p2z;

  if (sb == sa) {
    p1y = _min[1]; p2y = _max[1];
  } else {
    p1y = _max[1]; p2y = _min[1];
  }

  if (sc == sa) {
    p1z = _min[2]; p2z = _max[2];
  } else {
    p1z = _max[2]; p2z = _min[2];
  }

  double dot1 = n[0]*p1x + n[1]*p1y + n[2]*p1z + d ;
  double dot2 = n[0]*p2x + n[1]*p2y + n[2]*p2z + d ;
  bool sd1 = dot1 >= 0;
  bool sd2 = dot2 >= 0;
  return (sd1 != sd2);
}

// looking for silhouettes recursively
void UsdNprOctree::FindSilhouettes(const GfVec3d& n, double d, 
  std::vector<const UsdNprHalfEdge*>& silhouettes,
  std::vector<bool>& checked) 
{
  if (_isLeaf) 
  {
    for (int j = 0; j < _dualEdges.size(); ++j)
    {
      size_t dualEdgeIndex = _dualEdges[j]->GetIndex();
      if (!checked[dualEdgeIndex]) 
      {
        checked[dualEdgeIndex] = true;
        bool b1 = (n * _dualEdges[j]->GetDualPoint(0) + d )> 0;
        bool b2 = (n * _dualEdges[j]->GetDualPoint(1) + d )> 0;
        if (b1 != b2) 
        {
          silhouettes.push_back(_dualEdges[j]->GetEdge());
        }
      }
    }
  } 
  else 
  {
    for (int j = 0; j < 8; ++j)
      if (_children[j] && _children[j]->_TouchPlane(n, d))
	        _children[j]->FindSilhouettes(n, d, silhouettes, checked);
  }
}


void UsdNprOctree::Log()
{
  if(_isLeaf)
  {
    for(const auto& dualEdge: _dualEdges)
    {
      const UsdNprHalfEdge* halfEdge = dualEdge->GetEdge();
      std::cout << "(" << halfEdge->vertex << "," << halfEdge->next->vertex << "),";
    }
    std::cout << std::endl;
  }
   else 
  {
    for (int i = 0; i < 8; i++)
      if (_children[i])_children[i]->Log();
  }
}

void UsdNprOctree::CountDualEdges(size_t* count)
{
  if(_isLeaf)
  {
    *count += _dualEdges.size();
  }
   else 
  {
    for (int i = 0; i < 8; i++)
      if (_children[i])_children[i]->CountDualEdges(count);
  }
}


// destructor
UsdNprDualMesh::~UsdNprDualMesh()
{
  // delete dual edges
  int sz = _dualEdges.size();
  for (int j=0; j<sz; ++j) delete _dualEdges[j];
  delete _halfEdgeMesh;
}

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

// mesh
void UsdNprDualMesh::InitMesh(const UsdGeomMesh& mesh, HdDirtyBits varyingBits)
{
  _halfEdgeMesh = new UsdNprHalfEdgeMesh(_ConvertVaryingBits(varyingBits));
  _halfEdgeMesh->Compute(mesh, UsdTimeCode::EarliestTime());
  _meshXform = GfMatrix4f(1);
}

void UsdNprDualMesh::UpdateMesh(const UsdGeomMesh& mesh, const UsdTimeCode& timeCode, 
  bool recomputeAdjacency)
{
  if(recomputeAdjacency)_halfEdgeMesh->Compute(mesh, timeCode);
  else _halfEdgeMesh->Update(mesh, timeCode);
}

char UsdNprDualMesh::GetMeshVaryingBits()
{
  return _halfEdgeMesh->GetVaryingBits();
}

// clear octree
void UsdNprDualMesh::Clear()
{
  _creases.clear();
  _boundaries.clear();

  for(auto& dualEdge: _dualEdges)delete dualEdge;
  _dualEdges.clear();

  for(int i=0;i<8;++i)
    if(_children[i])
    {
      delete _children[i];
      _children[i] = NULL;
    }
}

// build octree
void UsdNprDualMesh::Build() 
{
  Clear();
  
  _isLeaf = false;
  for(int j=0;j<8;++j)_children[j] = new UsdNprOctree();

  const std::vector<UsdNprHalfEdge>& halfEdges = _halfEdgeMesh->GetHalfEdges();
  _dualEdgeIndex = 0;
  for(const auto& halfEdge: halfEdges) 
    ProjectEdge(&halfEdge);

  // rebuilding the acceleration structure each frame is too expensive
  // it is faster to do brute force silhouette detection
  if(!_halfEdgeMesh->IsTopoVarying() && !_halfEdgeMesh->IsDeformVarying())
    for(int j=0;j<8;++j)_children[j]->Split();
}

void UsdNprDualMesh::ProjectEdge(const UsdNprHalfEdge* halfEdge) 
{
  const GfVec3f* positions = _halfEdgeMesh->GetPositionsPtr();
  size_t numPoints =  _halfEdgeMesh->GetNumPoints();
  size_t numTriangles = _halfEdgeMesh->GetNumTriangles();

  const UsdNprHalfEdge* twinEdge = halfEdge->twin;
  // check boundary
  if(! twinEdge)
  {
    _boundaries.push_back(halfEdge);
    return;
  }

  // insure every internal edge is projected exactly once
  if (twinEdge->triangle < halfEdge->triangle) return;

  // facing of the edge
  GfVec3f trn1, trn2;
  halfEdge->GetTriangleNormal(positions, trn1);
  float ff = trn1 * 
    (positions[twinEdge->next->vertex] -
    positions[halfEdge->next->vertex]);

#ifndef NDEBUG
  if (fabs(ff) < DBL_EPSILON)
    std::cerr << "WARNING : potential facing precision problem " << ff << std::endl;
#endif

  bool facing = (ff > 0);

  GfVec4d n1( trn1[0], trn1[1], trn1[2], 
    - (trn1 * positions[halfEdge->vertex])); 

  twinEdge->GetTriangleNormal(positions, trn2);
  GfVec4d n2( trn2[0], trn2[1], trn2[2], 
    - (trn2 * positions[twinEdge->vertex]));

  if(GfAbs(trn1 * trn2) < 0.25)_creases.push_back(halfEdge);

  GfVec4d n = n2 - n1;

  double t[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  
  // if the edge should be projected on two surfaces, it will intersect
  // the plane passing orig and the intersection line of the two surfaces
  // these are the 12 possible intersections
  if (n[0] - n[1] != 0) t[1] = - (n1[0] - n1[1]) / (n[0] - n[1]);
  if (n[0] - n[2] != 0) t[2] = - (n1[0] - n1[2]) / (n[0] - n[2]);
  if (n[0] - n[3] != 0) t[3] = - (n1[0] - n1[3]) / (n[0] - n[3]);
  if (n[0] + n[1] != 0) t[4] = - (n1[0] + n1[1]) / (n[0] + n[1]);
  if (n[0] + n[2] != 0) t[5] = - (n1[0] + n1[2]) / (n[0] + n[2]);
  if (n[0] + n[3] != 0) t[6] = - (n1[0] + n1[3]) / (n[0] + n[3]);
  if (n[1] - n[2] != 0) t[7] = - (n1[1] - n1[2]) / (n[1] - n[2]);
  if (n[1] - n[3] != 0) t[8] = - (n1[1] - n1[3]) / (n[1] - n[3]);
  if (n[1] + n[2] != 0) t[9] = - (n1[1] + n1[2]) / (n[1] + n[2]);
  if (n[1] + n[3] != 0)t[10] = - (n1[1] + n1[3]) / (n[1] + n[3]);
  if (n[2] - n[3] != 0)t[11] = - (n1[2] - n1[3]) / (n[2] - n[3]);
  if (n[2] + n[3] != 0)t[12] = - (n1[2] + n1[3]) / (n[2] + n[3]);

  int p1 = 0, p2;
  while (p1 < 13) 
  {
    p2 = 13;
    for (int i=1; i<13; i++) 
    {
      if (t[i] > t[p1] && t[i] < t[p2])	p2 = i;
    }

    // the part that is projected on one of the surfaces
    GfVec4d pos1 = n1 + t[p1]*n, pos2 = n1 + t[p2]*n;
    GfVec4d mid = 0.5*(pos1+pos2);

    GfVec4d MID = GfVec4d(GfAbs(mid[0]), GfAbs(mid[1]), GfAbs(mid[2]), GfAbs(mid[3]));
    p1 = p2;

    // the surface projected on
    int tp;
    if (MID[0] >= MID[1] && MID[0] >= MID[2] && MID[0] >= MID[3])
      tp = mid[0] > 0 ? PX : NX;
    else if (MID[1] >= MID[2] && MID[1] >= MID[3])
      tp = mid[1] > 0 ? PY : NY;
    else if (MID[2] >= MID[3])
      tp = mid[2] > 0 ? PZ : NZ;
    else
      tp = mid[3] > 0 ? PW : NW;

    // project this part on the surface
    UsdNprDualEdge* dualEdge = 
      new UsdNprDualEdge(halfEdge, facing, tp, pos1, pos2, _dualEdgeIndex++);
    _children[tp]->InsertEdge(dualEdge);
    _dualEdges.push_back(dualEdge);
  }
}

// looking for silhouettes recursively
void UsdNprDualMesh::FindSilhouettes(const GfMatrix4d& viewMatrix, 
  std::vector<const UsdNprHalfEdge*>& silhouettes,
  std::vector<bool>& checked)
{

  GfVec3d pos(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
  pos = _meshXform.GetInverse().Transform(pos);
  
  _children[0]->FindSilhouettes(GfVec3d(pos[1], pos[2], 1), pos[0],
			       silhouettes, checked);
  
  _children[1]->FindSilhouettes(GfVec3d(pos[2], 1, pos[0]), pos[1],
			       silhouettes, checked);
  _children[2]->FindSilhouettes(GfVec3d(1, pos[0], pos[1]), pos[2],
			       silhouettes, checked);
  _children[3]->FindSilhouettes(pos, 1, silhouettes, checked);
  
  _children[4]->FindSilhouettes(GfVec3d(pos[1], pos[2], 1), -pos[0],
			       silhouettes, checked);
            
  _children[5]->FindSilhouettes(GfVec3d(pos[2], 1, pos[0]), -pos[1],
			       silhouettes, checked);
  _children[6]->FindSilhouettes(GfVec3d(1, pos[0], pos[1]), -pos[2],
			       silhouettes, checked);
  _children[7]->FindSilhouettes(pos, -1, silhouettes, checked);
  
}

size_t UsdNprDualMesh::GetNumHalfEdges() const 
{ 
  return _halfEdgeMesh->GetNumHalfEdges(); 
};

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

void UsdNprDualMesh::ComputeOutputGeometry(std::vector<const UsdNprHalfEdge*>& silhouettes,
  const GfVec3f& viewPoint, VtArray<GfVec3f>& points, VtArray<int>& faceVertexCounts,
  VtArray<int>& faceVertexIndices)
{
  size_t numEdges = silhouettes.size() + _creases.size() + _boundaries.size();
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
    const GfVec3f* positions = _halfEdgeMesh->GetPositionsPtr();
    const GfVec3f& A = _meshXform.Transform(positions[halfEdge->vertex]);
    const GfVec3f& B = _meshXform.Transform(positions[halfEdge->twin->vertex]);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 0);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 1);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 2);
    points[index++] = _ComputePoint(A, B, viewPoint, width, 3);
  }
  
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
  
}

PXR_NAMESPACE_CLOSE_SCOPE
