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
#ifndef HD_RESOURCE_REGISTRY_H
#define HD_RESOURCE_REGISTRY_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <map>
#include <atomic>
#include <tbb/concurrent_vector.h>

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferArrayRegistry.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/shaderKey.h"
#include "pxr/imaging/hd/strategyBase.h"
#include "pxr/imaging/hd/textureResource.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/token.h"

typedef boost::shared_ptr<class HdBufferArray> HdBufferArraySharedPtr;
typedef boost::shared_ptr<class HdMeshTopology> HdMeshTopologySharedPtr;
typedef boost::shared_ptr<class HdBasisCurvesTopology> HdBasisCurvesTopologySharedPtr;
typedef boost::weak_ptr<class HdBufferArrayRange> HdBufferArrayRangePtr;
typedef boost::shared_ptr<class HdComputation> HdComputationSharedPtr;
typedef boost::shared_ptr<class HdGLSLProgram> HdGLSLProgramSharedPtr;
typedef boost::shared_ptr<class HdDispatchBuffer> HdDispatchBufferSharedPtr;
typedef boost::shared_ptr<class HdPersistentBuffer> HdPersistentBufferSharedPtr;
typedef boost::shared_ptr<class Hd_VertexAdjacency> Hd_VertexAdjacencySharedPtr;
typedef boost::shared_ptr<class Hd_GeometricShader> Hd_GeometricShaderSharedPtr;

/// \class HdResourceRegistry
///
/// A central registry of all GPU resources.
///
class HdResourceRegistry : public boost::noncopyable  {
public:
    HD_MALLOC_TAG_NEW("new HdResourceRegistry");

    /// Returns an instance of resource registry
    static HdResourceRegistry& GetInstance() {
        return TfSingleton<HdResourceRegistry>::GetInstance();
    }

