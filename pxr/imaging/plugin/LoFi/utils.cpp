#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/plugin/LoFi/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

uint32_t LOFI_GL_VERSION = 0;

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

void 
LoFiComputeVertexColors(  const VtArray<GfVec3f>& positions,
                          VtArray<GfVec3f>& colors)
{
  // we want smooth vertex normals
  colors.resize(positions.size());
  memset(colors.data(), 0.f, colors.size() * sizeof(GfVec3f));

  // set random color per vertex  
  for(int i = 0; i < colors.size(); ++i)
  {
    colors[i] = GfVec3f(
      (float)rand()/(float)RAND_MAX,
      (float)rand()/(float)RAND_MAX,
      (float)rand()/(float)RAND_MAX
    );
  }
}

void
LoFiCurvesAdjacency( const VtArray<int>& curveVertexCount,
                      const size_t numControlPoints,
                      VtArray<int>& samples)
{
  size_t sizeAdjacency = (numControlPoints - curveVertexCount.size()) * 4;
  samples.resize(sizeAdjacency);
  size_t baseIdx = 0;
  size_t sampleIdx = 0;
  for(const auto& cnt: curveVertexCount) {
    size_t numSegments = cnt-1;
    size_t first = sampleIdx;
    size_t last = sampleIdx + numSegments * 4 - 1;
    for(size_t seg = 0; seg < numSegments; ++seg) {
      samples[sampleIdx++] = baseIdx  + seg - 1;
      samples[sampleIdx++] = baseIdx  + seg    ;
      samples[sampleIdx++] = baseIdx  + seg + 1;
      samples[sampleIdx++] = baseIdx  + seg + 2;
    }
    samples[first] = baseIdx;
    samples[last] = baseIdx + numSegments;
    baseIdx += cnt;
  }
}

void
LoFiCurvesSegments( const VtArray<int>& curveVertexCount,
                    const size_t numControlPoints,
                    VtArray<int>& samples)
{
  size_t sizeSegments = (numControlPoints - curveVertexCount.size()) * 2;
  samples.resize(sizeSegments);
  size_t baseIdx = 0;
  size_t sampleIdx = 0;
  for(const auto& cnt: curveVertexCount) {
    size_t numSegments = cnt-1;
    for(size_t seg = 0; seg < numSegments; ++seg) {
      samples[sampleIdx++] = baseIdx  + seg;
      samples[sampleIdx++] = baseIdx  + seg + 1;
    }
    baseIdx += cnt;
  }
}

void 
LoFiComputeCurveNormals(const VtArray<GfVec3f>& positions,
                          const VtArray<int>& curveVertexCounts,
                          const VtArray<int>& samples,
                          VtArray<GfVec3f>& normals)
{
  normals.resize(positions.size());
  for(auto& normal: normals)normal = pxr::GfVec3f(1.f,0.f,0.f);
}

PXR_NAMESPACE_CLOSE_SCOPE
