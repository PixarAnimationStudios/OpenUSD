//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_VERTEX_ARRAY_H
#define PXR_IMAGING_PLUGIN_LOFI_VERTEX_ARRAY_H

#include "pxr/pxr.h"
//#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
//#include "pxr/imaging/plugin/LoFi/elementBuffer.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"


PXR_NAMESPACE_OPEN_SCOPE


/// \class LoFiVertexBuffer
///
///
class LoFiVertexBuffer
{
public:
  // constructor
  LoFiVertexBuffer();

  // destructor
  ~LoFiVertexBuffer();

  // allocate
  void Reallocate();

  // get GL VAO
  GLuint Get(){return _vao;};

  void Bind();
  void Unbind();

private:
  // datas
  GLuint                            _vao;
  std::vector<GLuint>               _vbos;
  GLuint                            _ebo;
  //std::vector<LoFiVertexBuffer*>    _vbos;
  //std::vector<LoFiElementBuffer*>   _ebos;

  // prims
  std::vector<HdRprim*>             _prims;

  // flags
  bool                              _hasColors;
  bool                              _hasUVs;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
