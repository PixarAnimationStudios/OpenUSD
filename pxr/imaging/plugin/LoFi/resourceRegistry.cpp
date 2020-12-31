//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include <iostream>
#include "pxr/imaging/plugin/LoFi/debugCodes.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/timer.h"

#include "pxr/imaging/plugin/LoFi/shader.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiResourceRegistry::LoFiResourceRegistry(Hgi* hgi)
  : _hgi(hgi)
{
}

LoFiResourceRegistry::~LoFiResourceRegistry()
{
}

void LoFiResourceRegistry::InvalidateShaderRegistry()
{
    // Derived classes that hold shaders will override this,
    // but the base registry has nothing to do.
}

void
LoFiResourceRegistry::ReloadResource(
    TfToken const& resourceType,
    std::string const& path)
{
}

VtDictionary
LoFiResourceRegistry::GetResourceAllocation() const
{
    return VtDictionary();
}

Hgi*
LoFiResourceRegistry::GetHgi()
{
    return _hgi;
}

HgiBlitCmds*
LoFiResourceRegistry::GetGlobalBlitCmds()
{
    if (!_blitCmds) {
        _blitCmds = _hgi->CreateBlitCmds();
    }
    return _blitCmds.get();
}

template <typename ID, typename T>
HdInstance<T>
_Register(ID id, HdInstanceRegistry<T> &registry, TfToken const &lofiToken)
{
    HdInstance<T> instance = registry.GetInstance(id);
    if (instance.IsFirstInstance()) {
      HD_PERF_COUNTER_INCR(lofiToken);
    }
    return instance;
}

HdInstance<LoFiVertexArraySharedPtr>
LoFiResourceRegistry::RegisterVertexArray(
  HdInstance<LoFiVertexArraySharedPtr>::ID id)
{
  return _Register(id, _vertexArrayRegistry, LoFiRegistryTokens->vertexArray);
}

LoFiVertexArraySharedPtr
LoFiResourceRegistry::GetVertexArray(HdInstance<LoFiVertexArraySharedPtr>::ID id)
{
  bool found;
  auto instance = _vertexArrayRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return LoFiVertexArraySharedPtr(nullptr);
}

bool
LoFiResourceRegistry::HasVertexArray(HdInstance<LoFiVertexArraySharedPtr>::ID id)
{
  bool found = false;
  auto instance = _vertexArrayRegistry.FindInstance(id, &found);
  return found;
}

HdInstance<LoFiVertexBufferSharedPtr>
LoFiResourceRegistry::RegisterVertexBuffer(
  HdInstance<LoFiVertexBufferSharedPtr>::ID id)
{
  return _Register(id, _vertexBufferRegistry, LoFiRegistryTokens->vertexBuffer);
}

bool
LoFiResourceRegistry::HasVertexBuffer(HdInstance<LoFiVertexBufferSharedPtr>::ID id)
{
  bool found = false;
  auto instance = _vertexBufferRegistry.FindInstance(id, &found);
  return found;
}

LoFiVertexBufferSharedPtr
LoFiResourceRegistry::GetVertexBuffer(HdInstance<LoFiVertexBufferSharedPtr>::ID id)
{
  auto instance = _vertexBufferRegistry.GetInstance(id);
  return instance.GetValue();
}

HdInstance<LoFiGLSLShaderSharedPtr>
LoFiResourceRegistry::RegisterGLSLShader(
  HdInstance<LoFiGLSLShaderSharedPtr>::ID id)
{
  return _Register(id, _glslShaderRegistry, LoFiRegistryTokens->glslShader);
}

LoFiGLSLShaderSharedPtr
LoFiResourceRegistry::GetGLSLShader(HdInstance<LoFiVertexArraySharedPtr>::ID id)
{
  bool found = false;
  auto instance = _glslShaderRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return LoFiGLSLShaderSharedPtr(nullptr);
}

HdInstance<LoFiGLSLProgramSharedPtr>
LoFiResourceRegistry::RegisterGLSLProgram(
  HdInstance<LoFiGLSLProgramSharedPtr>::ID id)
{
  return _Register(id, _glslProgramRegistry, LoFiRegistryTokens->glslProgram);
}

LoFiGLSLProgramSharedPtr
LoFiResourceRegistry::GetGLSLProgram(HdInstance<LoFiGLSLProgramSharedPtr>::ID id)
{
  bool found = false;
  auto instance = _glslProgramRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return LoFiGLSLProgramSharedPtr(nullptr);
}

HdInstance<LoFiTextureResourceSharedPtr>
LoFiResourceRegistry::RegisterTextureResource(TextureKey id)
{
    return _textureResourceRegistry.GetInstance(id);
}

HdInstance<LoFiTextureResourceSharedPtr>
LoFiResourceRegistry::FindTextureResource(TextureKey id, bool *found)
{
    return _textureResourceRegistry.FindInstance(id, found);
}

HdInstance<LoFiTextureResourceHandleSharedPtr>
LoFiResourceRegistry::RegisterTextureResourceHandle(
        HdInstance<LoFiTextureResourceHandleSharedPtr>::ID id)
{
    return _textureResourceHandleRegistry.GetInstance(id);
}

HdInstance<LoFiTextureResourceHandleSharedPtr>
LoFiResourceRegistry::FindTextureResourceHandle(
        HdInstance<LoFiTextureResourceHandleSharedPtr>::ID id, bool *found)
{
    return _textureResourceHandleRegistry.FindInstance(id, found);
}

void
LoFiResourceRegistry::_Commit()
{
  for(auto& instance: _vertexBufferRegistry)
  {
    LoFiVertexBufferSharedPtr& vertexBuffer = instance.second.value;
    if(vertexBuffer->GetNeedReallocate())
    {
      vertexBuffer->Reallocate();
      vertexBuffer->Populate();
      TF_DEBUG(LOFI_REGISTRY).Msg("Reallocate Vertex Buffer : %s\n", vertexBuffer->GetName().c_str());
    }
    else if(vertexBuffer->GetNeedUpdate())
    {
      vertexBuffer->Populate();
      TF_DEBUG(LOFI_REGISTRY).Msg("Populate Vertex Buffer : %s\n", vertexBuffer->GetName().c_str());
    }
    
  }

  for(auto& instance: _vertexArrayRegistry)
  {
    LoFiVertexArraySharedPtr& vertexArray = instance.second.value;
    if(vertexArray->GetNeedUpdate())
    {
      vertexArray->Populate();
       TF_DEBUG(LOFI_REGISTRY).Msg("Populate Vertex ARRAY !!!\n");
    }
   
  }
}

void
LoFiResourceRegistry::_GarbageCollect()
{
  _vertexArrayRegistry.GarbageCollect();
  _vertexBufferRegistry.GarbageCollect();
  _glslShaderRegistry.GarbageCollect();
  _glslProgramRegistry.GarbageCollect();
  _textureResourceRegistry.GarbageCollect();
  _textureResourceHandleRegistry.GarbageCollect();
}

void
LoFiResourceRegistry::_GarbageCollectBprims()
{
}


PXR_NAMESPACE_CLOSE_SCOPE

