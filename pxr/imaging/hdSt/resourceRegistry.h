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
#ifndef PXR_IMAGING_HD_ST_RESOURCE_REGISTRY_H
#define PXR_IMAGING_HD_ST_RESOURCE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hgi/hgi.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferArrayRegistry.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/resourceRegistry.h"

#include <tbb/concurrent_vector.h>

#include <atomic>
#include <map>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdComputationSharedPtr = std::shared_ptr<class HdComputation>;
using HdStDispatchBufferSharedPtr = std::shared_ptr<class HdStDispatchBuffer>;
using HdStGLSLProgramSharedPtr = std::shared_ptr<class HdStGLSLProgram>;

using HdSt_BasisCurvesTopologySharedPtr =
    std::shared_ptr<class HdSt_BasisCurvesTopology>;

using HdStShaderCodePtr =
    std::weak_ptr<class HdStShaderCode>;
using HdSt_GeometricShaderSharedPtr =
    std::shared_ptr<class HdSt_GeometricShader>;

using HdStTextureResourceSharedPtr =
    std::shared_ptr<class HdStTextureResource>;
using HdStTextureResourceHandleSharedPtr =
    std::shared_ptr<class HdStTextureResourceHandle>;

using HdStTextureHandleSharedPtr =
    std::shared_ptr<class HdStTextureHandle>;
using HdStTextureObjectSharedPtr =
    std::shared_ptr<class HdStTextureObject>;
using HdStPersistentBufferSharedPtr =
    std::shared_ptr<class HdStPersistentBuffer>; 
using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;
using Hd_VertexAdjacencySharedPtr = 
    std::shared_ptr<class Hd_VertexAdjacency>;
using HdSt_MeshTopologySharedPtr = 
    std::shared_ptr<class HdSt_MeshTopology>;
class HdStTextureIdentifier;
class HdSamplerParameters;

/// \class HdStResourceRegistry
///
/// A central registry of all GPU resources.
///
class HdStResourceRegistry final : public HdResourceRegistry 
{
public:
    HF_MALLOC_TAG_NEW("new HdStResourceRegistry");

    HDST_API
    explicit HdStResourceRegistry(Hgi* hgi);

    HDST_API
    ~HdStResourceRegistry() override;

    HDST_API
    void InvalidateShaderRegistry() override;

    HDST_API
    VtDictionary GetResourceAllocation() const override;

    /// Returns Hgi used to create/destroy GPU resources.
    HDST_API
    Hgi* GetHgi();

    /// ------------------------------------------------------------------------
    /// Texture allocation API
    /// ------------------------------------------------------------------------
    ///

    /// Allocate texture handle (encapsulates texture and sampler
    /// object, bindless texture sampler handle, memory request and
    /// callback to shader).
    ///
    /// The actual allocation of the associated GPU texture and
    /// sampler resources and loading of the texture file is delayed
    /// until the commit phase.
    HDST_API
    HdStTextureHandleSharedPtr AllocateTextureHandle(
        /// Path to file and information to identify a texture if the
        /// file is a container for several textures (e.g., OpenVDB
        /// file containing several grids, movie file containing frames).
        const HdStTextureIdentifier &textureId,
        /// Texture type, e.g., uv, ptex, ...
        HdTextureType textureType,
        /// Sampling parameters such as wrapS, ...
        /// wrapS, wrapT, wrapR mode, min filer, mag filter
        const HdSamplerParameters &samplerParams,
        /// Memory request. The texture is down-sampled to meet the
        /// target memory which is the maximum of all memory requests
        /// associated to the texture.
        /// If all memory requests are 0, no down-sampling will happen.
        size_t memoryRequest,
        /// Also create a GL texture sampler handle for bindless
        /// textures.
        bool createBindlessHandle,
        /// After the texture is committed (or after it has been
        /// changed) the given shader code can add additional buffer
        /// sources and computations using the texture metadata with
        /// AddResourcesFromTextures.
        HdStShaderCodePtr const &shaderCode);

    /// Allocate texture object.
    ///
    /// The actual allocation of the associated GPU texture and
    /// sampler resources and loading of the texture file is delayed
    /// until the commit phase.
    HDST_API
    HdStTextureObjectSharedPtr AllocateTextureObject(
        /// Path to file and information to identify a texture if the
        /// file is a container for several textures (e.g., OpenVDB
        /// file containing several grids, movie file containing frames).
        const HdStTextureIdentifier &textureId,
        /// Texture type, e.g., uv, ptex, ...
        HdTextureType textureType);

    /// ------------------------------------------------------------------------
    /// BAR allocation API
    /// ------------------------------------------------------------------------
    /// 
    /// The Allocate* flavor of methods allocate a new BAR for the given buffer 
    /// specs using the chosen aggregation strategy.

