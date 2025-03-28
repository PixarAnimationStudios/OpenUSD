//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_RESOURCE_REGISTRY_H
#define PXR_IMAGING_HD_ST_RESOURCE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/bufferArrayRegistry.h"
#include "pxr/imaging/hdSt/enums.h"

#include "pxr/imaging/hgi/hgi.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/resourceRegistry.h"

#include <tbb/concurrent_vector.h>

#include <atomic>
#include <map>
#include <memory>

#ifdef PXR_MATERIALX_SUPPORT_ENABLED
#include <MaterialXCore/Library.h>
MATERIALX_NAMESPACE_BEGIN
    using ShaderPtr = std::shared_ptr<class Shader>;
MATERIALX_NAMESPACE_END
#endif

PXR_NAMESPACE_OPEN_SCOPE

using HdStComputationSharedPtr = std::shared_ptr<class HdStComputation>;
using HdStDispatchBufferSharedPtr = std::shared_ptr<class HdStDispatchBuffer>;
using HdStGLSLProgramSharedPtr = std::shared_ptr<class HdStGLSLProgram>;
using HioGlslfxSharedPtr = std::shared_ptr<class HioGlslfx>;

using HdSt_BasisCurvesTopologySharedPtr =
    std::shared_ptr<class HdSt_BasisCurvesTopology>;

using HdStShaderCodePtr =
    std::weak_ptr<class HdStShaderCode>;
using HdSt_GeometricShaderSharedPtr =
    std::shared_ptr<class HdSt_GeometricShader>;
using HdStRenderPassShaderSharedPtr =
    std::shared_ptr<class HdStRenderPassShader>;

using HdStTextureHandleSharedPtr =
    std::shared_ptr<class HdStTextureHandle>;
using HdStTextureObjectSharedPtr =
    std::shared_ptr<class HdStTextureObject>;
using HdStBufferResourceSharedPtr = 
    std::shared_ptr<class HdStBufferResource>;
using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;
using HdSt_VertexAdjacencyBuilderSharedPtr = 
    std::shared_ptr<class HdSt_VertexAdjacencyBuilder>;
using HdSt_MeshTopologySharedPtr = 
    std::shared_ptr<class HdSt_MeshTopology>;
using HgiResourceBindingsSharedPtr = 
    std::shared_ptr<HgiResourceBindingsHandle>;
using HgiGraphicsPipelineSharedPtr = 
    std::shared_ptr<HgiGraphicsPipelineHandle>;
using HgiComputePipelineSharedPtr = 
    std::shared_ptr<HgiComputePipelineHandle>;

class HdStTextureIdentifier;
class HdSamplerParameters;
class HdStStagingBuffer;

/// \enum HdStComputeQueue
///
/// Determines the 'compute queue' a computation should be added into.
///
/// We only perform synchronization between queues, not within one queue.
/// In OpenGL terms that means we insert memory barriers between computations
/// of two queues, but not between two computations in the same queue.
///
/// A prim determines the role for each queue based on its local knowledge of
/// compute dependencies. Eg. HdStMesh knows computing normals should wait
/// until the primvar refinement computation has fnished. It can assign one
/// queue to primvar refinement and a following queue for normal computations.
///
enum HdStComputeQueue {
    HdStComputeQueueZero=0,
    HdStComputeQueueOne,
    HdStComputeQueueTwo,
    HdStComputeQueueThree,
    HdStComputeQueueCount};

