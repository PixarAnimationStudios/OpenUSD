//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_SHADER_CODE_H
#define PXR_IMAGING_HD_ST_SHADER_CODE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/enums.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using HdStBindingRequestVector = std::vector<class HdStBindingRequest>;

using HdStShaderCodeSharedPtr =
    std::shared_ptr<class HdStShaderCode>;
using HdStShaderCodeSharedPtrVector =
    std::vector<HdStShaderCodeSharedPtr>;

using HdSt_MaterialParamVector =
    std::vector<class HdSt_MaterialParam>;
using HdBufferSourceSharedPtr =
    std::shared_ptr<class HdBufferSource>;
using HdBufferSourceSharedPtrVector =
    std::vector<HdBufferSourceSharedPtr>;
using HdBufferArrayRangeSharedPtr =
    std::shared_ptr<class HdBufferArrayRange>;
using HdStTextureHandleSharedPtr =
    std::shared_ptr<class HdStTextureHandle>;
using HdStComputationSharedPtr =
    std::shared_ptr<class HdStComputation>;

class HioGlslfx;
class HdSt_ResourceBinder;
class HdStResourceRegistry;

/// \class HdStShaderCode
///
/// A base class representing the implementation (code) of a shader,
/// used in conjunction with HdRenderPass.
///
/// This interface provides a simple way for clients to affect the
/// composition of shading programs used for a render pass.
class HdStShaderCode : public std::enable_shared_from_this<HdStShaderCode>
{
public:
    typedef size_t ID;

    HDST_API
    HdStShaderCode();
    HDST_API
    virtual ~HdStShaderCode();

    /// Returns the hash value of the shader code and configuration.
    ///
    /// It is computed from the the GLSL code as well as the resource
    /// signature of the shader (as determined from its parameters).
    /// If two shaders have the same hash, the GLSL code as expanded
    /// by codegen should also be the same.
    /// 
    virtual ID ComputeHash() const = 0;

    /// Returns the combined hash values of multiple shaders.
    HDST_API
    static ID ComputeHash(HdStShaderCodeSharedPtrVector const &shaders);

    /// Returns the hash value of the paths of the texture prims
    /// consumed by this shader.
    ///
    /// Unless textures are bindless, shaders using different textures
    /// cannot be used in the same draw batch. Since textures can be
    /// animated, it can happen that two texture prims use the same
    /// texture at some time but different textures at other times. To
    /// avoid re-computing the draw batches over time, we use the this
    /// hash when grouping the draw batches.
    ///
    HDST_API
    virtual ID ComputeTextureSourceHash() const;

    /// Returns the shader source provided by this shader
    /// for \a shaderStageKey
    virtual std::string GetSource(TfToken const &shaderStageKey) const = 0;

    /// Returns the resource layout for the shader stages specified by
    /// \a shaderStageKeys. This is initialized using the shader's
    /// HioGlslfx configuration.
    HDST_API
    VtDictionary GetLayout(TfTokenVector const &shaderStageKeys) const;

    // XXX: Should be pure-virtual
    /// Returns the shader parameters for this shader.
    HDST_API
    virtual HdSt_MaterialParamVector const& GetParams() const;

    /// Returns whether primvar filtering is enabled for this shader.
    HDST_API
    virtual bool IsEnabledPrimvarFiltering() const;

    /// Returns the names of primvar that are used by this shader.
    HDST_API
    virtual TfTokenVector const& GetPrimvarNames() const;

    /// @}

    ///
    /// \name Texture system
    /// @{

    /// Information necessary to bind textures and create accessor
    /// for the texture. Can be used for a single texture, or an array of 
    /// textures.
    ///
    struct NamedTextureHandle {
        /// Name by which the texture(s) will be accessed, i.e., the name
        /// of the accesor for the texture(s) will be HdGet_name(...).
        ///
        TfToken name;
        /// Equal to handle->GetTextureObject()->GetTextureType().
        /// Saved here for convenience (note that name and type
        /// completely determine the creation of the texture accesor
        /// HdGet_name(...)).
        ///
        HdStTextureType type;
        /// The texture(s). Multiple handles indicate an array of textures.
        std::vector<HdStTextureHandleSharedPtr> handles;
        
        /// A hash unique to the corresponding asset; used to
        /// split draw batches when not using bindless textures.
        size_t hash;
    };
    using NamedTextureHandleVector = std::vector<NamedTextureHandle>;

    /// Textures that need to be bound for this shader.
    ///
    HDST_API
    virtual NamedTextureHandleVector const & GetNamedTextureHandles() const;

    /// @}

    // XXX: Should be pure-virtual
    /// Returns a buffer which stores parameter fallback values and texture
    /// handles.
    HDST_API
    virtual HdBufferArrayRangeSharedPtr const& GetShaderData() const;

    /// Binds shader-specific resources to \a program
    /// XXX: this interface is meant to be used for bridging
    /// the GlfSimpleLightingContext mechanism, and not for generic use-cases.
    virtual void BindResources(int program,
                               HdSt_ResourceBinder const &binder) = 0;

    /// Unbinds shader-specific resources.
    virtual void UnbindResources(int program,
                                 HdSt_ResourceBinder const &binder) = 0;

    /// Add custom bindings (used by codegen)
    virtual void AddBindings(HdStBindingRequestVector* customBindings) = 0;

    /// Material tags can be set in the meta-data of a glslfx file to control
    /// what rprim collection that prims using this shader should go into.
    /// E.g. We can use it to split opaque and translucent prims into different
    /// collections. When no material tags are specified in the shader, a empty
    /// token is returned.
    HDST_API
    virtual TfToken GetMaterialTag() const;

    /// \class ResourceContext
    ///
    /// The context available in implementations of
    /// AddResourcesFromTextures.
    class ResourceContext {
    public:
        HDST_API
        void AddSource(HdBufferArrayRangeSharedPtr const &range,
                       HdBufferSourceSharedPtr const &source);

        HDST_API
        void AddSources(HdBufferArrayRangeSharedPtr const &range,
                        HdBufferSourceSharedPtrVector &&sources);

        HDST_API
        void AddComputation(HdBufferArrayRangeSharedPtr const &range,
                            HdStComputationSharedPtr const &computation,
                            HdStComputeQueue const queue);
        
        HdStResourceRegistry * GetResourceRegistry() const {
            return _registry;
        }

    private:
        friend class HdStResourceRegistry;
        ResourceContext(HdStResourceRegistry *);
        HdStResourceRegistry *_registry;
    };

    /// This function is called after textures have been allocated and
    /// loaded to add buffer sources and computations to the resource
    /// registry that require texture meta data not available until
    /// the texture is allocated or loaded. For example, the OpenGl
    /// texture sampler handle (in the bindless case) is not available
    /// until after the texture commit phase.
    ///
    HDST_API
    virtual void AddResourcesFromTextures(ResourceContext &ctx) const;

private:

    // No copying
    HdStShaderCode(const HdStShaderCode &)                      = delete;
    HdStShaderCode &operator =(const HdStShaderCode &)          = delete;

    // Returns the HioGlslfx instance used to configure this shader.
    // This can return nullptr for shaders without a GLSLFX instance.
    virtual HioGlslfx const * _GetGlslfx() const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_SHADER_H
