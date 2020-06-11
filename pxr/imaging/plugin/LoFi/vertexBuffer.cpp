//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/plugin/LoFi/utils.h"
#include "pxr/imaging/plugin/LoFi/topology.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

// constructor
LoFiVertexBuffer::LoFiVertexBuffer(LoFiTopology* topo, LoFiAttributeChannel channel,
  uint32_t numInputElements, uint32_t numOutputElements, HdInterpolation interpolation)
  : _topology(topo)
  , _channel(channel)
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
    case CHANNEL_TANGENT:
    case CHANNEL_COLOR:
      _tuppleSize = 3;
      _elementSize = sizeof(GfVec3f);
      break;
    case CHANNEL_UV:
      _tuppleSize = 2;
      _elementSize = sizeof(GfVec2f);
      break;
    case CHANNEL_WIDTH:
      _tuppleSize = 1;
      _elementSize = sizeof(float);
      break;  
    default:
      _tuppleSize = 1;
      _elementSize = sizeof(float);
      break;
  }
}

// destructor
LoFiVertexBuffer::~LoFiVertexBuffer()
{
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
  if(_needReallocate)
  {
    if(!_vbo)
    {
      glGenBuffers(1, &_vbo);
    } 

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    glBufferData(
      GL_ARRAY_BUFFER, 
      _numOutputElements * _elementSize, 
      NULL, 
      GL_DYNAMIC_DRAW
    );

    _needReallocate = false;
  }
}

void LoFiVertexBuffer::Populate()
{
  if(_needUpdate)
  {
    VtArray<char> datas(ComputeOutputSize());
    ComputeOutputDatas(_topology, datas.data());

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferSubData(
      GL_ARRAY_BUFFER, 
      0, 
      _numOutputElements * _elementSize,
      datas.cdata()
    );

    _needUpdate = false;
  }
}

void LoFiVertexBuffer::Bind()
{
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glVertexAttribPointer(_channel, _tuppleSize, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(_channel);
}

void LoFiVertexBuffer::ComputeOutputDatas(const LoFiTopology* topo,
                                          char* result)
{
  if(topo->type == LoFiTopology::Type::POINTS)
  {
    memcpy(result, _rawInputDatas, _numInputElements * _elementSize);
  }
  else if(topo->type == LoFiTopology::Type::LINES)
  {
    switch(_interpolation) {
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
        size_t curveIdx = 0;
        size_t sampleIdx = 0;
        size_t offsetIdx = 0;
        const int* samples = topo->samples;
        for(size_t i=0;i<topo->numBases;++i) 
        {
          size_t numSegments = 1;
          while(samples[sampleIdx+2] != samples[sampleIdx+3]) {
            numSegments++;
            sampleIdx+=4;
          }
          numSegments++;

          for(size_t j=0;j<numSegments;++j) 
          {
            memcpy(
              (void*)(result + (offsetIdx + j) * _elementSize), 
              (void*)(_rawInputDatas + curveIdx * _elementSize),
              _elementSize);
          }

          offsetIdx += numSegments;
          sampleIdx += 4;
          curveIdx++;
        }
        break;
      }
      case LoFiInterpolationVertex:
      default:
      {
        memcpy(result, _rawInputDatas, _numInputElements * _elementSize);
      }
    }
  }
  else if(topo->type == LoFiTopology::Type::TRIANGLES)
  {
    const int* samples = topo->samples;
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
            (void*)(_rawInputDatas + samples[i * 3 + 1] * _elementSize), 
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
            (void*)(_rawInputDatas + samples[i * 3] * _elementSize), 
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
            (void*)(_rawInputDatas + samples[i * 3] * _elementSize), 
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
            (void*)(_rawInputDatas + samples[i * 3 + 2] * _elementSize), 
            _elementSize
          );
        }  
        break;
      } 
    }
  }
  
}


PXR_NAMESPACE_CLOSE_SCOPE
