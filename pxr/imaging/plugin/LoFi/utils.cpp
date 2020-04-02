#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

#ifdef __APPLE__
extern uint32_t LOFI_GL_VERSION = 0;
#endif
int 
LoFiTriangulateMesh(const VtArray<int>& counts, 
                    const VtArray<int>& indices, 
                    VtArray<GfVec3i>& samples)
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
      samples[cnt++] = GfVec3i(indices[base], prim, base);
      samples[cnt++] = GfVec3i(indices[base+i], prim, base + i);
      samples[cnt++] = GfVec3i(indices[base+i+1], prim, base + i + 1);
    }
    prim++;
    base += count;
  }
  return cnt / 3;
}

void 
LoFiComputeVertexNormals( const VtArray<GfVec3f>& positions,
                          const VtArray<int>& counts,
                          const VtArray<int>& indices,
                          const VtArray<GfVec3i>& samples,
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
    GfVec3f ab = positions[samples[i*3+1][0]] - positions[samples[i*3][0]];
    GfVec3f ac = positions[samples[i*3+2][0]] - positions[samples[i*3][0]];
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


PXR_NAMESPACE_CLOSE_SCOPE
