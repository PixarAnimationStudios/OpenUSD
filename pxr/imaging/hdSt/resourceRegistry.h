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

#include <atomic>
#include <map>
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <tbb/concurrent_vector.h>

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferArrayRegistry.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/strategyBase.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdBufferArray> 
    HdBufferArraySharedPtr;
typedef boost::shared_ptr<class HdComputation>
    HdComputationSharedPtr;
typedef boost::shared_ptr<class HdStDispatchBuffer>
    HdStDispatchBufferSharedPtr;
typedef boost::shared_ptr<class HdStGLSLProgram>
    HdStGLSLProgramSharedPtr;
typedef boost::shared_ptr<class HdStPersistentBuffer>
    HdStPersistentBufferSharedPtr;
typedef boost::shared_ptr<class HdStResourceRegistry>
    HdStResourceRegistrySharedPtr;
typedef boost::shared_ptr<class HdStTextureResource>
    HdStTextureResourceSharedPtr;
typedef boost::shared_ptr<class HdStTextureResourceHandle>
    HdStTextureResourceHandleSharedPtr;
typedef boost::shared_ptr<class HdSt_BasisCurvesTopology>
    HdSt_BasisCurvesTopologySharedPtr;
typedef boost::shared_ptr<class HdSt_GeometricShader>
    HdSt_GeometricShaderSharedPtr;
typedef boost::shared_ptr<class HdSt_MeshTopology>
    HdSt_MeshTopologySharedPtr;
typedef boost::shared_ptr<class Hd_VertexAdjacency>
    Hd_VertexAdjacencySharedPtr;

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

    HDST_API
    void InvalidateShaderRegistry() override;

    HDST_API
    VtDictionary GetResourceAllocation() const override;

    /// Allocate new non uniform buffer array range
    HDST_API
    HdBufferArrayRangeSharedPtr AllocateNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Allocate new immutable non uniform buffer array range
    HDST_API
    HdBufferArrayRangeSharedPtr AllocateNonUniformImmutableBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Allocate new uniform buffer range
    HDST_API
    HdBufferArrayRangeSharedPtr AllocateUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Allocate new shader storage buffer range
    HDST_API
    HdBufferArrayRangeSharedPtr AllocateShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Allocate single entry (non-aggregated) buffer array range
    HDST_API
    HdBufferArrayRangeSharedPtr AllocateSingleBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Append source data for given range to be committed later.
    HDST_API
    void AddSources(HdBufferArrayRangeSharedPtr const &range,
                    HdBufferSourceVector &sources);

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
        HdBufferArrayUsageHint newUsageHint,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of non uniform buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayUsageHint newUsageHint,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of non uniform immutable buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeNonUniformImmutableBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayUsageHint newUsageHint,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of uniform buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayUsageHint newUsageHint,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of shader storage buffer.
    HDST_API
    HdBufferArrayRangeSharedPtr MergeShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayUsageHint newUsageHint,
        HdBufferArrayRangeSharedPtr const &range);


    /// Instance Registries
    ///
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
    void SetNonUniformAggregationStrategy(HdAggregationStrategy *strategy) {
        _nonUniformAggregationStrategy.reset(strategy);
    }

    /// Set the aggregation strategy for non uniform immutable parameters
    /// (vertex, varying, facevarying)
    /// Takes ownership of the passed in strategy object.
    void SetNonUniformImmutableAggregationStrategy(
        HdAggregationStrategy *strategy) {
        _nonUniformImmutableAggregationStrategy.reset(strategy);
    }

    /// Set the aggregation strategy for uniform (shader globals)
    /// Takes ownership of the passed in strategy object.
    void SetUniformAggregationStrategy(HdAggregationStrategy *strategy) {
        _uniformUboAggregationStrategy.reset(strategy);
    }

    /// Set the aggregation strategy for SSBO (uniform primvars)
    /// Takes ownership of the passed in strategy object.
    void SetShaderStorageAggregationStrategy(HdAggregationStrategy *strategy) {
        _uniformSsboAggregationStrategy.reset(strategy);
    }

    /// Set the aggregation strategy for single buffers (for nested instancer).
    /// Takes ownership of the passed in strategy object.
    void SetSingleStorageAggregationStrategy(HdAggregationStrategy *strategy) {
        _singleAggregationStrategy.reset(strategy);
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

        HdBufferArrayRangeSharedPtr range;
        HdBufferSourceVector sources;
    };

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

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_RESOURCE_REGISTRY_H
