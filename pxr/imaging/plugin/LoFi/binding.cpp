//
// Copyright 2020 benmalartre
//
// unlicensed
//

#include "pxr/imaging/plugin/LoFi/tokens.h"
#include "pxr/imaging/plugin/LoFi/binding.h"
#include "pxr/imaging/plugin/LoFi/shader.h"
#include "pxr/base/tf/hash.h"
#include <boost/functional/hash.hpp>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(LoFiBindingSuffixTokens,
                        LOFI_BINDING_SUFFIX_TOKENS);

void LoFiBinder::Clear()
{
  _uniformBindings.clear();
  _textureBindings.clear();
  _attributeBindings.clear();
}

void LoFiBinder::CreateUniformBinding(const TfToken& name, const TfToken& dataType,
 size_t location)
{
  _uniformBindings.push_back({
    LoFiBindingType::UNIFORM,
    0,
    name,
    dataType,
    0
  });
}

void LoFiBinder::CreateTextureBinding(const TfToken& name, const TfToken& dataType,
 size_t location)
{
  _textureBindings.push_back({
    LoFiBindingType::TEXTURE,
    location,
    name,
    dataType,
    0
  });
}

void LoFiBinder::CreateAttributeBinding(const TfToken& name, const TfToken& dataType,
 size_t location)
{
  _attributeBindings.push_back({
    LoFiBindingType::VERTEX,
    location,
    name,
    dataType,
    0
  });
}

bool LoFiBinder::HaveAttribute(const TfToken& name)
{
  for(const auto& attr: _attributeBindings)
  {
    if(attr.name == name)return true;
  }
  return false;
}

void LoFiBinder::ComputeProgramName()
{
  std::string name;
  switch(_programType)
  {
    case LoFiProgramType::LOFI_PROGRAM_MESH:
      name += "MESH_";
      break;
    case LoFiProgramType::LOFI_PROGRAM_POINT:
      name += "POINT_";
      break;
    case LoFiProgramType::LOFI_PROGRAM_CURVE:
      name += "CURVE_";
      break;
    case LoFiProgramType::LOFI_PROGRAM_CONTOUR:
      name += "CONTOUR_";
      break;
  }
  size_t hash = 0;
  for(const auto& attr: _attributeBindings)
  {
    boost::hash_combine(hash, attr.name.Hash());
  }
  for(const auto& uniform: _uniformBindings)
  {
    boost::hash_combine(hash, uniform.name.Hash());
  }
  name += std::to_string(hash);
  _programName = TfToken(name);
}

void LoFiBinder::Bind()
{
  glUseProgram(_program->Get());
  for(const auto& uniform: _uniformBindings)
  {
    if(uniform.type == LoFiBindingType::UNIFORM)
    {
      if(uniform.dataType == LoFiGLTokens->vec2)
      {
        glUniform2fv(uniform.location, 1, (const GLfloat*)uniform.rawDatasPtr);
      }
      else if(uniform.dataType == LoFiGLTokens->vec3)
      {
        glUniform3fv(uniform.location, 1, (const GLfloat*)uniform.rawDatasPtr);
      }
      else if(uniform.dataType == LoFiGLTokens->mat4)
      {
        glUniformMatrix4fv(uniform.location, 1, GL_FALSE, (const GLfloat*)uniform.rawDatasPtr);
      }
    }
  }
}

const LoFiBinding& LoFiBinder::GetUniformBinding(const TfToken& name) const
{
  for(const auto& binding: _uniformBindings)
  {
    if(binding.name == name)return binding;
  }
  return LoFiBinding();
}

const LoFiBinding& LoFiBinder::GetTextureBinding(const TfToken& name) const
{
  for(const auto& binding: _textureBindings)
  {
    if(binding.name == name)return binding;
  }
  return LoFiBinding();
}

const LoFiBinding& LoFiBinder::GetAttributeBinding(const TfToken& name) const
{
  for(const auto& binding: _attributeBindings)
  {
    if(binding.name == name)return binding;
  }
  return LoFiBinding();
}


PXR_NAMESPACE_CLOSE_SCOPE