    HDST_API
    HdBufferArrayRangeSharedPtr AllocateNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    HDST_API
    HdBufferArrayRangeSharedPtr AllocateNonUniformImmutableBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    HDST_API
    HdBufferArrayRangeSharedPtr AllocateUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    HDST_API
    HdBufferArrayRangeSharedPtr AllocateShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    HDST_API
    HdBufferArrayRangeSharedPtr AllocateSingleBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// ------------------------------------------------------------------------
    /// BAR allocation/migration/update API
    /// ------------------------------------------------------------------------
    /// 
    /// The Update* flavor of methods handle both allocation of a new BAR and
    /// reallocation-migration based on the existing range, updated/added specs,
    /// removed specs and usage hint. This allows client code to be less verbose
    /// when a range's signature (specs) can change.
    ///
    /// If \p curRange is invalid, this is equivalent to calling Allocate*.
    /// Otherwise, checks if \p curRange needs to be migrated to a new range
    /// (based on \p updatedOrAddedSpecs and \p removedSpecs and \p usageHint).
    /// If migration is necessary, allocate a new range and register necessary
    /// migration computations and return the new range.
    /// Otherwise, just return the same range.
    
    HDST_API
    HdBufferArrayRangeSharedPtr UpdateNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint);

    HDST_API
    HdBufferArrayRangeSharedPtr UpdateNonUniformImmutableBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint);

    HDST_API
    HdBufferArrayRangeSharedPtr UpdateUniformBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint);

    HDST_API
    HdBufferArrayRangeSharedPtr UpdateShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint);

    /// ------------------------------------------------------------------------
    /// Resource update & computation queuing API
    /// ------------------------------------------------------------------------

    /// Append source data for given range to be committed later.
    HDST_API
    void AddSources(HdBufferArrayRangeSharedPtr const &range,
                    HdBufferSourceSharedPtrVector &&sources);

    /// Append a source data for given range to be committed later.
    HDST_API
    void AddSource(HdBufferArrayRangeSharedPtr const &range,
                   HdBufferSourceSharedPtr const &source);

    /// Append a source data just to be resolved (used for cpu computations).
    HDST_API
    void AddSource(HdBufferSourceSharedPtr const &source);

    /// Append a gpu computation into queue.
    /// The parameter 'range' specifies the destination buffer range,
    /// which has to be allocated by caller of this function.
    ///
    /// note: GPU computations will be executed in the order that
    /// they are registered.
    HDST_API
    void AddComputation(HdBufferArrayRangeSharedPtr const &range,
                        HdComputationSharedPtr const &computaion);

    /// ------------------------------------------------------------------------
    /// Dispatch & persistent buffer API
    /// ------------------------------------------------------------------------

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

    /// ------------------------------------------------------------------------
    /// Instance Registries
    /// ------------------------------------------------------------------------
    
    /// These registries implement sharing and deduplication of data based
    /// on computed hash identifiers. Each returned HdInstance object retains
    /// a shared pointer to a data instance. When an HdInstance is registered
    /// for a previously unused ID, the data pointer will be null and it is
    /// the caller's responsibility to set its value. The instance registries
    /// are cleaned of unreferenced entries during garbage collection.
    ///
    /// Note: As entries can be registered from multiple threads, the returned
    /// object holds a lock on the instance registry. This lock is held
    /// until the returned HdInstance object is destroyed.

    /// Topology instancing
    HDST_API
    HdInstance<HdSt_MeshTopologySharedPtr>
    RegisterMeshTopology(HdInstance<HdSt_MeshTopologySharedPtr>::ID id);

    HDST_API
    HdInstance<HdSt_BasisCurvesTopologySharedPtr>
    RegisterBasisCurvesTopology(
        HdInstance<HdSt_BasisCurvesTopologySharedPtr>::ID id);

    HDST_API
    HdInstance<Hd_VertexAdjacencySharedPtr>
    RegisterVertexAdjacency(HdInstance<Hd_VertexAdjacencySharedPtr>::ID id);

    /// Topology Index buffer array range instancing
    /// Returns the HdInstance points to shared HdBufferArrayRange,
    /// distinguished by given ID.
    /// *Refer the comment on RegisterTopology for the same consideration.
    HDST_API
    HdInstance<HdBufferArrayRangeSharedPtr>
    RegisterMeshIndexRange(
        HdInstance<HdBufferArrayRangeSharedPtr>::ID id, TfToken const &name);

    HDST_API
    HdInstance<HdBufferArrayRangeSharedPtr>
    RegisterBasisCurvesIndexRange(
       HdInstance<HdBufferArrayRangeSharedPtr>::ID id, TfToken const &name);

    /// Primvar array range instancing
    /// Returns the HdInstance pointing to shared HdBufferArrayRange,
    /// distinguished by given ID.
    /// *Refer the comment on RegisterTopology for the same consideration.
    HDST_API
    HdInstance<HdBufferArrayRangeSharedPtr>
    RegisterPrimvarRange(
        HdInstance<HdBufferArrayRangeSharedPtr>::ID id);

    /// ExtComputation data array range instancing
    /// Returns the HdInstance pointing to shared HdBufferArrayRange,
    /// distinguished by given ID.
    /// *Refer the comment on RegisterTopology for the same consideration.
    HDST_API
    HdInstance<HdBufferArrayRangeSharedPtr>
    RegisterExtComputationDataRange(
        HdInstance<HdBufferArrayRangeSharedPtr>::ID id);

    /// Register a texture into the texture registry.
    /// Typically the other id's used refer to unique content
    /// where as for textures it's a unique id provided by the scene delegate.
    /// Hydra expects the id's to be unique in the context of a scene/stage
    /// aka render index.  However, the texture registry can be shared between
    /// multiple render indices, so the renderIndexId is used to create
    /// a globally unique id for the texture resource.
    HDST_API
    HdInstance<HdStTextureResourceSharedPtr>
    RegisterTextureResource(TextureKey id);

    /// Find a texture in the texture registry. If found, it returns it.
    /// See RegisterTextureResource() for parameter details.
    HDST_API
    HdInstance<HdStTextureResourceSharedPtr>
    FindTextureResource(TextureKey id, bool *found);

    /// Register a geometric shader.
    HDST_API
    HdInstance<HdSt_GeometricShaderSharedPtr>
    RegisterGeometricShader(HdInstance<HdSt_GeometricShaderSharedPtr>::ID id);

    /// Register a GLSL program into the program registry.
    HDST_API
    HdInstance<HdStGLSLProgramSharedPtr>
    RegisterGLSLProgram(HdInstance<HdStGLSLProgramSharedPtr>::ID id);

    /// Register a texture resource handle.
    HDST_API
    HdInstance<HdStTextureResourceHandleSharedPtr>
    RegisterTextureResourceHandle(
        HdInstance<HdStTextureResourceHandleSharedPtr>::ID id);

    /// Find a texture resource handle.
    HDST_API
    HdInstance<HdStTextureResourceHandleSharedPtr>
    FindTextureResourceHandle(
        HdInstance<HdStTextureResourceHandleSharedPtr>::ID id, bool *found);

