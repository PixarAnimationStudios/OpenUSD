//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include <iostream>
#include "pxr/imaging/plugin/Lofi/debugCodes.h"
#include "pxr/imaging/plugin/Lofi/resourceRegistry.h"
#include "pxr/imaging/plugin/Lofi/mesh.h"
#include "pxr/imaging/plugin/Lofi/timer.h"

PXR_NAMESPACE_OPEN_SCOPE

class LoFiMesh;

LoFiResourceRegistry::LoFiResourceRegistry()
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
  return _Register(id, _vertexArrayRegistry, LoFiTokens->vertexArray);
}

LoFiVertexArraySharedPtr
LoFiResourceRegistry::GetVertexArray(HdInstance<LoFiVertexArraySharedPtr>::ID id)
{
  bool found;
  auto instance = _vertexArrayRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return LoFiVertexArraySharedPtr(nullptr);
}

HdInstance<LoFiGLSLShaderSharedPtr>
LoFiResourceRegistry::RegisterGLSLShader(
  HdInstance<LoFiGLSLShaderSharedPtr>::ID id)
{
  return _Register(id, _glslShaderRegistry, LoFiTokens->glslShader);
}

LoFiGLSLShaderSharedPtr
LoFiResourceRegistry::GetGLSLShader(HdInstance<LoFiVertexArraySharedPtr>::ID id)
{
  bool found;
  auto instance = _glslShaderRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return LoFiGLSLShaderSharedPtr(nullptr);
}

HdInstance<LoFiGLSLProgramSharedPtr>
LoFiResourceRegistry::RegisterGLSLProgram(
  HdInstance<LoFiGLSLProgramSharedPtr>::ID id)
{
  return _Register(id, _glslProgramRegistry, LoFiTokens->glslProgram);
}

LoFiGLSLProgramSharedPtr
LoFiResourceRegistry::GetGLSLProgram(HdInstance<LoFiGLSLProgramSharedPtr>::ID id)
{
  bool found;
  auto instance = _glslProgramRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return LoFiGLSLProgramSharedPtr(nullptr);
}

HdInstance<LoFiMesh*>
LoFiResourceRegistry::RegisterMesh(
  HdInstance<LoFiMesh*>::ID id)
{
  return _Register(id, _meshesRegistry, LoFiTokens->mesh);
}

LoFiMesh*
LoFiResourceRegistry::GetMesh(HdInstance<LoFiMesh*>::ID id)
{
  bool found;
  auto instance = _meshesRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return NULL;
}


void
LoFiResourceRegistry::_Commit()
{
  /// infos about the mesh in the scene
  {
    if(TfDebug::IsEnabled(LOFI_GEOMETRY))
    {
      for(const auto instance: _meshesRegistry)
      {
        LoFiMesh* mesh = instance.second.value;
        if(mesh)mesh->InfosLog();
      }
    }
  }
  
  {
    for(auto& instance: _vertexArrayRegistry)
    {
      LoFiVertexArraySharedPtr vertexArray = instance.second.value;
      if(vertexArray->GetNeedReallocate())
      {
        vertexArray->Reallocate();
        vertexArray->Populate();
      }
      else if(vertexArray->GetNeedUpdate())
      {
        vertexArray->Populate();
      }
    }
  }
}

void
LoFiResourceRegistry::_GarbageCollect()
{
}

void
LoFiResourceRegistry::_GarbageCollectBprims()
{
}


PXR_NAMESPACE_CLOSE_SCOPE

