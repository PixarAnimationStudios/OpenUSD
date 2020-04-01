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
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


enum LoFiVertexBufferChannel : short {
  CHANNEL_POSITION,
  CHANNEL_NORMAL,
  CHANNEL_COLOR,
  CHANNEL_UVS
};

enum LoFiVertexBufferState : short {
  INVALID,
  TO_REALLOCATE,
  TO_UPDATE,
  TO_RECYCLE
};


typedef boost::shared_ptr<class LoFiVertexBuffer> LoFiVertexBufferSharedPtr;
typedef std::vector<LoFiVertexBufferSharedPtr> LoFiVertexBufferSharedPtrList;
typedef std::map<LoFiVertexBufferChannel, LoFiVertexBufferSharedPtr> 
  LoFiVertexBufferSharedPtrMap;

/// \class LoFiVertexBuffer
///
///
class LoFiVertexBuffer
{
public:
  // constructor
  LoFiVertexBuffer( LoFiVertexBufferChannel channel, 
                    uint32_t numInputElements,
                    uint32_t numOutputElements);

  // destructor
  ~LoFiVertexBuffer();

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
  uint64_t ComputeOutputSize() {return _numOutputElements * _elementSize;};
  void ComputeOutputDatas(const GfVec3i* samples,
                          char* outputDatas);

  // opengl
  void Reallocate();
  void Populate(const void* triangulatedDatas);
  GLuint Get() const {return _vbo;};

private: 
  // description
  std::string                       _name;
  short                             _channel;
  size_t                            _datasHash;
  size_t                            _registryKey;
  uint32_t                          _numInputElements;
  uint32_t                          _numOutputElements;
  uint32_t                          _elementSize;
  
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
