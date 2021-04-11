#pragma once

#include <iostream>
#include "pxr/pxr.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Triangulate a polygonal mesh
/// Samples store a GfVec3i X per triangle vertex where
///  - X[0] Vertex index on original topology
///  - X[1] Face index on original topology
///  - X[2] Sample index on original topology
int 
UsdNprTriangulateMesh(const VtArray<int>& counts, 
                    const VtArray<int>& indices, 
                    VtArray<int>& samples);

/// Compute smooth vertex normals on a triangulated polymesh
void 
UsdNprComputeNormals(const VtArray<GfVec3f>& positions,
                          const VtArray<int>& counts,
                          const VtArray<int>& indices,
                          const VtArray<int>& triangles,
                          VtArray<GfVec3f>& polygonNormals,
                          VtArray<GfVec3f>& vertexNormals);

/// Compute triangle normals
void 
UsdNprComputeTriangleNormals( const VtArray<GfVec3f>& positions,
                              const VtArray<int>& samples,
                              VtArray<GfVec3f>& normals);
                          
/// Triangulate data
template<typename T>
void
UsdNprTriangulateDatas( const VtArray<int>& samples,
                      const VtArray<T>& datas,
                      VtArray<T>& result)
{
  result.resize(samples.size());
  for(int i=0;i<samples.size();++i)
  {
    result[i] = datas[samples[i]];
  }
};

PXR_NAMESPACE_CLOSE_SCOPE