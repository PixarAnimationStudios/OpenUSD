//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_VERTEX_ARRAY_H
#define PXR_IMAGING_PLUGIN_LOFI_VERTEX_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"


PXR_NAMESPACE_OPEN_SCOPE

class LoFiVertexBuffer;

typedef boost::shared_ptr<class LoFiVertexArray> LoFiVertexArraySharedPtr;

/// \class LoFiVertexArray
///
///
class LoFiVertexArray
{
public:
  // constructor
  LoFiVertexArray();

  // destructor
  ~LoFiVertexArray();

  // allocate
  void Reallocate();

  // get GL VAO
  GLuint Get(){return _vao;};

  // buffers
  bool HaveBuffer(LoFiVertexBufferChannelBits channel);
  LoFiVertexBufferSharedPtr GetBuffer(LoFiVertexBufferChannelBits channel);

  void SetBuffer(LoFiVertexBufferChannelBits channel,
    LoFiVertexBufferSharedPtr buffer);

  static LoFiVertexBufferSharedPtr 
  CreateBuffer( LoFiVertexBufferChannelBits channel, 
                uint32_t numInputElements, 
                uint32_t numOutputElements);

  // channels
  inline void SetHaveChannel(LoFiVertexBufferChannelBits channel) { 
    _channels |= channel;
  };
  inline bool HaveChannel(LoFiVertexBufferChannelBits channel) { 
    return _channels & channel;}
  ;

  // elements
  inline uint32_t GetNumElements(){return _numElements;};
  inline void SetNumElements(uint32_t numElements){_numElements = numElements;};
  
  void Bind();
  void Unbind();

private:
  // datas
  LoFiVertexBufferSharedPtrMap      _vbos;
  GLuint                            _vao;
  GLuint                            _ebo;

  // flags
  uint32_t                          _channels;
  uint32_t                          _numElements;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
