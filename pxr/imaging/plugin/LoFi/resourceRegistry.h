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
#include "pxr/imaging/plugin/LoFi/shader.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"
#include "pxr/imaging/plugin/LoFi/shader.h"

PXR_NAMESPACE_OPEN_SCOPE

class LoFiMesh;

typedef boost::shared_ptr<class LoFiGLSLShader>
    LoFiGLSLShaderSharedPtr;
typedef boost::shared_ptr<class LoFiGLSLProgram>
    LoFiGLSLProgramSharedPtr;
typedef boost::shared_ptr<class LoFiVertexArray>
    LoFiVertexArraySharedPtr;
typedef boost::shared_ptr<class LoFiResourceRegistry>
    LoFiResourceRegistrySharedPtr;

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

    /// Register a mesh into the program registry.
    HdInstance<LoFiMesh*>
    RegisterMesh(HdInstance<LoFiMesh*>::ID id);

    /// Query a mesh into the program registry.
    LoFiMesh*
    GetMesh(HdInstance<LoFiMesh*>::ID id);



protected:
    virtual void _Commit();
    virtual void _GarbageCollect();
    virtual void _GarbageCollectBprims();

private:
    // Not copyable
    LoFiResourceRegistry(const LoFiResourceRegistry&) = delete;
    LoFiResourceRegistry& operator=(const LoFiResourceRegistry&) = delete;

    // vaos
    HdInstanceRegistry<LoFiVertexArraySharedPtr> _vertexArrayRegistry;

    // vbos
    //HdInstanceRegistry<LoFiVertexBuffer*> _vertexBufferRegistry;

    // ebos
    //HdInstanceRegistry<LoFiElementBuffer*> _elementBufferRegistry;

    // glsl shader registry
    HdInstanceRegistry<LoFiGLSLShaderSharedPtr> _glslShaderRegistry;

    // glsl program registry
    HdInstanceRegistry<LoFiGLSLProgramSharedPtr> _glslProgramRegistry;

    // tmp mesh registry
    HdInstanceRegistry<LoFiMesh*>                _meshesRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_PLUGIN_LOFI_RESOURCE_REGISTRY_H
