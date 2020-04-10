//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include <iostream>
#include "pxr/imaging/plugin/Lofi/debugCodes.h"
#include "pxr/imaging/plugin/Lofi/resourceRegistry.h"
#include "pxr/imaging/plugin/Lofi/timer.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE


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

HdInstance<LoFiGLSLShaderSharedPtr>
LoFiResourceRegistry::RegisterGLSLShader(
  HdInstance<LoFiGLSLShaderSharedPtr>::ID id)
{
  return _Register(id, _glslShaderRegistry, LoFiRegistryTokens->glslShader);
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
  return _Register(id, _glslProgramRegistry, LoFiRegistryTokens->glslProgram);
}

LoFiGLSLProgramSharedPtr
LoFiResourceRegistry::GetGLSLProgram(HdInstance<LoFiGLSLProgramSharedPtr>::ID id)
{
  bool found;
  auto instance = _glslProgramRegistry.FindInstance(id, &found);
  if(found) return instance.GetValue();
  else return LoFiGLSLProgramSharedPtr(nullptr);
}


void
LoFiResourceRegistry::_Commit()
{
  TF_STATUS("RESSOURCE REGISTRY COMMIT BEGIN...");
  for(auto& instance: _vertexArrayRegistry)
  {
    LoFiVertexArraySharedPtr vertexArray = instance.second.value;
    if(vertexArray->GetNeedReallocate())
    {
      TF_STATUS("REALLOCATE VERTEX ARRAY...");
      vertexArray->Reallocate();
      TF_STATUS("POPULATE VERTEX ARRAY...");
      vertexArray->Populate();
      TF_STATUS("ALL DONE...");
    }
    else if(vertexArray->GetNeedUpdate())
    {
      vertexArray->Populate();
    }
  }
  TF_STATUS("RESSOURCE REGISTRY COMMIT DONE...");
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