    /// Allocate new non uniform buffer array range
    HdBufferArrayRangeSharedPtr AllocateNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Allocate new uniform buffer range
    HdBufferArrayRangeSharedPtr AllocateUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Allocate new shader storage buffer range
    HdBufferArrayRangeSharedPtr AllocateShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Allocate single entry (non-aggregated) buffer array range
    HdBufferArrayRangeSharedPtr AllocateSingleBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs);

    /// Check if \p range is compatible with \p newBufferSpecs.
    /// If not, allocate new bufferArrayRange with merged buffer specs,
    /// register migration computation and return the new range.
    /// Otherwise just return the same range.
    HdBufferArrayRangeSharedPtr MergeBufferArrayRange(
        HdAggregationStrategy *strategy,
        HdBufferArrayRegistry &bufferArrayRegistry,
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of non uniform buffer.
    HdBufferArrayRangeSharedPtr MergeNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of uniform buffer.
    HdBufferArrayRangeSharedPtr MergeUniformBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// MergeBufferArrayRange of shader storage buffer.
    HdBufferArrayRangeSharedPtr MergeShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferSpecVector const &newBufferSpecs,
        HdBufferArrayRangeSharedPtr const &range);

    /// Append source data for given range to be committed later.
    void AddSources(HdBufferArrayRangeSharedPtr const &range,
                    HdBufferSourceVector &sources);

    /// Append a source data for given range to be committed later.
    void AddSource(HdBufferArrayRangeSharedPtr const &range,
                   HdBufferSourceSharedPtr const &source);

    /// Append a source data just to be resolved (used for cpu computations).
    void AddSource(HdBufferSourceSharedPtr const &source);

    /// Append a gpu computation into queue.
    /// The parameter 'range' specifies the destination buffer range,
    /// which has to be allocated by caller of this function.
    ///
    /// note: GPU computations will be executed in the order that
    /// they are registered.
    void AddComputation(HdBufferArrayRangeSharedPtr const &range,
                        HdComputationSharedPtr const &computaion);

    /// Commits all in-flight source data to the GPU, freeing the source
    /// buffers.
    void Commit();

    /// cleanup all buffers and remove if empty
    void GarbageCollect();

    /// Set the aggregation strategy for non uniform parameters
    /// (vertex, varying, facevarying)
    void SetNonUniformAggregationStrategy(HdAggregationStrategy *strategy) {
        _nonUniformAggregationStrategy = strategy;
    }

    /// Set the aggregation strategy for uniform (shader globals)
    void SetUniformAggregationStrategy(HdAggregationStrategy *strategy) {
        _uniformUboAggregationStrategy = strategy;
    }

    /// Set the aggregation strategy for SSBO (uniform primvars)
    void SetShaderStorageAggregationStrategy(HdAggregationStrategy *strategy) {
        _uniformSsboAggregationStrategy = strategy;
    }

    /// Returns a report of resource allocation by role in bytes and
    /// a summary total allocation of GPU memory in bytes for this registry.
    VtDictionary GetResourceAllocation() const;

    /// Topology instancing
    /// Returns (in the pointer) the HdInstance points to shared HdMeshTopology,
    /// distinguished by given ID. If IsFirstInstance() of the instance is
    /// true, the caller is responsible to initialize the HdMeshTopology.
    /// Also the HdMeshTopology has to be owned by someone else, otherwise
    /// the entry will get deleted on GarbageCollect(). HdInstance is
    /// intended to be used for temporary pointer, so the caller should not
    /// hold the instance for an extended time. ID is used as a hash key
    /// and the resolution of hash collision is the client's responsibility.
    ///
    /// Note: As entries can be added by multiple threads the routine
    /// returns a lock on the instance registry.  This lock should be held
    /// until the HdInstance object is destroyed.
    std::unique_lock<std::mutex> RegisterMeshTopology(HdTopology::ID id, 
         HdInstance<HdTopology::ID, HdMeshTopologySharedPtr> *pInstance);

    std::unique_lock<std::mutex> RegisterBasisCurvesTopology(HdTopology::ID id,
         HdInstance<HdTopology::ID, HdBasisCurvesTopologySharedPtr> *pInstance);

    std::unique_lock<std::mutex> RegisterVertexAdjacency(HdTopology::ID id,
         HdInstance<HdTopology::ID, Hd_VertexAdjacencySharedPtr> *pInstance);

    /// Index buffer array range instancing
    /// Returns the HdInstance points to shared HdBufferArrayRange,
    /// distinguished by given ID.
    /// *Refer the comment on RegisterTopology for the same consideration.
    std::unique_lock<std::mutex> RegisterMeshIndexRange(HdTopology::ID id, TfToken const &name,
         HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr> *pInstance);

    std::unique_lock<std::mutex> RegisterBasisCurvesIndexRange(HdTopology::ID id, TfToken const &name,
         HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr> *pInstance);

    /// Registere a geometric shader.
    std::unique_lock<std::mutex> RegisterGeometricShader(HdShaderKey::ID id,
         HdInstance<HdShaderKey::ID, Hd_GeometricShaderSharedPtr> *pInstance);

    /// Register a GLSL program into the program registry.
    /// note: Currently no garbage collection enforced on the shader registry
    std::unique_lock<std::mutex> RegisterGLSLProgram(HdGLSLProgram::ID id,
        HdInstance<HdGLSLProgram::ID, HdGLSLProgramSharedPtr> *pInstance);

    /// Register a texture into the texture registry.
    /// XXX garbage collection?
    std::unique_lock<std::mutex> RegisterTextureResource(HdTextureResource::ID id,
         HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr> *pInstance);

    /// Find a texture in the texture registry. If found, it returns it.
    std::unique_lock<std::mutex> FindTextureResource(HdTextureResource::ID id,
         HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr> *instance, 
         bool *found);

    /// Register a buffer allocated with \a count * \a commandNumUints *
    /// sizeof(GLuint) to be used as an indirect dispatch buffer.
    HdDispatchBufferSharedPtr RegisterDispatchBuffer(
        TfToken const &role, int count, int commandNumUints);

    /// Register a buffer initialized with \a dataSize bytes of \a data
    /// to be used as a persistently mapped shader storage buffer.
    HdPersistentBufferSharedPtr RegisterPersistentBuffer(
        TfToken const &role, size_t dataSize, void *data);

    void InvalidateGeometricShaderRegistry();

    /// Remove any entries associated with expired dispatch buffers.
    void GarbageCollectDispatchBuffers();

    /// Remove any entries associated with expired persistently mapped buffers.
    void GarbageCollectPersistentBuffers();

    /// Debug dump
    friend std::ostream &operator <<(std::ostream &out,
                                     const HdResourceRegistry& self);