using HdStComputationComputeQueuePairVector = 
    std::vector<std::pair<HdStComputationSharedPtr, HdStComputeQueue>>;


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
    void ReloadResource(TfToken const& resourceType,
                        std::string const& path) override;

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
    /// object, memory request and callback to shader).
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
        HdStTextureType textureType,
        /// Sampling parameters such as wrapS, ...
        /// wrapS, wrapT, wrapR mode, min filer, mag filter
        const HdSamplerParameters &samplerParams,
        /// Memory request. The texture is down-sampled to meet the
        /// target memory which is the maximum of all memory requests
        /// associated to the texture.
        /// If all memory requests are 0, no down-sampling will happen.
        size_t memoryRequest,
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
        HdStTextureType textureType);

    /// Sets how much memory a single texture can consume in bytes by
    /// texture type.
    ///
    /// Only has an effect if non-zero and only applies to textures if
    /// no texture handle referencing the texture has a memory
    /// request.
    ///
    HDST_API
    void SetMemoryRequestForTextureType(
        HdStTextureType textureType,
        size_t memoryRequest);

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
                        HdStComputationSharedPtr const &computation,
                        HdStComputeQueue const queue);

    /// ------------------------------------------------------------------------
    /// Dispatch & buffer API
    /// ------------------------------------------------------------------------

    /// Register a buffer allocated with \a count * \a commandNumUints *
    /// sizeof(uint32_t) to be used as an indirect dispatch buffer.
    HDST_API
    HdStDispatchBufferSharedPtr RegisterDispatchBuffer(
        TfToken const &role, int count, int commandNumUints);

    /// Register a misc buffer resource.
    /// Usually buffers are part of a buffer array (buffer aggregation) and are
    /// managed via buffer array APIs.
    /// RegisterBufferResource lets you create a standalone buffer that can
    /// be used for misc purposes (Eg. GPU frustum cull prim count read back).
    HDST_API
    HdStBufferResourceSharedPtr RegisterBufferResource(
        TfToken const &role, 
        HdTupleType tupleType,
        HgiBufferUsage bufferUsage,
        std::string debugName = "");

    /// Remove any entries associated with expired dispatch buffers.
    HDST_API
    void GarbageCollectDispatchBuffers();

    /// Remove any entries associated with expired misc buffers.
    HDST_API
    void GarbageCollectBufferResources();

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
    HdInstance<HdSt_VertexAdjacencyBuilderSharedPtr>
    RegisterVertexAdjacencyBuilder(
        HdInstance<HdSt_VertexAdjacencyBuilderSharedPtr>::ID id);

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

    /// Register a geometric shader.
    HDST_API
    HdInstance<HdSt_GeometricShaderSharedPtr>
    RegisterGeometricShader(HdInstance<HdSt_GeometricShaderSharedPtr>::ID id);

    /// Register a render pass shader.
    HDST_API
    HdInstance<HdStRenderPassShaderSharedPtr>
    RegisterRenderPassShader(HdInstance<HdStRenderPassShaderSharedPtr>::ID id);

    /// Register a GLSL program into the program registry.
    HDST_API
    HdInstance<HdStGLSLProgramSharedPtr>
    RegisterGLSLProgram(HdInstance<HdStGLSLProgramSharedPtr>::ID id);

    /// Register a GLSLFX file.
    HDST_API
    HdInstance<HioGlslfxSharedPtr>
    RegisterGLSLFXFile(HdInstance<HioGlslfxSharedPtr>::ID id);

#ifdef PXR_MATERIALX_SUPPORT_ENABLED
    /// Register MaterialX GLSLFX Shader.
    HDST_API
    HdInstance<MaterialX::ShaderPtr>
    RegisterMaterialXShader(HdInstance<MaterialX::ShaderPtr>::ID id);
