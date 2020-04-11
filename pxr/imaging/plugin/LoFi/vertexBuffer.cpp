//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

// constructor
LoFiVertexBuffer::LoFiVertexBuffer(LoFiAttributeChannel channel,
  uint32_t numInputElements, uint32_t numOutputElements, HdInterpolation interpolation)
  : _channel(channel)
  , _hash(0)
  , _key(0)
  , _numInputElements(numInputElements)
  , _numOutputElements(numOutputElements) 
  , _needReallocate(true)
  , _needUpdate(true)
  , _interpolation(interpolation)
  , _vbo(0)
{
  switch(channel)
  {
    case CHANNEL_POSITION:
    case CHANNEL_NORMAL:
    case CHANNEL_COLOR:
      _elementSize = sizeof(GfVec3f);
      break;
    case CHANNEL_UV:
      _elementSize = sizeof(GfVec2f);
      break;
    default:
      _elementSize = 1;
  }
}

// destructor
LoFiVertexBuffer::~LoFiVertexBuffer()
{
  TF_STATUS(TfStringPrintf("DELETE LOFI VERTEX BUFFER %d", _vbo));
  if(_vbo)glDeleteBuffers(1, &_vbo);
}

size_t LoFiVertexBuffer::ComputeKey(const SdfPath& id)
{
  _key = id.GetHash();
  boost::hash_combine(_key, _channel);
  boost::hash_combine(_key, _numInputElements);
  boost::hash_combine(_key, _elementSize);
  return _key;
}

size_t LoFiVertexBuffer::ComputeHash(const char* datas)
{
  _hash = ArchHash(datas, _numInputElements * _elementSize);
  boost::hash_combine(_hash, _key);
  return _hash;
}

// allocate
void LoFiVertexBuffer::Reallocate()
{
  if(!_vbo)
  {
    glGenBuffers(1, &_vbo);
    TF_STATUS(TfStringPrintf("CREATE NEW VERTEX BUFFER OBJECT %d", _vbo));
  } 

  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(
    GL_ARRAY_BUFFER, 
    _numOutputElements * _elementSize, 
    NULL, 
    GL_DYNAMIC_DRAW
  );
  glVertexAttribPointer(_channel, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(_channel);
  _needReallocate = false;
}

void LoFiVertexBuffer::Populate(const void* datas)
{
  
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferSubData(
    GL_ARRAY_BUFFER, 
    0, 
    _numOutputElements * _elementSize,
    datas
  );
  _needUpdate = false;
}

void LoFiVertexBuffer::ComputeOutputDatas(const GfVec3i* samples,
                                          char* result)
{
  switch(_interpolation) 
  {
    case LoFiInterpolationConstant:
    {
      for(size_t i=0;i<_numOutputElements;++i) 
      {
        memcpy(result + i * _elementSize, _rawInputDatas, _elementSize);
      }
      break;
    }
  
    case LoFiInterpolationUniform:
    {
      for(size_t i=0;i<_numOutputElements;++i) 
      {
        memcpy(
          (void*)(result + i * _elementSize),
          (void*)(_rawInputDatas + samples[i][1] * _elementSize), 
          _elementSize
        );
      }
      break;
    }

    case LoFiInterpolationVarying:
    {
      for(size_t i=0;i<_numOutputElements;++i) 
      {
        memcpy(
          (void*)(result + i * _elementSize),
          (void*)(_rawInputDatas + samples[i][0] * _elementSize), 
          _elementSize
        );
      }
      break;
    }
      
    case LoFiInterpolationVertex:
    {
      for(size_t i=0;i<_numOutputElements;++i) 
      {
        memcpy(
          (void*)(result + i * _elementSize),
          (void*)(_rawInputDatas + samples[i][0] * _elementSize), 
          _elementSize
        );
      }
      break;
    }
      
    case LoFiInterpolationFaceVarying:
    {
      for(size_t i=0;i<_numOutputElements;++i) 
      {
        memcpy(
          (void*)(result + i * _elementSize),
          (void*)(_rawInputDatas + samples[i][2] * _elementSize), 
          _elementSize
        );
      }  
      break;
    } 
  }
}


PXR_NAMESPACE_CLOSE_SCOPE
