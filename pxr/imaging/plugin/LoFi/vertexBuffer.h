//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_VERTEX_BUFFER_H
#define PXR_IMAGING_PLUGIN_LOFI_VERTEX_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/base/arch/hash.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/plugin/LoFi/topology.h"

#include <boost/functional/hash.hpp>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

enum LoFiAttributeChannel : short {
  CHANNEL_POSITION,
  CHANNEL_NORMAL,
  CHANNEL_TANGENT,
  CHANNEL_ROTATION,
  CHANNEL_COLOR,
  CHANNEL_UV,
  CHANNEL_WIDTH,
  CHANNEL_ID,
  CHANNEL_SCALE,
  CHANNEL_SHAPE_POSITION,
  CHANNEL_SHAPE_NORMAL,
  CHANNEL_SHAPE_UV,
  CHANNEL_SHAPE_COLOR,
  CHANNEL_UNDEFINED
};

enum LoFiVertexBufferState : short {
  INVALID,
  TO_REALLOCATE,
  TO_UPDATE,
  TO_RECYCLE
};


typedef std::shared_ptr<class LoFiVertexBuffer> LoFiVertexBufferSharedPtr;
typedef std::vector<LoFiVertexBufferSharedPtr> LoFiVertexBufferSharedPtrList;
typedef std::map<LoFiAttributeChannel, LoFiVertexBufferSharedPtr> LoFiVertexBufferSharedPtrMap;

/// \class LoFiVertexBuffer
///
///
class LoFiVertexBuffer
{
public:
  // constructor
  LoFiVertexBuffer( LoFiAttributeChannel channel, 
                    uint32_t numInputElements,
                    uint32_t numOutputElements,
                    HdInterpolation interpolation);

  // destructor
  ~LoFiVertexBuffer();

  // hash
  size_t ComputeHash(const char* datas);
  inline size_t GetHash(){return _hash;};
  inline void SetHash(size_t hash){ _hash = hash;};

  // registry key
  size_t ComputeKey(const SdfPath& id);
  inline size_t GetKey(){return _key;};
  inline void SetKey(size_t key){_key = key;};

  // state
  inline bool GetNeedReallocate(){return _needReallocate;};
  inline void SetNeedReallocate(bool needReallocate) {
    _needReallocate = needReallocate;
  };
  inline bool GetNeedUpdate(){return _needUpdate;};
  inline void SetNeedUpdate(bool needUpdate) {
    _needUpdate = needUpdate;
  };
  inline bool IsValid(){return _valid;};
  inline void SetValid(bool valid) {_valid = valid;};

  inline HdInterpolation GetInterpolation(){return _interpolation;};
  inline void SetInterpolation(HdInterpolation interpolation) {
    _interpolation = interpolation;
  };

  // raw input datas
  inline const void* GetRawInputDatas(){return _rawInputDatas;};
  inline void SetRawInputDatas(const char* datas){_rawInputDatas = datas;};

  // compute output datas
  uint32_t GetNumOutputElements() {return _numOutputElements;};
  void SetNumOutputElements(uint32_t numOutputElements) {
    _numOutputElements = numOutputElements;
  };
  uint32_t ComputeOutputSize() {return _numOutputElements * _elementSize;};

  void ComputeOutputDatas(const LoFiTopology* topo,
                          char* outputDatas);

  // opengl
  void Reallocate();
  void Populate(const void* datas);
  GLuint Get() const {return _vbo;};

private: 
  // description
  std::string                       _name;
  short                             _channel;
  size_t                            _hash;
  size_t                            _key;
  uint32_t                          _numInputElements;
  uint32_t                          _numOutputElements;
  uint32_t                          _elementSize;
  uint32_t                          _tuppleSize;
  
  bool                              _needReallocate;
  bool                              _needUpdate;
  bool                              _valid;
  HdInterpolation                   _interpolation;

  // datas
  const char*                       _rawInputDatas;

  // opengl
  GLuint                            _vbo;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_VERTEX_BUFFER_H