public:
    //
    // Unit test API
    //

    /// Set the aggregation strategy for non uniform parameters
    /// (vertex, varying, facevarying)
    /// Takes ownership of the passed in strategy object.
    void SetNonUniformAggregationStrategy(
                std::unique_ptr<HdAggregationStrategy> &&strategy) {
        _nonUniformAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for non uniform immutable parameters
    /// (vertex, varying, facevarying)
    /// Takes ownership of the passed in strategy object.
    void SetNonUniformImmutableAggregationStrategy(
                std::unique_ptr<HdAggregationStrategy> &&strategy) {
        _nonUniformImmutableAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for uniform (shader globals)
    /// Takes ownership of the passed in strategy object.
    void SetUniformAggregationStrategy(
                std::unique_ptr<HdAggregationStrategy> &&strategy) {
        _uniformUboAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for SSBO (uniform primvars)
    /// Takes ownership of the passed in strategy object.
    void SetShaderStorageAggregationStrategy(
                std::unique_ptr<HdAggregationStrategy> &&strategy) {
        _uniformSsboAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for single buffers (for nested instancer).
    /// Takes ownership of the passed in strategy object.
    void SetSingleStorageAggregationStrategy(
                std::unique_ptr<HdAggregationStrategy> &&strategy) {
        _singleAggregationStrategy = std::move(strategy);
    }

    /// Debug dump
    HDST_API
    friend std::ostream &operator <<(
        std::ostream &out,
        const HdStResourceRegistry& self);

protected:
    void _Commit() override;
    void _GarbageCollect() override;
    void _GarbageCollectBprims() override;

private:
    void _CommitTextures();
    // Wrapper function for BAR allocation
    HdBufferArrayRangeSharedPtr _AllocateBufferArrayRange(
        HdAggregationStrategy *strategy,
        HdBufferArrayRegistry &bufferArrayRegistry,
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);
    
    /// Wrapper function for BAR allocation/reallocation-migration.
    HdBufferArrayRangeSharedPtr _UpdateBufferArrayRange(
        HdAggregationStrategy *strategy,
        HdBufferArrayRegistry &bufferArrayRegistry,
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint);

    // Tally resources by key into the given dictionary. Any additions should
    // be cumulative with the existing key values. 
    void _TallyResourceAllocation(VtDictionary *result) const;

    // TODO: this is a transient structure. we'll revisit the BufferSource
    // interface later.
    struct _PendingSource {
        _PendingSource(HdBufferArrayRangeSharedPtr const &range)
            : range(range)
            , sources() 
        {
        }

        _PendingSource(HdBufferArrayRangeSharedPtr const &range,
                       HdBufferSourceSharedPtr     const &source)
            : range(range)
            , sources(1, source)
        {
        }

        _PendingSource(HdBufferArrayRangeSharedPtr const &range,
                       HdBufferSourceSharedPtrVector     && sources)
            : range(range)
            , sources(std::move(sources))
        {
        }

        HdBufferArrayRangeSharedPtr range;
        HdBufferSourceSharedPtrVector sources;
    };

    Hgi* _hgi;

    typedef tbb::concurrent_vector<_PendingSource> _PendingSourceList;
    _PendingSourceList    _pendingSources;
    std::atomic_size_t   _numBufferSourcesToResolve;
    
    struct _PendingComputation{
        _PendingComputation(HdBufferArrayRangeSharedPtr const &range,
                            HdComputationSharedPtr const &computation)
            : range(range), computation(computation) { }
        HdBufferArrayRangeSharedPtr range;
        HdComputationSharedPtr computation;
    };

    typedef tbb::concurrent_vector<_PendingComputation> _PendingComputationList;
    _PendingComputationList  _pendingComputations;

    // aggregated buffer array
    HdBufferArrayRegistry _nonUniformBufferArrayRegistry;
    HdBufferArrayRegistry _nonUniformImmutableBufferArrayRegistry;
    HdBufferArrayRegistry _uniformUboBufferArrayRegistry;
    HdBufferArrayRegistry _uniformSsboBufferArrayRegistry;
    HdBufferArrayRegistry _singleBufferArrayRegistry;

    // current aggregation strategies
    std::unique_ptr<HdAggregationStrategy> _nonUniformAggregationStrategy;
    std::unique_ptr<HdAggregationStrategy>
                                _nonUniformImmutableAggregationStrategy;
    std::unique_ptr<HdAggregationStrategy> _uniformUboAggregationStrategy;
    std::unique_ptr<HdAggregationStrategy> _uniformSsboAggregationStrategy;
    std::unique_ptr<HdAggregationStrategy> _singleAggregationStrategy;

    typedef std::vector<HdStDispatchBufferSharedPtr>
        _DispatchBufferRegistry;
    _DispatchBufferRegistry _dispatchBufferRegistry;

    typedef std::vector<HdStPersistentBufferSharedPtr>
        _PersistentBufferRegistry;
    _PersistentBufferRegistry _persistentBufferRegistry;

    // Register mesh topology.
    HdInstanceRegistry<HdSt_MeshTopologySharedPtr>
        _meshTopologyRegistry;

    // Register basisCurves topology.
    HdInstanceRegistry<HdSt_BasisCurvesTopologySharedPtr>
        _basisCurvesTopologyRegistry;

    // Register vertex adjacency.
    HdInstanceRegistry<Hd_VertexAdjacencySharedPtr>
        _vertexAdjacencyRegistry;

    // Register topology index buffers.
    typedef HdInstanceRegistry<HdBufferArrayRangeSharedPtr>
        _TopologyIndexRangeInstanceRegistry;
    typedef tbb::concurrent_unordered_map< TfToken,
                                           _TopologyIndexRangeInstanceRegistry,
                                           TfToken::HashFunctor >
        _TopologyIndexRangeInstanceRegMap;

    _TopologyIndexRangeInstanceRegMap _meshTopologyIndexRangeRegistry;
    _TopologyIndexRangeInstanceRegMap _basisCurvesTopologyIndexRangeRegistry;

    // Register shared primvar buffers.
    HdInstanceRegistry<HdBufferArrayRangeSharedPtr>
        _primvarRangeRegistry;

    // Register ext computation resource.
    HdInstanceRegistry<HdBufferArrayRangeSharedPtr>
        _extComputationDataRangeRegistry;

    // texture resource registry
    HdInstanceRegistry<HdStTextureResourceSharedPtr>
        _textureResourceRegistry;
    // geometric shader registry
    HdInstanceRegistry<HdSt_GeometricShaderSharedPtr>
        _geometricShaderRegistry;

    // glsl shader program registry
    HdInstanceRegistry<HdStGLSLProgramSharedPtr>
        _glslProgramRegistry;

    // texture resource handle registry
    HdInstanceRegistry<HdStTextureResourceHandleSharedPtr>
        _textureResourceHandleRegistry;

    // texture handle registry
    std::unique_ptr<class HdSt_TextureHandleRegistry> _textureHandleRegistry;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_RESOURCE_REGISTRY_H