#endif

    /// Register a Hgi resource bindings into the registry.
    HDST_API
    HdInstance<HgiResourceBindingsSharedPtr>
    RegisterResourceBindings(HdInstance<HgiResourceBindingsSharedPtr>::ID id);

    /// Register a Hgi graphics pipeline into the registry.
    HDST_API
    HdInstance<HgiGraphicsPipelineSharedPtr>
    RegisterGraphicsPipeline(HdInstance<HgiGraphicsPipelineSharedPtr>::ID id);

    /// Register a Hgi compute pipeline into the registry.
    HDST_API
    HdInstance<HgiComputePipelineSharedPtr>
    RegisterComputePipeline(HdInstance<HgiComputePipelineSharedPtr>::ID id);

    /// Finds a sub resource registry for the given identifier if it already
    /// exists or creates one by invoking the provided factory function.
    ///
    /// A sub resource registry is simply another resource registry defined
    /// externally but whose lifetime is tied to this HdStResourceRegistry and
    /// can be retrieved via this function.
    ///
    /// Any sub resource registries will have their Commit functions invoked
    /// when Commit is invoked on this instance, and their GarbageCollect
    /// functions invoked when GarbageCollect is invoked on this instance.
    HDST_API
    HdResourceRegistry*
    FindOrCreateSubResourceRegistry(
        const std::string& identifier,
        const std::function<std::unique_ptr<HdResourceRegistry>()>& factory);

    /// Returns the global hgi blit command queue for recording blitting work.
    /// When using this global cmd instead of creating a new HgiBlitCmds we
    /// reduce the number of command buffers being created.
    /// The returned pointer should not be held onto by the client as it is
    /// only valid until the HgiBlitCmds has been submitted.
    HDST_API
    HgiBlitCmds* GetGlobalBlitCmds();

    /// Returns the global hgi compute cmd queue for recording compute work.
    /// When using this global cmd instead of creating a new HgiComputeCmds we
    /// reduce the number of command buffers being created.
    /// The returned pointer should not be held onto by the client as it is
    /// only valid until the HgiComputeCmds has been submitted.
    HDST_API
    HgiComputeCmds* GetGlobalComputeCmds(
        HgiComputeDispatch dispatchMethod = HgiComputeDispatchSerial);

    /// Submits blit work queued in global blit cmds for GPU execution.
    /// We can call this when we want to submit some work to the GPU.
    /// To stall the CPU and wait for the GPU to finish, 'wait' can be provided.
    /// To insert a barrier to ensure memory writes are visible after the
    /// barrier a HgiMemoryBarrier can be provided.
    HDST_API
    void SubmitBlitWork(HgiSubmitWaitType wait = HgiSubmitWaitTypeNoWait);

    /// Submits compute work queued in global compute cmds for GPU execution.
    /// We can call this when we want to submit some work to the GPU.
    /// To stall the CPU and wait for the GPU to finish, 'wait' can be provided.
    /// To insert a barrier to ensure memory writes are visible after the
    /// barrier a HgiMemoryBarrier can be provided.
    HDST_API
    void SubmitComputeWork(HgiSubmitWaitType wait = HgiSubmitWaitTypeNoWait);

    /// Returns the staging buffer used when committing data to the GPU.
    HDST_API
    HdStStagingBuffer* GetStagingBuffer();

