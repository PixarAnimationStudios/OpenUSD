//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HDST_RESOURCE_REGISTRY_H
#define HDST_RESOURCE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hd/resourceRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStDispatchBuffer>
    HdStDispatchBufferSharedPtr;
typedef boost::shared_ptr<class HdStPersistentBuffer>
    HdStPersistentBufferSharedPtr;
typedef boost::shared_ptr<class HdStResourceRegistry>
    HdStResourceRegistrySharedPtr;
typedef boost::shared_ptr<class HdSt_GeometricShader>
    HdSt_GeometricShaderSharedPtr;
typedef boost::shared_ptr<class HdStGLSLProgram> HdStGLSLProgramSharedPtr;

/// \class HdStResourceRegistry
///
/// A central registry of all GPU resources.
///
class HdStResourceRegistry : public HdResourceRegistry  {
public:
    HF_MALLOC_TAG_NEW("new HdStResourceRegistry");

    HDST_API
    HdStResourceRegistry();

    HDST_API
    virtual ~HdStResourceRegistry();

    /// Register a buffer allocated with \a count * \a commandNumUints *
    /// sizeof(GLuint) to be used as an indirect dispatch buffer.
    HDST_API
    HdStDispatchBufferSharedPtr RegisterDispatchBuffer(
        TfToken const &role, int count, int commandNumUints);

    /// Register a buffer initialized with \a dataSize bytes of \a data
    /// to be used as a persistently mapped shader storage buffer.
    HDST_API
    HdStPersistentBufferSharedPtr RegisterPersistentBuffer(
        TfToken const &role, size_t dataSize, void *data);

    /// Remove any entries associated with expired dispatch buffers.
    HDST_API
    void GarbageCollectDispatchBuffers();

    /// Remove any entries associated with expired persistently mapped buffers.
    HDST_API
    void GarbageCollectPersistentBuffers();

    /// Check if \p range is compatible with \p newBufferSpecs.
    /// If not, allocate new bufferArrayRange with merged buffer specs,
    /// register migration computation and return the new range.
    /// Otherwise just return the same range.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeBufferArrayRange(
        HdAggregationStrategy *strategy,
        HdBufferArrayRegistry &bufferArrayRegistry,
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of non uniform buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of non uniform immutable buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeNonUniformImmutableBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of uniform buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of shader storage buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// Register a geometric shader.
    HDST_API
    std::unique_lock<std::mutex> RegisterGeometricShader(HdStShaderKey::ID id,
         HdInstance<HdStShaderKey::ID, HdSt_GeometricShaderSharedPtr> *pInstance);

    /// Register a GLSL program into the program registry.
    /// note: Currently no garbage collection enforced on the shader registry
    HDST_API
    std::unique_lock<std::mutex> RegisterGLSLProgram(HdStGLSLProgram::ID id,
        HdInstance<HdStGLSLProgram::ID, HdStGLSLProgramSharedPtr> *pInstance);

    void InvalidateShaderRegistry() override;

protected:
    virtual void _GarbageCollect() override;
    virtual void _TallyResourceAllocation(VtDictionary *result) const override;

private:

    typedef std::vector<HdStDispatchBufferSharedPtr>
        _DispatchBufferRegistry;
    _DispatchBufferRegistry _dispatchBufferRegistry;

    typedef std::vector<HdStPersistentBufferSharedPtr>
        _PersistentBufferRegistry;
    _PersistentBufferRegistry _persistentBufferRegistry;

    // geometric shader registry
    typedef HdInstance<HdStShaderKey::ID, HdSt_GeometricShaderSharedPtr>
         _GeometricShaderInstance;
    HdInstanceRegistry<_GeometricShaderInstance> _geometricShaderRegistry;

    // glsl shader program registry
    typedef HdInstance<HdStGLSLProgram::ID, HdStGLSLProgramSharedPtr>
        _GLSLProgramInstance;
    HdInstanceRegistry<_GLSLProgramInstance> _glslProgramRegistry;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_RESOURCE_REGISTRY_H
