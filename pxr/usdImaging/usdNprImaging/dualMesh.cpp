
#include "pxr/usdImaging/usdNprImaging/dualMesh.h"
#include "pxr/usdImaging/usdNprImaging/halfEdge.h"
#include "pxr/imaging/pxOsd/tokens.h"
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
		   int tp, const GfVec4f& pos1, const GfVec4f& pos2)
  : _halfEdge(halfEdge)
  , _facing(facing)
  , _checked(false)
{

  float d1, d2;
  switch (tp) {
  case PX:
  case NX:
    d1 = GfMax(FLT_EPSILON, fabs(pos1[0]));
    d2 = GfMax(FLT_EPSILON, fabs(pos2[0]));
    _points[0] = GfVec3f(pos1[1]/d1, pos1[2]/d1, pos1[3]/d1);
    _points[1] = GfVec3f(pos2[1]/d2, pos2[2]/d2, pos2[3]/d2);
    break;
  case PY:
  case NY:
    d1 = GfMax(FLT_EPSILON, fabs(pos1[1]));
    d2 = GfMax(FLT_EPSILON, fabs(pos2[1]));
    _points[0] = GfVec3f(pos1[2]/d1, pos1[3]/d1, pos1[0]/d1);
    _points[1] = GfVec3f(pos2[2]/d2, pos2[3]/d2, pos2[0]/d2);
    break;
  case PZ:
  case NZ:
    d1 = GfMax(FLT_EPSILON, fabs(pos1[2]));
    d2 = GfMax(FLT_EPSILON, fabs(pos2[2]));
    _points[0] = GfVec3f(pos1[3]/d1, pos1[0]/d1, pos1[1]/d1);
    _points[1] = GfVec3f(pos2[3]/d2, pos2[0]/d2, pos2[1]/d2);
    break;
  case PW:
  case NW:
    d1 = GfMax(FLT_EPSILON, fabs(pos1[3]));
    d2 = GfMax(FLT_EPSILON, fabs(pos2[3]));
    _points[0] = GfVec3f(pos1[0]/d1, pos1[1]/d1, pos1[2]/d1);
    _points[1] = GfVec3f(pos2[0]/d2, pos2[1]/d2, pos2[2]/d2);
  }
}

