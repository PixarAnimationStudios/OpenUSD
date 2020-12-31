#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hio/glslfxConfig.h"
#include "pxr/imaging/plugin/LoFi/debugCodes.h"
#include "pxr/imaging/plugin/LoFi/tokens.h"
#include "pxr/imaging/plugin/LoFi/utils.h"


PXR_NAMESPACE_OPEN_SCOPE

using HdBindingRequestVector = std::vector<class HdBindingRequest>;

using LoFiShaderCodeSharedPtr =
    std::shared_ptr<class LoFiShaderCode>;
using LoFiShaderCodeSharedPtrVector =
    std::vector<LoFiShaderCodeSharedPtr>;

using LoFiMaterialParamVector =
    std::vector<class LoFiMaterialParam>;
using HdBufferSourceSharedPtr =
    std::shared_ptr<class HdBufferSource>;
using HdBufferSourceSharedPtrVector =
    std::vector<HdBufferSourceSharedPtr>;
using HdBufferArrayRangeSharedPtr =
    std::shared_ptr<class HdBufferArrayRange>;
using LoFiTextureHandleSharedPtr =
    std::shared_ptr<class LoFiTextureHandle>;
using HdComputationSharedPtr =
    std::shared_ptr<class HdComputation>;

class HdRenderPassState;
class LoFiBinder;
class LoFiResourceRegistry;

class HioGlslfx;

class LoFiShaderCode {
public:
  LoFiShaderCode(const TfToken& filename);

  std::string
  GetSource(const TfToken& key);

  ///
  /// \name Texture system
  /// @{

  /// Information necessary to bind textures and create accessor
  /// for the texture.
  ///
  struct NamedTextureHandle {
      /// Name by which the texture will be accessed, i.e., the name
      /// of the accesor for thexture will be HdGet_name(...).
      ///
      TfToken name;
      /// Equal to handle->GetTextureObject()->GetTextureType().
      /// Saved here for convenience (note that name and type
      /// completely determine the creation of the texture accesor
      /// HdGet_name(...)).
      ///
      HdTextureType type;
      /// The texture.
      LoFiTextureHandleSharedPtr handle;

      /// A hash unique to the corresponding asset; used to
      /// split draw batches when not using bindless textures.
      size_t hash;
  };
  using NamedTextureHandleVector = std::vector<NamedTextureHandle>;

  /// Textures that need to be bound for this shader.
  ///
  LOFI_API
  virtual NamedTextureHandleVector const & GetNamedTextureHandles() const;

  // non copyable
  LoFiShaderCode(const LoFiShaderCode& ) = delete;
  LoFiShaderCode(LoFiShaderCode&&) = delete;

  ~LoFiShaderCode(){};

private:
  TfToken                           _filename;
  std::string                       _name;

  std::unique_ptr<HioGlslfx>        _glslfx;
  size_t                            _hash;

};
  

PXR_NAMESPACE_CLOSE_SCOPE
