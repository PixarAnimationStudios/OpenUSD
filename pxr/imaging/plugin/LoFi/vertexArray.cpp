//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// constructor
LoFiVertexArray::LoFiVertexArray(LoFiTopology::Type type)
: _vao(0)
, _channels(0)
, _needUpdate(true)
, _adjacency(false)
{
  if(type == LoFiTopology::LINES) {
    _topology = new LoFiCurvesTopology();
  }
  else _topology = new LoFiTopology();
  _topology->type = type;
}

// destructor
LoFiVertexArray::~LoFiVertexArray()
{
  _buffers.clear();
  delete(_topology);
  if(_vao)glDeleteVertexArrays(1, &_vao);
}

// state
void 
LoFiVertexArray::UpdateState()
{
  for(auto elem: _buffers)
  {
    LoFiVertexBufferSharedPtr buffer = elem.second;
    if(buffer->GetNeedUpdate())
    {
      _needUpdate = true;
      break;
    }
  }
}

// populate
void 
LoFiVertexArray::Populate()
{
  if(!_vao)
  {
    #ifdef __APPLE__
      if(LOFI_GL_VERSION >= 330) glGenVertexArrays(1, &_vao);
      else glGenVertexArraysAPPLE(1, &_vao);
    #else
      glGenVertexArrays(1, &_vao);
    #endif
  }
  Bind();
  
  for(auto& elem: _buffers)
  {
    LoFiVertexBufferSharedPtr buffer = elem.second;
    buffer->Bind();
  }
  Unbind();
  _needUpdate = false;
}

// draw
void
LoFiVertexArray::Draw() const
{
  Bind();
  switch(_topology->type)
  {
    case LoFiTopology::Type::POINTS:
      glDrawArrays(GL_POINTS, 0, _numElements);
      break;
    case LoFiTopology::Type::LINES:
      if(_adjacency)
        glDrawArrays(GL_LINES_ADJACENCY, 0, _numElements);
      else 
        glDrawArrays(GL_LINES, 0, _numElements);
      break;
    case LoFiTopology::Type::TRIANGLES:
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES, 0, _numElements);
      break;
  }
  
  Unbind();
}

// allocate
void 
LoFiVertexArray::Bind() const
{
  glPointSize(5);
  #ifdef __APPLE__
    if(LOFI_GL_VERSION >= 330) glBindVertexArray(_vao);
    else glBindVertexArrayAPPLE(_vao);
  #else
    glBindVertexArray(_vao);
  #endif
}

void 
LoFiVertexArray::Unbind() const 
{
  #ifdef __APPLE__
    if(LOFI_GL_VERSION >= 330) glBindVertexArray(0);
    else glBindVertexArrayAPPLE(0);
  #else
    glBindVertexArray(0);
  #endif
}

bool 
LoFiVertexArray::HasBuffer(LoFiAttributeChannel channel)
{
  if(_buffers.find(channel) != _buffers.end())return true;
  else return false;
}

LoFiVertexBufferSharedPtr
LoFiVertexArray::GetBuffer(LoFiAttributeChannel channel)
{
  return _buffers[channel];
}

void 
LoFiVertexArray::SetBuffer(LoFiAttributeChannel channel, 
  LoFiVertexBufferSharedPtr buffer)
{
  _buffers[channel] = buffer;
}

LoFiVertexBufferSharedPtr 
LoFiVertexArray::CreateBuffer(LoFiTopology* topo, LoFiAttributeChannel channel, 
  uint32_t numInputElements, uint32_t numOutputElements, 
  HdInterpolation interpolation, const std::string& name)
{
  return LoFiVertexBufferSharedPtr(
    new LoFiVertexBuffer(topo, channel, numInputElements, numOutputElements, 
      interpolation, name)
  );
}

PXR_NAMESPACE_CLOSE_SCOPE
