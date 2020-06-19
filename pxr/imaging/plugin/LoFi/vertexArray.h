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
#include "pxr/imaging/plugin/LoFi/topology.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"


PXR_NAMESPACE_OPEN_SCOPE

extern uint32_t LOFI_GL_VERSION;

class LoFiVertexBuffer;

typedef std::shared_ptr<class LoFiVertexArray> LoFiVertexArraySharedPtr;

/// \class LoFiVertexArray
///
///
class LoFiVertexArray
{
public:
  // constructor
  LoFiVertexArray(LoFiTopology::Type type);

  // destructor
  ~LoFiVertexArray();

  // get GL VAO
  GLuint Get() const {return _vao;};

  // topology
  LoFiTopology* GetTopology(){return _topology;};

  // buffers
  bool HasBuffer(LoFiAttributeChannel channel);
  LoFiVertexBufferSharedPtr GetBuffer(LoFiAttributeChannel channel);
  void SetBuffer(LoFiAttributeChannel channel, LoFiVertexBufferSharedPtr buffer);

  static LoFiVertexBufferSharedPtr 
  CreateBuffer( LoFiTopology* topo,
                LoFiAttributeChannel channel, 
                uint32_t numInputElements, 
                uint32_t numOutputElements,
                HdInterpolation interpolation,
                const std::string& name="Buffer");

  // channels
  inline void SetHaveChannel(LoFiAttributeChannel channel) { 
    _channels |= channel;
  };
  inline bool GetHaveChannel(LoFiAttributeChannel channel) { 
    return ((_channels & channel) == channel);
  };

  // state
  inline bool GetNeedUpdate(){return _needUpdate;};
  inline void SetNeedUpdate(bool needUpdate) {
    _needUpdate = needUpdate;
  };
  void UpdateState();

  // elements
  inline uint32_t GetNumElements() const{return _numElements;};
  inline void SetNumElements(uint32_t numElements){_numElements = numElements;};

  // adjacency
  void UseAdjacency(){_adjacency=true;};

  // allocate
  void Populate();
  void Bind() const;
  void Unbind() const;

  // draw
  void Draw() const;

private:
  // datas
  LoFiTopology*                     _topology;
  LoFiVertexBufferSharedPtrMap      _buffers;
  GLuint                            _vao;
  bool                              _adjacency;

  // flags
  uint32_t                          _channels;
  uint32_t                          _numElements;
  bool                              _needUpdate;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