public:
    //
    // Unit test API
    //

    /// Set the aggregation strategy for non uniform parameters
    /// (vertex, varying, facevarying)
    /// Takes ownership of the passed in strategy object.
    void SetNonUniformAggregationStrategy(
                std::unique_ptr<HdStAggregationStrategy> &&strategy) {
        _nonUniformAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for non uniform immutable parameters
    /// (vertex, varying, facevarying)
    /// Takes ownership of the passed in strategy object.
    void SetNonUniformImmutableAggregationStrategy(
                std::unique_ptr<HdStAggregationStrategy> &&strategy) {
        _nonUniformImmutableAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for uniform (shader globals)
    /// Takes ownership of the passed in strategy object.
    void SetUniformAggregationStrategy(
                std::unique_ptr<HdStAggregationStrategy> &&strategy) {
        _uniformUboAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for SSBO (uniform primvars)
    /// Takes ownership of the passed in strategy object.
    void SetShaderStorageAggregationStrategy(
                std::unique_ptr<HdStAggregationStrategy> &&strategy) {
        _uniformSsboAggregationStrategy = std::move(strategy);
    }

    /// Set the aggregation strategy for single buffers (for nested instancer).
    /// Takes ownership of the passed in strategy object.
    void SetSingleStorageAggregationStrategy(
                std::unique_ptr<HdStAggregationStrategy> &&strategy) {
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

private:
    void _CommitTextures();
    // Wrapper function for BAR allocation
    HdBufferArrayRangeSharedPtr _AllocateBufferArrayRange(
        HdStAggregationStrategy *strategy,
        HdStBufferArrayRegistry &bufferArrayRegistry,
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);
    
    /// Wrapper function for BAR allocation/reallocation-migration.
    HdBufferArrayRangeSharedPtr _UpdateBufferArrayRange(
        HdStAggregationStrategy *strategy,
        HdStBufferArrayRegistry &bufferArrayRegistry,
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
    std::atomic_size_t    _numBufferSourcesToResolve;
    
    struct _PendingComputation{
        _PendingComputation(HdBufferArrayRangeSharedPtr const &range,
                            HdStComputationSharedPtr const &computation)
            : range(range), computation(computation) { }
        HdBufferArrayRangeSharedPtr range;
        HdStComputationSharedPtr computation;
    };

    // If we need more 'compute queues' we can increase HdStComputeQueueCount.
    // We could also consider tbb::concurrent_priority_queue if we want this
    // to be dynamically scalable.
    typedef tbb::concurrent_vector<_PendingComputation> _PendingComputationList;
    _PendingComputationList  _pendingComputations[HdStComputeQueueCount];

    // aggregated buffer array
    HdStBufferArrayRegistry _nonUniformBufferArrayRegistry;
    HdStBufferArrayRegistry _nonUniformImmutableBufferArrayRegistry;
    HdStBufferArrayRegistry _uniformUboBufferArrayRegistry;
    HdStBufferArrayRegistry _uniformSsboBufferArrayRegistry;
    HdStBufferArrayRegistry _singleBufferArrayRegistry;

    // current aggregation strategies
    std::unique_ptr<HdStAggregationStrategy> _nonUniformAggregationStrategy;
    std::unique_ptr<HdStAggregationStrategy>
                                _nonUniformImmutableAggregationStrategy;
    std::unique_ptr<HdStAggregationStrategy> _uniformUboAggregationStrategy;
    std::unique_ptr<HdStAggregationStrategy> _uniformSsboAggregationStrategy;
    std::unique_ptr<HdStAggregationStrategy> _singleAggregationStrategy;

    typedef std::vector<HdStDispatchBufferSharedPtr>
        _DispatchBufferRegistry;
    _DispatchBufferRegistry _dispatchBufferRegistry;

    typedef std::vector<HdStBufferResourceSharedPtr>
        _BufferResourceRegistry;
    _BufferResourceRegistry _bufferResourceRegistry;

    // Register mesh topology.
    HdInstanceRegistry<HdSt_MeshTopologySharedPtr>
        _meshTopologyRegistry;

    // Register basisCurves topology.
    HdInstanceRegistry<HdSt_BasisCurvesTopologySharedPtr>
        _basisCurvesTopologyRegistry;

    // Register vertex adjacency.
    HdInstanceRegistry<HdSt_VertexAdjacencyBuilderSharedPtr>
        _vertexAdjacencyBuilderRegistry;

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

    // geometric shader registry
    HdInstanceRegistry<HdSt_GeometricShaderSharedPtr>
        _geometricShaderRegistry;
    
    // render pass shader registry
    HdInstanceRegistry<HdStRenderPassShaderSharedPtr>
        _renderPassShaderRegistry;

    // glsl shader program registry
    HdInstanceRegistry<HdStGLSLProgramSharedPtr>
        _glslProgramRegistry;

    // glslfx file registry
    HdInstanceRegistry<HioGlslfxSharedPtr>
        _glslfxFileRegistry;

#ifdef PXR_MATERIALX_SUPPORT_ENABLED
    // MaterialX glslfx shader registry
    HdInstanceRegistry<MaterialX::ShaderPtr> _materialXShaderRegistry;
#endif

    // texture handle registry
    std::unique_ptr<class HdSt_TextureHandleRegistry> _textureHandleRegistry;

    // Hgi resource bindings registry
    HdInstanceRegistry<HgiResourceBindingsSharedPtr>
        _resourceBindingsRegistry;

    // Hgi graphics pipeline registry
    HdInstanceRegistry<HgiGraphicsPipelineSharedPtr>
        _graphicsPipelineRegistry;

    // Hgi compute pipeline registry
    HdInstanceRegistry<HgiComputePipelineSharedPtr>
        _computePipelineRegistry;

    using _SubResourceRegistryMap =
        tbb::concurrent_unordered_map<std::string,
                                    std::unique_ptr<HdResourceRegistry>>;
    _SubResourceRegistryMap _subResourceRegistries;

    HgiBlitCmdsUniquePtr _blitCmds;
    HgiComputeCmdsUniquePtr _computeCmds;

    std::unique_ptr<HdStStagingBuffer> _stagingBuffer;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_RESOURCE_REGISTRY_H
