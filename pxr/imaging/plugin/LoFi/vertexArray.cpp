//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// constructor
LoFiVertexArray::LoFiVertexArray()
: _vao(0)
, _channels(0)
{
  GlfContextCaps const& caps = GlfContextCaps::GetInstance();
  _glVersion = caps.glVersion;
}

// destructor
LoFiVertexArray::~LoFiVertexArray()
{

}

// allocate
void 
LoFiVertexArray::Reallocate()
{
  if(!_vao)
  {
    #ifdef __APPLE__
      if(_glVersion >= 330) glGenVertexArrays(1, &_vao);
      else glGenVertexArraysAPPLE(1, &_vao);
    #else
      glGenVertexArrays(1, &_vao);
    #endif
  }
  Bind();

  for(auto elem: _buffers)
  {
    LoFiVertexBufferSharedPtr buffer = elem.second;
    buffer->Reallocate();
  }
  Unbind();
  //_needReallocate = false;
  //_needUpdate = true;
}

void 
LoFiVertexArray::Populate()
{

  Bind();
  
  for(auto elem: _buffers)
  {
    LoFiVertexBufferSharedPtr buffer = elem.second;
    if(buffer->GetNeedUpdate())
    {
      VtArray<char> datas(buffer->ComputeOutputSize());
      buffer->ComputeOutputDatas(_topology, datas.data());
      buffer->Populate(datas.cdata());
    }
    
  }

  Unbind();
}

// draw
void
LoFiVertexArray::Draw()
{
  Bind();
  glDrawArrays(GL_TRIANGLES, 0, _numElements);
  Unbind();
}

// allocate
void 
LoFiVertexArray::Bind() const
{
  #ifdef __APPLE__
    if(_glVersion >= 330) glBindVertexArray(_vao);
    else glBindVertexArrayAPPLE(_vao);
  #else
    glBindVertexArray(_vao);
  #endif
}

void 
LoFiVertexArray::Unbind() const 
{
  #ifdef __APPLE__
    if(_glVersion >= 330) glBindVertexArray(0);
    else glBindVertexArrayAPPLE(0);
  #else
    glBindVertexArray(0);
  #endif
}

bool 
LoFiVertexArray::HaveBuffer(LoFiVertexBufferChannel channel)
{
  if(_buffers.find(channel) != _buffers.end())return true;
  else return false;
}

LoFiVertexBufferSharedPtr 
LoFiVertexArray::GetBuffer(LoFiVertexBufferChannel channel)
{
  return _buffers[channel];
}

void 
LoFiVertexArray::SetBuffer(LoFiVertexBufferChannel channel,
  LoFiVertexBufferSharedPtr buffer)
{
  _buffers[channel] = buffer;
}

LoFiVertexBufferSharedPtr 
LoFiVertexArray::CreateBuffer(LoFiVertexBufferChannel channel, 
  uint32_t numInputElements, uint32_t numOutputElements)
{
  return LoFiVertexBufferSharedPtr(
    new LoFiVertexBuffer(channel, numInputElements, numOutputElements)
  );
}

PXR_NAMESPACE_CLOSE_SCOPE
