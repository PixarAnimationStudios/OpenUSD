#include "pxr/imaging/glf/glew.h"
#include "pxr/usdImaging/usdNprImaging/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

int 
UsdNprTriangulateMesh(const VtArray<int>& counts, 
                    const VtArray<int>& indices, 
                    VtArray<int>& samples)
{
  int num_triangles = 0;
  for(auto count : counts)
  {
    num_triangles += count - 2;
  }

  samples.resize(num_triangles * 3);

  int base = 0;
  int cnt = 0;
  int prim = 0;
  for(auto count: counts)
  {
    for(int i = 1; i < count - 1; ++i)
    {
      samples[cnt++] = indices[base];
      samples[cnt++] = indices[base+i];
      samples[cnt++] = indices[base+i+1];
    }
    prim++;
    base += count;
  }
  return cnt / 3;
}

void 
UsdNprComputeVertexNormals( const VtArray<GfVec3f>& positions,
                          const VtArray<int>& counts,
                          const VtArray<int>& indices,
                          const VtArray<int>& samples,
                          VtArray<GfVec3f>& normals)
{
  // we want smooth vertex normals
  normals.resize(positions.size());
  memset(normals.data(), 0.f, normals.size() * sizeof(GfVec3f));

  // first compute triangle normals
  int totalNumTriangles = samples.size()/3;
  VtArray<GfVec3f> triangleNormals;
  triangleNormals.resize(totalNumTriangles);

  for(int i = 0; i < totalNumTriangles; ++i)
  {
    GfVec3f ab = positions[samples[i*3+1]] - positions[samples[i*3]];
    GfVec3f ac = positions[samples[i*3+2]] - positions[samples[i*3]];
    triangleNormals[i] = (ab ^ ac).GetNormalized();
  }

  // then polygons normals
  int numPolygons = counts.size();
  VtArray<GfVec3f> polygonNormals;
  polygonNormals.resize(numPolygons);
  int base = 0;
  for(int i=0; i < counts.size(); ++i)
  {
    int numVertices = counts[i];
    int numTriangles = numVertices - 2;
    GfVec3f n(0.f, 0.f, 0.f);
    for(int j = 0; j < numTriangles; ++j)
    {
      n += triangleNormals[base + j];
    }
    polygonNormals[i] = n.GetNormalized();
    base += numTriangles;
  }

  // finaly average vertex normals  
  base = 0;
  for(int i = 0; i < counts.size(); ++i)
  {
    int numVertices = counts[i];
    for(int j = 0; j < numVertices; ++j)
    {
      normals[indices[base + j]] += polygonNormals[i];
    }
    base += numVertices;
  }
  
  for(auto& n: normals) n.Normalize();
  
}

void 
UsdNprComputeTriangleNormals( const VtArray<GfVec3f>& positions,
                              const VtArray<int>& samples,
                              VtArray<GfVec3f>& normals)
{
  int totalNumTriangles = samples.size()/3;
  normals.resize(totalNumTriangles);

  for(int i = 0; i < totalNumTriangles; ++i)
  {
    GfVec3f ab = positions[samples[i*3+1]] - positions[samples[i*3]];
    GfVec3f ac = positions[samples[i*3+2]] - positions[samples[i*3]];
    normals[i] = (ab ^ ac).GetNormalized();
  }
}

PXR_NAMESPACE_CLOSE_SCOPE
