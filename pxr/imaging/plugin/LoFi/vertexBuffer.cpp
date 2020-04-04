//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"

PXR_NAMESPACE_OPEN_SCOPE

// constructor
LoFiVertexBuffer::LoFiVertexBuffer(LoFiVertexBufferChannel channel,
  uint32_t numInputElements, uint32_t numOutputElements)
  : _channel(channel)
  , _datasHash(0)
  , _registryKey(0)
  , _numInputElements(numInputElements)
  , _numOutputElements(numOutputElements) 
  , _needReallocate(true)
  , _needUpdate(true)
  , _interpolation(HdInterpolationConstant)
  , _vbo(0)
{
  switch(channel)
  {
    case CHANNEL_POSITION:
    case CHANNEL_NORMAL:
    case CHANNEL_COLOR:
      _elementSize = sizeof(GfVec3f);
      break;
    case CHANNEL_UVS:
      _elementSize = sizeof(GfVec2f);
      break;
    default:
      _elementSize = 1;
  }
}

// destructor
LoFiVertexBuffer::~LoFiVertexBuffer()
{
  if(_vbo)glDeleteBuffers(1, &_vbo);
}

size_t LoFiVertexBuffer::ComputeDatasHash(const char* datas)
{
  size_t hash = 0;
  hash = ArchHash(datas, _numInputElements * _elementSize);
  boost::hash_combine(hash, _channel);
  boost::hash_combine(hash, _numInputElements);
  boost::hash_combine(hash, _elementSize);
  return hash;
}

size_t LoFiVertexBuffer::ComputeRegistryKey()
{
  size_t key = 0;
  boost::hash_combine(key, _datasHash);
  boost::hash_combine(key, _numOutputElements);
  return key;
}

// allocate
void LoFiVertexBuffer::Reallocate()
{
  if(!_vbo) glGenBuffers(1, &_vbo);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(
    GL_ARRAY_BUFFER, 
    _numOutputElements * _elementSize, 
    NULL, 
    GL_DYNAMIC_DRAW
  );
  glVertexAttribPointer(_channel, 3, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(_channel);
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
