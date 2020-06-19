//
// Copyright 2020 benmalartre
//
// Unlicensed 
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_RESOURCE_REGISTRY_H
#define PXR_IMAGING_PLUGIN_LOFI_RESOURCE_REGISTRY_H

#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

class LoFiVertexArray;
class LoFiVertexBuffer;
class LoFiGLSLShader;
class LoFiGLSLProgram;
class LoFiTextureResource;
class LoFiTextureResourceHandle;

typedef std::shared_ptr<class LoFiGLSLShader>
    LoFiGLSLShaderSharedPtr;
typedef std::shared_ptr<class LoFiGLSLProgram>
    LoFiGLSLProgramSharedPtr;
typedef std::shared_ptr<class LoFiVertexArray>
    LoFiVertexArraySharedPtr;
typedef std::shared_ptr<class LoFiVertexBuffer>
    LoFiVertexBufferSharedPtr;
typedef std::shared_ptr<class LoFiResourceRegistry>
    LoFiResourceRegistrySharedPtr;
typedef std::shared_ptr<class LoFiTextureResource>
    LoFiTextureResourceSharedPtr;
typedef std::shared_ptr<class LoFiTextureResourceHandle>
    LoFiTextureResourceHandleSharedPtr;  

/// \class LoFiResourceRegistry
///
/// A central registry for resources.
///
class LoFiResourceRegistry : public HdResourceRegistry {
public:
    HF_MALLOC_TAG_NEW("new LoFiResourceRegistry");

    LoFiResourceRegistry();

    virtual ~LoFiResourceRegistry();

    /// Invalidate any shaders registered with this registry.
    virtual void InvalidateShaderRegistry();

    /// Generic method to inform RenderDelegate a resource needs to be reloaded.
    /// This method can be used by the application to inform the renderDelegate
    /// that a resource, which may not have any prim representation in Hydra, 
    /// needs to be reloaded. For example a texture found in a material network.
    /// The `path` can be absolute or relative. It should usually match the
    /// path found for textures during HdMaterial::Sync.
    virtual void ReloadResource(
        TfToken const& resourceType,
        std::string const& path);

    /// Returns a report of resource allocation by role in bytes and
    /// a summary total allocation of GPU memory in bytes for this registry.
    virtual VtDictionary GetResourceAllocation() const;

    /// Register a vertex array object
    HdInstance<LoFiVertexArraySharedPtr>
    RegisterVertexArray(HdInstance<LoFiVertexArraySharedPtr>::ID id);

    /// Query a vertex array object
    LoFiVertexArraySharedPtr
    GetVertexArray(HdInstance<LoFiVertexArraySharedPtr>::ID id);

    /// Check a vertex buffer object
    bool
    HasVertexArray(HdInstance<LoFiVertexArraySharedPtr>::ID id);

    /// Register a vertex buffer object
    HdInstance<LoFiVertexBufferSharedPtr>
    RegisterVertexBuffer(HdInstance<LoFiVertexBufferSharedPtr>::ID id);

    /// Check a vertex buffer object
    bool
    HasVertexBuffer(HdInstance<LoFiVertexBufferSharedPtr>::ID id);

    /// Query a vertex buffer object
    LoFiVertexBufferSharedPtr
    GetVertexBuffer(HdInstance<LoFiVertexBufferSharedPtr>::ID id);

    /// Register a GLSL shader into the program registry.
    HdInstance<LoFiGLSLShaderSharedPtr>
    RegisterGLSLShader(HdInstance<LoFiGLSLShaderSharedPtr>::ID id);

    /// Query a GLSL shader into the program registry
    LoFiGLSLShaderSharedPtr
    GetGLSLShader(HdInstance<LoFiGLSLShaderSharedPtr>::ID id);

    /// Register a GLSL program into the program registry.
    HdInstance<LoFiGLSLProgramSharedPtr>
    RegisterGLSLProgram(HdInstance<LoFiGLSLProgramSharedPtr>::ID id);

    /// Query a GLSL program into the program registry.
    LoFiGLSLProgramSharedPtr
    GetGLSLProgram(HdInstance<LoFiGLSLProgramSharedPtr>::ID id);

    /// Register a texture into the texture registry.
    /// Typically the other id's used refer to unique content
    /// where as for textures it's a unique id provided by the scene delegate.
    /// Hydra expects the id's to be unique in the context of a scene/stage
    /// aka render index.  However, the texture registry can be shared between
    /// multiple render indices, so the renderIndexId is used to create
    /// a globally unique id for the texture resource.
    LOFI_API
    HdInstance<LoFiTextureResourceSharedPtr>
    RegisterTextureResource(TextureKey id);

    /// Find a texture in the texture registry. If found, it returns it.
    /// See RegisterTextureResource() for parameter details.
    LOFI_API
    HdInstance<LoFiTextureResourceSharedPtr>
    FindTextureResource(TextureKey id, bool *found);

    /// Register a texture resource handle.
    LOFI_API
    HdInstance<LoFiTextureResourceHandleSharedPtr>
    RegisterTextureResourceHandle(
        HdInstance<LoFiTextureResourceHandleSharedPtr>::ID id);

    /// Find a texture resource handle.
    LOFI_API
    HdInstance<LoFiTextureResourceHandleSharedPtr>
    FindTextureResourceHandle(
        HdInstance<LoFiTextureResourceHandleSharedPtr>::ID id, bool *found);


protected:
    virtual void _Commit() override;
    virtual void _GarbageCollect() override;
    virtual void _GarbageCollectBprims() override;

private:
    // Not copyable
    LoFiResourceRegistry(const LoFiResourceRegistry&) = delete;
    LoFiResourceRegistry& operator=(const LoFiResourceRegistry&) = delete;

    // vaos
    HdInstanceRegistry<LoFiVertexArraySharedPtr> _vertexArrayRegistry;

    // vbos
    HdInstanceRegistry<LoFiVertexBufferSharedPtr> _vertexBufferRegistry;

    // glsl shader registry
    HdInstanceRegistry<LoFiGLSLShaderSharedPtr> _glslShaderRegistry;

    // glsl program registry
    HdInstanceRegistry<LoFiGLSLProgramSharedPtr> _glslProgramRegistry;

    // texture resource registry
    HdInstanceRegistry<LoFiTextureResourceSharedPtr> _textureResourceRegistry;

     // texture resource handle registry
    HdInstanceRegistry<LoFiTextureResourceHandleSharedPtr> _textureResourceHandleRegistry;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_PLUGIN_LOFI_RESOURCE_REGISTRY_H
