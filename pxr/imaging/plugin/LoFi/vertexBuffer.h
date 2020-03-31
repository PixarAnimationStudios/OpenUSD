//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_VERTEX_BUFFER_H
#define PXR_IMAGING_PLUGIN_LOFI_VERTEX_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/base/arch/hash.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include <boost/functional/hash.hpp>


PXR_NAMESPACE_OPEN_SCOPE


enum LoFiVertexBufferChannelBits : short {
  CHANNEL_NONE      = 0,
  CHANNEL_POSITION  = 1,
  CHANNEL_NORMAL    = 2,
  CHANNEL_COLOR     = 4,
  CHANNEL_UVS       = 8,
};


typedef boost::shared_ptr<class LoFiVertexBuffer> LoFiVertexBufferSharedPtr;
typedef std::vector<LoFiVertexBufferSharedPtr> LoFiVertexBufferSharedPtrList;
typedef std::map<LoFiVertexBufferChannelBits, LoFiVertexBufferSharedPtr> 
  LoFiVertexBufferSharedPtrMap;

/// \class LoFiVertexBuffer
///
///
class LoFiVertexBuffer
{
public:
  // constructor
  LoFiVertexBuffer( LoFiVertexBufferChannelBits channel, 
                    uint32_t numInputElements,
                    uint32_t numOutputElements);

  // destructor
  ~LoFiVertexBuffer();

  // allocate
  void Reallocate();

  // get opengl vertex buffer object
  GLuint Get(){return _vbo;};

  // hash
  size_t ComputeDatasHash(const char* datas);
  inline size_t GetDatasHash(){return _datasHash;};
  inline void SetDatasHash(size_t hash){ _datasHash = hash;};

  // registry key
  size_t ComputeRegistryKey();
  inline size_t GetRegistryKey(){return _registryKey;};
  inline void SetRegistryKey(size_t key){_registryKey = key;};

  // state
  inline bool GetNeedReallocate(){return _needReallocate;};
  inline void SetNeedReallocate(bool needReallocate) {
    _needReallocate = needReallocate;
  };
  inline bool GetNeedUpdate(){return _needUpdate;};
  inline void SetNeedUpdate(bool needUpdate) {
    _needUpdate = needUpdate;
  };

  void Bind();
  void Unbind();

private: 
  // description
  short                             _channel;
  size_t                            _datasHash;
  size_t                            _registryKey;
  uint32_t                          _numInputElements;
  uint32_t                          _numOutputElements;
  uint32_t                          _elementSize;
  bool                              _needReallocate;
  bool                              _needUpdate;
  HdInterpolation                   _interpolation;

  // opengl
  GLuint                            _vbo;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_VERTEX_BUFFER_H
