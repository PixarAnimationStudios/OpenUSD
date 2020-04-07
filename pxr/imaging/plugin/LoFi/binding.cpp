//
// Copyright 2020 benmalartre
//
// unlicensed
//

#include "pxr/imaging/plugin/LoFi/binding.h"

PXR_NAMESPACE_OPEN_SCOPE


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


PXR_NAMESPACE_CLOSE_SCOPE