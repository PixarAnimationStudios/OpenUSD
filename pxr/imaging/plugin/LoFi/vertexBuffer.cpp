//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// constructor
LoFiVertexBuffer::LoFiVertexBuffer(LoFiVertexBufferChannelBits channel,
  uint32_t numInputElements, uint32_t numOutputElements)
  : _channel(channel)
  , _datasHash(0)
  , _registryKey(0)
  , _numInputElements(numInputElements)
  , _numOutputElements(numOutputElements) 
  , _needReallocate(true)
  , _needUpdate(true)
  , _interpolation(HdInterpolationConstant)
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
  std::cerr << "DELETE FUCKIN BUFFER: ";
  switch(_channel)
  {
    case CHANNEL_POSITION:
      std::cout << "CHANNEL POSITION" << std::endl;
      break;
    case CHANNEL_NORMAL:
      std::cout << "CHANNEL NORMAL" << std::endl;
      break;
    case CHANNEL_COLOR:
      std::cout << "CHANNEL COLOR" << std::endl;
      break;
    case CHANNEL_UVS:
      std::cout << "CHANNEL UVS" << std::endl;
      break;
  }
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

}

// allocate
void LoFiVertexBuffer::Bind()
{

}

void LoFiVertexBuffer::Unbind()
{
  
}

PXR_NAMESPACE_CLOSE_SCOPE
