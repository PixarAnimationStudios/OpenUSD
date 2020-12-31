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
#include "pxr/imaging/plugin/LoFi/shader.h"
#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiDrawItem;

enum LoFiProgramType {
  LOFI_PROGRAM_MESH,
  LOFI_PROGRAM_CURVE,
  LOFI_PROGRAM_POINT,
  LOFI_PROGRAM_CONTOUR
};

enum LoFiBindingType { 
  UNKNOWN,
  VERTEX,
  INDEX,
  UNIFORM,
  UNIFORM_ARRAY,
  TBO,
  TEXTURE
};

/// Suffixes appended to material param names for a binding name.
///
#define LOFI_BINDING_SUFFIX_TOKENS              \
    ((fallback, "_fallback"))                   \
    ((samplingTransform, "_samplingTransform")) \
    ((layout, "_layout"))                       \
    ((texture, "_texture"))                     \
    ((valid, "_valid"))

TF_DECLARE_PUBLIC_TOKENS(LoFiBindingSuffixTokens,
                         LOFI_BINDING_SUFFIX_TOKENS);

/// \struct LoFiBinding
///
struct LoFiBinding {
  LoFiBindingType     type;
  size_t              location;
  TfToken             name;
  TfToken             dataType;
  size_t              arraySize;
  void*               rawDatasPtr;
};
typedef std::vector<LoFiBinding> LoFiBindingList;

class LoFiBinder {
public:
  void Clear();
  void CreateUniformBinding(const TfToken& name, const TfToken& dataType, size_t location);
  void CreateTextureBinding(const TfToken& name, const TfToken& dataType, size_t location);
  void CreateAttributeBinding(const TfToken& name, const TfToken& dataType, size_t location);

  const LoFiBindingList& GetUniformBindings() const {return _uniformBindings;};
  const LoFiBindingList& GetTextureBindings() const {return _textureBindings;};
  const LoFiBindingList& GetAttributeBindings() const {return _attributeBindings;};

  void SetProgramType(LoFiProgramType type){_programType = type;};
  LoFiProgramType GetProgramType() const {return _programType;};
  TfToken GetProgramName() const {return _programName;};
  void ComputeProgramName();
  bool HaveAttribute(const TfToken& name);

  size_t GetNumVertexPerPrimitive() const {return _numVertexPerPrimitive;};
  void SetNumVertexPerPrimitive(size_t numVertexPerPrimitive){
    _numVertexPerPrimitive = numVertexPerPrimitive;
  };

  void Bind();
  const LoFiBinding& GetUniformBinding(const TfToken& name) const;
  const LoFiBinding& GetTextureBinding(const TfToken& name) const;
  const LoFiBinding& GetAttributeBinding(const TfToken& name) const;

private:
  LoFiBindingList           _uniformBindings;
  LoFiBindingList           _textureBindings;
  LoFiBindingList           _attributeBindings;

  LoFiProgramType           _programType;
  TfToken                   _programName;
  LoFiGLSLProgramSharedPtr  _program;
  size_t                    _numVertexPerPrimitive;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_BINDING_H