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

  // get GL VAO
  GLuint Get() const {return _vao;};

  // buffers
  bool HaveBuffer(LoFiVertexBufferChannel channel);
  LoFiVertexBufferSharedPtr GetBuffer(LoFiVertexBufferChannel channel);

  void SetBuffer(LoFiVertexBufferChannel channel,
    LoFiVertexBufferSharedPtr buffer);

  static LoFiVertexBufferSharedPtr 
  CreateBuffer( LoFiVertexBufferChannel channel, 
                uint32_t numInputElements, 
                uint32_t numOutputElements);

  // channels
  inline void SetHaveChannel(LoFiVertexBufferChannel channel) { 
    _channels |= channel;
  };
  inline bool HaveChannel(LoFiVertexBufferChannel channel) { 
    return ((_channels & channel) == channel);
  };

  // state
  inline bool NeedReallocate(){return true;};
  inline bool NeedUpdate(){return true;};

  // elements
  inline uint32_t GetNumElements() const{return _numElements;};
  inline void SetNumElements(uint32_t numElements){_numElements = numElements;};

  // topology
  inline void SetTopologyPtr(const GfVec3i* topology) {
    _topology = topology;
  }
  const GfVec3i* GetTopologyPtr() const {return _topology;};

  // allocate
  void Reallocate();
  void Populate();
  void Bind() const;
  void Unbind() const;

  // draw
  void Draw();

private:
  // datas
  LoFiVertexBufferSharedPtrMap      _buffers;
  GLuint                            _vao;
  GLuint                            _ebo;

  // flags
  uint32_t                          _channels;
  uint32_t                          _numElements;

  // topology
  const GfVec3i*                    _topology;

#ifdef __APPLE__      
  uint32_t                          _glVersion;
#endif
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