// intersect a box
bool UsdNprDualEdge::Touch(const GfVec3f& minp, const GfVec3f& maxp) const {
  int m = 4;
  GfVec3f step = (_points[1] - _points[0]) / m;
  GfVec3f A = _points[0], B = _points[0] + step;
  for (int i = 0; i < m; i++) 
  {
    GfVec3f bmin(GfMin(A[0], B[0]), GfMin(A[1], B[1]), GfMin(A[2], B[2]));
    GfVec3f bmax(GfMax(A[0], B[0]), GfMax(A[1], B[1]), GfMax(A[2], B[2]));
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

void UsdNprOctree::Split() 
{
  int esz = _dualEdges.size();

  if (esz <= NPR_OCTREE_MAX_EDGE_NUMBER || 
      (esz <= 2* NPR_OCTREE_MAX_EDGE_NUMBER && _depth > 3) ||
      (esz <= 3* NPR_OCTREE_MAX_EDGE_NUMBER && _depth > 4) ||
      _depth > 6 ) {
    _isLeaf = true;
    return;
  }

  _isLeaf = false;

  float xx[] = {_min[0], 0.5f*(_min[0]+_max[0]), _max[0]};
  float yy[] = {_min[1], 0.5f*(_min[1]+_max[1]), _max[1]};
  float zz[] = {_min[2], 0.5f*(_min[2]+_max[2]), _max[2]};
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++)
      for (int k = 0; k < 2; k++) 
      {
        int m = 4*i + 2*j + k;
        _children[m] = 
          new UsdNprOctree( GfVec3f(xx[i], yy[j], zz[k]),
                          GfVec3f(xx[i+1], yy[j+1], zz[k+1]),
                          _depth+1 );
        int esz = _dualEdges.size();
        for (int t = 0; t < esz; t++)
          if (_dualEdges[t]->Touch(_children[m]->GetBBoxMin(), _children[m]->GetBBoxMax()))
            _children[m]->InsertEdge(_dualEdges[t]);
        if (_children[m]->GetNumDualEdges() == 0)
        {
          delete _children[m];
          _children[m] = NULL;
        } 
        else _children[m]->Split();
      }
  _dualEdges.clear();
}

// intersect a plane

bool UsdNprOctree::_TouchPlane(const GfVec3f& n, float d) {
  bool sa = n[0]>=0, sb = n[1]>=0, sc = n[2]>=0;
  float p1x = _min[0], p1y, p1z, p2x = _max[0], p2y, p2z;

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

  float dot1 = n[0]*p1x + n[1]*p1y + n[2]*p1z + d ;
  float dot2 = n[0]*p2x + n[1]*p2y + n[2]*p2z + d ;
  bool sd1 = dot1 >= 0;
  bool sd2 = dot2 >= 0;
  return (sd1 != sd2);
}

// looking for silhouettes recursively
void UsdNprOctree::FindSilhouettes(const GfVec3f& n, float d, std::vector<const UsdNprHalfEdge*>& silhouettes) 
{
  if (_isLeaf) 
  {
    for (int j = 0; j < _dualEdges.size(); ++j)
    {
      if (!_dualEdges[j]->IsChecked()) 
      {
        _dualEdges[j]->Check();
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
    for (int i = 0; i < 8; i++)
      if (_children[i] && _children[i]->_TouchPlane(n, d))
	      _children[i]->FindSilhouettes(n, d, silhouettes);
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


// destructor
UsdNprDualMesh::~UsdNprDualMesh()
{
  // delete dual edges
  int sz = _dualEdges.size();
  for (int j=0; j<sz; ++j) delete _dualEdges[j];
  int sm = _meshes.size();
  for (int j=0;j<sm; ++j)delete _meshes[j];
}

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
  return outVaryingBits;
}

// mesh
void UsdNprDualMesh::AddMesh(const UsdGeomMesh& mesh, HdDirtyBits varyingBits)
{
  UsdNprHalfEdgeMesh* halfEdgeMesh = 
    new UsdNprHalfEdgeMesh(_ConvertVaryingBits(varyingBits));
  size_t meshIndex = _meshes.size();
  halfEdgeMesh->Compute(mesh, meshIndex);
  _meshes.push_back(halfEdgeMesh);
}

void UsdNprDualMesh::UpdateMesh(const UsdGeomMesh& mesh, size_t index)
{

}

// build octree
void UsdNprDualMesh::Build() 
{
  // clear boundaries
  _boundaries.clear();
  _silhouettes.clear();
  _isLeaf = false;
  for(int j=0;j<8;++j)_children[j] = new UsdNprOctree();

  for(const auto& mesh: _meshes)
  {
    const std::vector<UsdNprHalfEdge>& halfEdges = mesh->GetHalfEdges();

    for(const auto& halfEdge: halfEdges) 
      ProjectEdge(&halfEdge);
  }

  for(int j=0;j<8;++j)_children[j]->Split();
}

short _UsdNprGetDualSurfaceIndex(const GfVec4f& dualPoint)
{
  float base = -1;
  size_t index = 0;
  for(int i=0;i<4;++i)
  {
    float ac = GfAbs(dualPoint[i]);
    if(ac>base) {
      index = i; base = ac;
    }
  }
  if(dualPoint[index]<0)index+=4;
  return index;
}

void UsdNprDualMesh::ProjectEdge(const UsdNprHalfEdge* halfEdge) 
{
  UsdNprHalfEdgeMesh* mesh = _meshes[halfEdge->mesh];
  const GfVec3f* positions = mesh->GetPositionsPtr();
  size_t numPoints =  mesh->GetNumPoints();
  size_t numTriangles = mesh->GetNumTriangles();

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
  GfVec3f trn;
  halfEdge->GetTriangleNormal(positions, trn);
  float ff = trn * 
    (positions[twinEdge->next->vertex] -
    positions[halfEdge->next->vertex]);

#ifndef NDEBUG
  if (fabs(ff) < FLT_EPSILON)
    std::cerr << "WARNING : potential facing precision problem " << ff << std::endl;
#endif

  bool facing = (ff > 0);

  GfVec4f n1( trn[0], trn[1], trn[2], - GfDot(trn, positions[halfEdge->vertex])); 

  twinEdge->GetTriangleNormal(positions, trn);
  GfVec4f n2( trn[0], trn[1], trn[2], - GfDot(trn, positions[twinEdge->vertex]));
  
  /*
  size_t idx1 = _UsdNprGetDualSurfaceIndex(n1);
  size_t idx2 = _UsdNprGetDualSurfaceIndex(n2);

  if(idx1 == idx2)
  {
    UsdNprDualEdge* dualEdge = 
      new UsdNprDualEdge(halfEdge, facing, idx1, n1, n2);
    _children[idx1]->InsertEdge(dualEdge);
    _dualEdges.push_back(dualEdge);
  }
  else
  {
    UsdNprDualEdge* dualEdge1 = 
      new UsdNprDualEdge(halfEdge, facing, idx1, n1, n2);
    _children[idx1]->InsertEdge(dualEdge1);
    _dualEdges.push_back(dualEdge1);

    UsdNprDualEdge* dualEdge2 = 
      new UsdNprDualEdge(halfEdge, facing, idx2, n1, n2);
    _children[idx2]->InsertEdge(dualEdge2);
    _dualEdges.push_back(dualEdge2);
  }
  */
  
  GfVec4f n = n2 - n1;

  float t[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  
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
    GfVec4f pos1 = n1 + t[p1]*n, pos2 = n1 + t[p2]*n;
    GfVec4f mid = 0.5*(pos1+pos2);

    GfVec4f MID = GfVec4f(GfAbs(mid[0]), GfAbs(mid[1]), GfAbs(mid[2]), GfAbs(mid[3]));
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
      new UsdNprDualEdge(halfEdge, facing, tp, pos1, pos2);
    _children[tp]->InsertEdge(dualEdge);
    _dualEdges.push_back(dualEdge);
  }
  
}

void UsdNprDualMesh::UncheckAllEdges()
{
  for(auto& dualEdge: _dualEdges)
    dualEdge->Uncheck();
}

void UsdNprDualMesh::ClearSilhouettes()
{
  _silhouettes.clear();
}

// looking for silhouettes recursively
void UsdNprDualMesh::FindSilhouettes(const GfMatrix4d& viewMatrix)
{
  ClearSilhouettes();
  GfVec3f pos(viewMatrix[3][0], viewMatrix[3][1], viewMatrix[3][2]);
  _children[0]->FindSilhouettes(GfVec3f(pos[1], pos[2], 1), pos[0],
			       _silhouettes);
  _children[1]->FindSilhouettes(GfVec3f(pos[2], 1, pos[0]), pos[1],
			       _silhouettes);
  _children[2]->FindSilhouettes(GfVec3f(1, pos[0], pos[1]), pos[2],
			       _silhouettes);
  _children[3]->FindSilhouettes(pos, 1, _silhouettes);
  _children[4]->FindSilhouettes(GfVec3f(pos[1], pos[2], 1), -pos[0],
			       _silhouettes);
  _children[5]->FindSilhouettes(GfVec3f(pos[2], 1, pos[0]), -pos[1],
			       _silhouettes);
  _children[6]->FindSilhouettes(GfVec3f(1, pos[0], pos[1]), -pos[2],
			       _silhouettes);
  _children[7]->FindSilhouettes(pos, -1, _silhouettes);

  _points.resize(_silhouettes.size() * 2);
  /*
  const GfVec3f* positions = _mesh->GetPositionsPtr();
  size_t index = 0;
  for(const auto& silhouette: _silhouettes)
  {
    _points[index++] = positions[silhouette->vertex];
    _points[index++] = positions[silhouette->twin->vertex];
  }
  */
}

void UsdNprDualMesh::ComputeOutputGeometry()
{
  size_t numEdges = _dualEdges.size();
  size_t numPoints = numEdges * 4;

  // topology
  VtArray<int> faceVertexCounts(numEdges);
  for(int i=0;i<numEdges;++i)faceVertexCounts[i] = 4;
  VtArray<int> faceVertexIndices(numPoints);
  for(int i=0;i<numPoints;++i)faceVertexIndices[i] = i;

  _topology = 
    HdMeshTopology(
      PxOsdOpenSubdivTokens->none,
      UsdGeomTokens->rightHanded,
      faceVertexCounts,
      faceVertexIndices);

  // points
  _points.resize(numPoints);
  size_t index = 0;
  float width = 0.02;
  for(const auto& dualEdge: _dualEdges)
  {
    const UsdNprHalfEdge* halfEdge = dualEdge->GetEdge();
    const UsdNprHalfEdgeMesh* mesh = GetMesh(halfEdge->mesh);
    const GfVec3f* positions = mesh->GetPositionsPtr();
    _points[index++] = positions[halfEdge->vertex] + GfVec3f(0,-width,0);
    _points[index++] = positions[halfEdge->twin->vertex] + GfVec3f(0,-width,0);
    _points[index++] = positions[halfEdge->twin->vertex] + GfVec3f(0,width,0);
    _points[index++] = positions[halfEdge->vertex] + GfVec3f(0,width,0);
  }
}


PXR_NAMESPACE_CLOSE_SCOPE