private:
    friend class TfSingleton<HdResourceRegistry>;

    HdResourceRegistry();
    ~HdResourceRegistry();

    // aggregated buffer array
    HdBufferArrayRegistry _nonUniformBufferArrayRegistry;
    HdBufferArrayRegistry _uniformUboBufferArrayRegistry;
    HdBufferArrayRegistry _uniformSsboBufferArrayRegistry;
    HdBufferArrayRegistry _singleBufferArrayRegistry;

    // current aggregation strategies
    HdAggregationStrategy *_nonUniformAggregationStrategy;
    HdAggregationStrategy *_uniformUboAggregationStrategy;
    HdAggregationStrategy *_uniformSsboAggregationStrategy;
    HdAggregationStrategy *_singleAggregationStrategy;

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


    // instancing registries
    //   meshTopology to HdMeshTopology
    typedef HdInstance<HdTopology::ID, HdMeshTopologySharedPtr>
        _MeshTopologyInstance;
    HdInstanceRegistry<_MeshTopologyInstance> _meshTopologyRegistry;

    typedef HdInstance<HdTopology::ID, HdBasisCurvesTopologySharedPtr>
        _BasisCurvesTopologyInstance;
    HdInstanceRegistry<_BasisCurvesTopologyInstance> _basisCurvesTopologyRegistry;

    //   topology to vertex adjacency
    typedef HdInstance<HdTopology::ID, Hd_VertexAdjacencySharedPtr>
        _VertexAdjacencyInstance;
    HdInstanceRegistry<_VertexAdjacencyInstance> _vertexAdjacencyRegistry;

    //   topology to HdBufferArrayRange
    typedef HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr>
        _TopologyIndexRangeInstance;
    typedef tbb::concurrent_unordered_map<TfToken, HdInstanceRegistry<_TopologyIndexRangeInstance>, TfToken::HashFunctor >
        _TopologyIndexRangeInstanceRegMap;

    _TopologyIndexRangeInstanceRegMap _meshTopologyIndexRangeRegistry;
    _TopologyIndexRangeInstanceRegMap _basisCurvesTopologyIndexRangeRegistry;

    // geometric shader registry
    typedef HdInstance<HdShaderKey::ID, Hd_GeometricShaderSharedPtr>
         _GeometricShaderInstance;
    HdInstanceRegistry<_GeometricShaderInstance> _geometricShaderRegistry;

    // glsl shader program registry
    typedef HdInstance<HdGLSLProgram::ID, HdGLSLProgramSharedPtr>
        _GLSLProgramInstance;
    HdInstanceRegistry<_GLSLProgramInstance> _glslProgramRegistry;

    // texture resource registry
    typedef HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr>
         _TextureResourceRegistry;
    HdInstanceRegistry<_TextureResourceRegistry> _textureResourceRegistry;

    typedef std::vector<HdDispatchBufferSharedPtr>
        _DispatchBufferRegistry;
    _DispatchBufferRegistry _dispatchBufferRegistry;

    typedef std::vector<HdPersistentBufferSharedPtr>
        _PersistentBufferRegistry;
    _PersistentBufferRegistry _persistentBufferRegistry;

};

#endif //HD_RESOURCE_REGISTRY_H
