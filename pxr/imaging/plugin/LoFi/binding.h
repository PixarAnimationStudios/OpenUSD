//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_BINDING_H
#define PXR_IMAGING_PLUGIN_LOFI_BINDING_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiDrawItem;

enum LoFiBindingType { 
  UNKNOWN,
  VERTEX,
  INDEX,
  UNIFORM,
  UNIFORM_ARRAY,
  TBO,
  TEXTURE
};

/// \struct LoFiBinding
///
struct LoFiBinding {
  LoFiBindingType     type;
  size_t              location;
  TfToken             name;
  TfToken             dataType;
  size_t              arraySize;
};
typedef std::vector<LoFiBinding> LoFiBindingList;


class LoFiBinder {
public:
  void CreateUniformBinding(const TfToken& name, const TfToken& dataType, size_t location);
  void CreateTextureBinding(const TfToken& name, const TfToken& dataType, size_t location);
  void CreateAttributeBinding(const TfToken& name, const TfToken& dataType, size_t location);

  const LoFiBindingList& GetUniformBindings(){return _uniformBindings;};
  const LoFiBindingList& GetTextureBindings(){return _textureBindings;};
  const LoFiBindingList& GetAttributeBindings(){return _attributeBindings;};

private:
  LoFiBindingList _uniformBindings;
  LoFiBindingList _textureBindings;
  LoFiBindingList _attributeBindings;

  int _uniformLocation;
  int _textureLocation;
  int _attributeLocation;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_BINDING_H