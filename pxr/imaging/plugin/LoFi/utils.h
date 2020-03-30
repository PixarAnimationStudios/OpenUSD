#pragma once

#include <iostream>
#include "pxr/pxr.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/bboxCache.h"

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE

// check opengl error
//----------------------------------------------------------------------------
static bool 
LoFiGLCheckError(const char* message)
{
  GLenum err = glGetError();
  if(err)
  {
    if(!TfDebug::IsEnabled(LOFI_ERROR))return true;
    while(err != GL_NO_ERROR)
    {
      switch(err)
      {
        case GL_INVALID_OPERATION:
          std::cerr << "[OpenGL Error] " << message 
            << " INVALID_OPERATION" << std::endl;
          break;
        case GL_INVALID_ENUM:
          std::cerr << "[OpenGL Error] " << message 
            << " INVALID_ENUM" << std::endl;
          break;
        case GL_INVALID_VALUE:
          std::cerr << "[OpenGL Error] " << message 
            << " INVALID_VALUE" << std::endl;
          break;
        case GL_OUT_OF_MEMORY:
          std::cerr << "[OpenGL Error] " << message 
            << " OUT_OF_MEMORY" << std::endl;
          break;
        case GL_STACK_UNDERFLOW:
          std::cerr << "[OpenGL Error] " << message 
            << "STACK_UNDERFLOW" <<std::endl;
          break;
        case GL_STACK_OVERFLOW:
          std::cerr << "[OpenGL Error] " << message 
            << "STACK_OVERFLOW" << std::endl;
          break;
        default:
          std::cerr << "[OpenGL Error] " << message 
            << " UNKNOWN_ERROR" << std::endl;
          break;
      }
      err = glGetError();
    }
    return true;
  }
  return false;
}

static void 
LoFiGLFlushError()
{
  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR){};
}

/// Compute smooth vertex normals on a triangulated polymesh
void 
LoFiComputeVertexNormals(const VtArray<GfVec3f>& positions,
                          const VtArray<int>& counts,
                          const VtArray<int>& indices,
                          const VtArray<int>& triangles,
                          VtArray<GfVec3f>& normals);
/*
void 
LoFiComputeVertexColors(const VtArray<GfVec3f>& positions,
                        const VtArray<int>& counts,
                        const VtArray<int>& indices,
                        const VtArray<int>& triangles,
                        GfVec3f color,
                        VtArray<GfVec3f>& normals);

void 
LoFiComputeVertexColors(const VtArray<GfVec3f>& positions,
                        const VtArray<int>& counts,s
                        const VtArray<int>& indices,
                        GfVec3f color,
                        VtArray<GfVec3f>& normals);
*/

/// Triangulate a polygonal mesh
int 
LoFiTriangulateMesh(const VtArray<int>& counts, 
                    const VtArray<int>& indices, 
                    VtArray<int>& triangles,
                    VtArray<int>& samples);

/// Triangulate mesh datas
template<typename T>
void 
LoFiTriangulateData(const VtArray<int>& indices, 
                    const VtArray<T>& datas,
                    VtArray<T>& result);

PXR_NAMESPACE_CLOSE_SCOPE