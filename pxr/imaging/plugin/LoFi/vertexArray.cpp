//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// constructor
LoFiVertexArray::LoFiVertexArray()
{
  _channels = CHANNEL_NONE;
}

// destructor
LoFiVertexArray::~LoFiVertexArray()
{

}

// allocate
void 
LoFiVertexArray::Reallocate()
{

}

// allocate
void 
LoFiVertexArray::Bind()
{

}

void 
LoFiVertexArray::Unbind()
{
  
}

bool 
LoFiVertexArray::HaveBuffer(LoFiVertexBufferChannelBits channel)
{
  if(_vbos.find(channel) != _vbos.end())return true;
  else return false;
}

LoFiVertexBufferSharedPtr 
LoFiVertexArray::GetBuffer(LoFiVertexBufferChannelBits channel)
{
  return _vbos[channel];
}

void 
LoFiVertexArray::SetBuffer(LoFiVertexBufferChannelBits channel,
  LoFiVertexBufferSharedPtr buffer)
{
  _vbos[channel] = buffer;
}

LoFiVertexBufferSharedPtr 
LoFiVertexArray::CreateBuffer(LoFiVertexBufferChannelBits channel, 
  uint32_t numInputElements, uint32_t numOutputElements)
{
  return LoFiVertexBufferSharedPtr(
    new LoFiVertexBuffer(channel, numInputElements, numOutputElements)
  );
}

PXR_NAMESPACE_CLOSE_SCOPE
