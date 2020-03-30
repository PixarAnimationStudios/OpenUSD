#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

int 
LoFiTriangulateMesh(const pxr::VtArray<int>& counts, 
                const pxr::VtArray<int>& indices, 
                pxr::VtArray<int>& triangles,
                pxr::VtArray<int>& samples)
{
  int num_triangles = 0;
  for(auto count : counts)
  {
    num_triangles += count - 2;
  }

  triangles.resize(num_triangles * 3);
  samples.resize(num_triangles * 3);

  int base = 0;
  int cnt = 0;
  for(auto count: counts)
  {
    for(int i = 1; i < count - 1; ++i)
    {
      triangles[cnt] = indices[base        ];
      samples[cnt++] = base;
      triangles[cnt] = indices[base + i    ];
      samples[cnt++] = base + i;
      triangles[cnt] = indices[base + i + 1];
      samples[cnt++] = base + i + 1;
    }
    base += count;
  }
  return cnt / 3;
}

void 
LoFiComputeVertexNormals( const pxr::VtArray<pxr::GfVec3f>& positions,
                          const pxr::VtArray<int>& counts,
                          const pxr::VtArray<int>& indices,
                          const pxr::VtArray<int>& triangles,
                          pxr::VtArray<pxr::GfVec3f>& normals)
{
  // we want smooth vertex normals
  normals.resize(positions.size());

  // first compute triangle normals
  int totalNumTriangles = triangles.size()/3;
  pxr::VtArray<pxr::GfVec3f> triangleNormals;
  triangleNormals.resize(totalNumTriangles);

  for(int i = 0; i < totalNumTriangles; ++i)
  {
    pxr::GfVec3f ab = positions[triangles[i*3+1]] - positions[triangles[i*3]];
    pxr::GfVec3f ac = positions[triangles[i*3+2]] - positions[triangles[i*3]];
    triangleNormals[i] = ab ^ ac;
    triangleNormals[i].Normalize();
  }

  // then polygons normals
  int numPolygons = counts.size();
  pxr::VtArray<pxr::GfVec3f> polygonNormals;
  polygonNormals.resize(numPolygons);
  int base = 0;
  for(int i=0; i < counts.size(); ++i)
  {
    int numVertices = counts[i];
    int numTriangles = numVertices - 2;
    pxr::GfVec3f n(0.f, 0.f, 0.f);
    for(int j = 0; j < numTriangles; ++j)
    {
      n += triangleNormals[base + j];
    }
    n.Normalize();
    polygonNormals[i] = n;
    base += numTriangles;
  }
  for(auto& n: polygonNormals) n.Normalize();

  // finaly average vertex normals
  for(auto& n : normals) n = pxr::GfVec3f(0.f, 0.f, 0.f);
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

template<typename T>
void 
LoFiTriangulateData(const VtArray<int>& indices, 
                    const VtArray<T>& datas,
                    VtArray<T>& result)
{
  result.resize(indices.size());
  for(int i=0;i<indices.size();++i)
  {
    result[i] = datas[indices[i]];
  }
};

PXR_NAMESPACE_CLOSE_SCOPE
