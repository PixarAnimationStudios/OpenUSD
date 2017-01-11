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
#include "pxr/imaging/hd/resourceRegistry.h"

#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferResource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/copyComputation.h"
#include "pxr/imaging/hd/dispatchBuffer.h"
#include "pxr/imaging/hd/persistentBuffer.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/interleavedMemoryManager.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vboMemoryManager.h"
#include "pxr/imaging/hd/vboSimpleMemoryManager.h"
#include "pxr/imaging/hd/vertexAdjacency.h"

#include "pxr/imaging/glf/textureRegistry.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/work/loops.h"

#include <boost/bind.hpp>
 
#include <algorithm>
#include <iostream>
#include <set>

TF_INSTANTIATE_SINGLETON(HdResourceRegistry);

HdResourceRegistry::HdResourceRegistry() :
    _nonUniformAggregationStrategy(NULL),
    _uniformUboAggregationStrategy(NULL),
    _uniformSsboAggregationStrategy(NULL),
    _singleAggregationStrategy(NULL),
    _numBufferSourcesToResolve(0)
{
    // default aggregation strategy for varying (vertex, varying) primvars
    _nonUniformAggregationStrategy = &HdVBOMemoryManager::GetInstance();

    // default aggregation strategy for uniform on SSBO (for primvars)
    _uniformSsboAggregationStrategy = &HdInterleavedSSBOMemoryManager::GetInstance();

    // default aggregation strategy for uniform on UBO (for globals)
    _uniformUboAggregationStrategy = &HdInterleavedUBOMemoryManager::GetInstance();

    // default aggregation strategy for single buffers (for nested instancer)
    _singleAggregationStrategy = &HdVBOSimpleMemoryManager::GetInstance();
}

HdResourceRegistry::~HdResourceRegistry()
{
    /*NOTHING*/
}




HdBufferArrayRangeSharedPtr
HdResourceRegistry::AllocateNonUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    return _nonUniformBufferArrayRegistry.AllocateRange(
                                    _nonUniformAggregationStrategy,
                                    role,
                                    bufferSpecs);
}

HdBufferArrayRangeSharedPtr
HdResourceRegistry::AllocateUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    return _uniformUboBufferArrayRegistry.AllocateRange(
                                    _uniformUboAggregationStrategy,
                                    role,
                                    bufferSpecs);
}

HdBufferArrayRangeSharedPtr
HdResourceRegistry::AllocateShaderStorageBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    return _uniformSsboBufferArrayRegistry.AllocateRange(
                                    _uniformSsboAggregationStrategy,
                                    role,
                                    bufferSpecs);
}

HdBufferArrayRangeSharedPtr
HdResourceRegistry::AllocateSingleBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs)
{
    return _singleBufferArrayRegistry.AllocateRange(
                                    _singleAggregationStrategy,
                                    role,
                                    bufferSpecs);
}

HdBufferArrayRangeSharedPtr
HdResourceRegistry::MergeBufferArrayRange(
    HdAggregationStrategy *strategy,
    HdBufferArrayRegistry &bufferArrayRegistry,
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayRangeSharedPtr const &range)
{
    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(range)) return HdBufferArrayRangeSharedPtr();

    // get existing buffer specs
    HdBufferSpecVector oldBufferSpecs;
    range->AddBufferSpecs(&oldBufferSpecs);

    // compare bufferspec
    if (!HdBufferSpec::IsSubset(newBufferSpecs, oldBufferSpecs)) {
        // create / moveto the new buffer array.

        HdComputationVector computations;

        // existing content has to be transferred.
        TF_FOR_ALL(it, oldBufferSpecs) {
            if (std::find(newBufferSpecs.begin(), newBufferSpecs.end(), *it)
                == newBufferSpecs.end()) {

                // migration computation
                computations.push_back(
                    HdComputationSharedPtr(new HdCopyComputationGPU(
                                               /*src=*/range, it->name)));
            }
        }
        // new buffer array should have a union of
        // new buffer specs and exsiting buffer specs.
        HdBufferSpecVector bufferSpecs = HdBufferSpec::ComputeUnion(
            newBufferSpecs, oldBufferSpecs);

        // allocate new range.
        HdBufferArrayRangeSharedPtr result = bufferArrayRegistry.AllocateRange(
            strategy, role, bufferSpecs);

        // register copy computation.
        if (!computations.empty()) {
            TF_FOR_ALL(it, computations) {
                AddComputation(result, *it);
            }
        }

        // The source range will be no longer used.
        // Increment version of the underlying bufferArray to notify
        // all batches pointing the range to be rebuilt.
        //
        // XXX: Currently we have migration computations for each individual
        // ranges, so the version is being incremented redundantly.
        // It shouldn't be a big issue, but we can put several range
        // computations into single computation to avoid that redundancy
        // if we like. Or alternatively the change tracker can take care of it.
        range->IncrementVersion();

        HD_PERF_COUNTER_INCR(HdPerfTokens->bufferArrayRangeMerged);

        return result;
    }

    return range;
}

HdBufferArrayRangeSharedPtr
HdResourceRegistry::MergeNonUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayRangeSharedPtr const &range)
{
    return MergeBufferArrayRange(_nonUniformAggregationStrategy,
                                 _nonUniformBufferArrayRegistry,
                                 role, newBufferSpecs, range);
}

HdBufferArrayRangeSharedPtr
HdResourceRegistry::MergeUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayRangeSharedPtr const &range)
{
    return MergeBufferArrayRange(_uniformUboAggregationStrategy,
                                 _uniformUboBufferArrayRegistry,
                                 role, newBufferSpecs, range);
}

HdBufferArrayRangeSharedPtr
HdResourceRegistry::MergeShaderStorageBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayRangeSharedPtr const &range)
{
    return MergeBufferArrayRange(_uniformSsboAggregationStrategy,
                                 _uniformSsboBufferArrayRegistry,
                                 role, newBufferSpecs, range);
}

void
HdResourceRegistry::AddSources(HdBufferArrayRangeSharedPtr const &range,
                               HdBufferSourceVector &sources)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (ARCH_UNLIKELY(sources.empty()))
    {
        TF_RUNTIME_ERROR("sources list is empty");
        return;
    }

    // range has to be valid
    if (ARCH_UNLIKELY(!(range && range->IsValid())))
    {
        TF_RUNTIME_ERROR("range is null or invalid");
        return;
    }

    // Check that each buffer is valid and if not erase it from the list
    // Can not use standard iterators here as erasing invalidates them
    // also the vector is unordered, so we can do a quick erase
    // by moving the item off the end of the vector.
    size_t srcNum = 0;
    while (srcNum < sources.size()) {
        if (ARCH_LIKELY(sources[srcNum]->IsValid())) {
            ++srcNum;
        } else {
            TF_RUNTIME_ERROR("Source Buffer for %s is invalid", sources[srcNum]->GetName().GetText());
            
            // Move the last item in the vector over
            // this one.  If it is the last item
            // it will copy over itself and the pop
            // will remove it anyway.
            sources[srcNum] = sources.back();
            sources.pop_back();

            // Don't increament srcNum as it now points
            // to the new item or is off the end of the vector
        }
    }

    // Check for no-valid buffer case
    if (!sources.empty()) {
        _PendingSourceList::iterator it = _pendingSources.emplace_back(range);
        TF_VERIFY(range.use_count() >=2);

        std::swap(it->sources, sources);
        
        _numBufferSourcesToResolve += it->sources.size(); // Atomic
    }
}

void
HdResourceRegistry::AddSource(HdBufferArrayRangeSharedPtr const &range,
                              HdBufferSourceSharedPtr const &source)
{
    if (ARCH_UNLIKELY((!source) || (!range)))
    {
        TF_RUNTIME_ERROR("An input pointer is null");
        return;
    }

    // range has to be valid
    if (ARCH_UNLIKELY(!range->IsValid()))
    {
        TF_RUNTIME_ERROR("range is invalid");
        return;
    }

    // Buffer has to be valid
    if (ARCH_UNLIKELY(!source->IsValid()))
    {
        TF_RUNTIME_ERROR("source buffer for %s is invalid", source->GetName().GetText());
        return;
    }

    _pendingSources.emplace_back(range, source);
    ++_numBufferSourcesToResolve;  // Atomic
}

void
HdResourceRegistry::AddSource(HdBufferSourceSharedPtr const &source)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();
    if (ARCH_UNLIKELY(!source))
    {
        TF_RUNTIME_ERROR("source pointer is null");
        return;
    }


    // Buffer has to be valid
    if (ARCH_UNLIKELY(!source->IsValid()))
    {
        TF_RUNTIME_ERROR("source buffer for %s is invalid", source->GetName().GetText());
        return;
    }

    _pendingSources.emplace_back(HdBufferArrayRangeSharedPtr(), source);


    ++_numBufferSourcesToResolve; // Atomic
}

void
HdResourceRegistry::AddComputation(HdBufferArrayRangeSharedPtr const &range,
                                   HdComputationSharedPtr const &computation)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // if the computation is buffer source computation, it will be appended
    // into pendingBufferSourceComputations, which is executed right after
    // the first buffer source transfers. Those computations produce
    // buffer sources as results of computation, so the registry also invokes
    // another transfers for such buffers. The computation isn't marked
    // as a buffer source computation will be executed at the end.

    _pendingComputations.emplace_back(range, computation);
}

void
HdResourceRegistry::Commit()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // TODO: requests should be sorted by resource, and range.
    {
        HD_TRACE_SCOPE("Resolve");
        // 1. resolve & resize phase:
        // for each pending source, resolve and check if it needs buffer
        // reallocation or not.

        size_t numBufferSourcesResolved = 0;
        int numThreads = 1; //omp_get_max_threads();
        int numIterations = 0;

        // iterate until all buffer sources have been resolved.
        while (numBufferSourcesResolved < _numBufferSourcesToResolve) {
            // XXX: Parallel for is currently much slower than a single
            // thread in all tested scenarios, disabling until we can figure out
            // what's going on here.
//#pragma omp parallel for
            for (int i = 0; i < numThreads; ++i) {
                // iterate over all pending sources
                for (_PendingSourceList::iterator reqIt = _pendingSources.begin();
                     reqIt != _pendingSources.end(); ++reqIt) {
                    TF_FOR_ALL(sourceIt, reqIt->sources) {
                        // execute computation.
                        // call IsResolved first since Resolve is virtual and
                        // could be costly.
                        if (!(*sourceIt)->IsResolved()) {
                            if ((*sourceIt)->Resolve()) {
                                TF_VERIFY((*sourceIt)->IsResolved());

                                ++numBufferSourcesResolved;

                                // call resize if it's the first source in sources.
                                if (reqIt->range &&
                                    (sourceIt.base() == reqIt->sources.begin())) {
                                    reqIt->range->Resize(
                                        (*sourceIt)->GetNumElements());
                                }
                            }
                        }
                    }
                }
            }
            if (++numIterations > 100) {
                TF_WARN("Too many iterations in resolving buffer source. "
                        "It's likely due to incosistent dependency.");
                break;
            }
        }

        TF_VERIFY(numBufferSourcesResolved == _numBufferSourcesToResolve);
        HD_PERF_COUNTER_ADD(HdPerfTokens->bufferSourcesResolved,
                            numBufferSourcesResolved);
    }

    {
        HD_TRACE_SCOPE("GPU computation prep");
        // 2. GPU computation prep phase:
        // for each gpu computation, make sure its destination buffer to be
        // allocated.
        //
        TF_FOR_ALL(compIt, _pendingComputations) {
            if (compIt->range) {
                // ask the size of destination buffer of the gpu computation
                int numElements = compIt->computation->GetNumOutputElements();
                if (numElements > 0) {
                    // We call BufferArray->Reallocate() later so that
                    // the reallocation happens only once per BufferArray.
                    //
                    // if the range is already larger than the current one,
                    // leave it as it is (there is a possibilty that GPU
                    // computation generates less data than it was).
                    int currentNumElements = compIt->range->GetNumElements();
                    if (currentNumElements < numElements) {
                        compIt->range->Resize(numElements);
                    }
                 }
            }
        }
    }

    {
        HD_TRACE_SCOPE("Reallocate buffer arrays");
        // 3. reallocation phase:
        //
        _nonUniformBufferArrayRegistry.ReallocateAll(
            _nonUniformAggregationStrategy);
        _uniformUboBufferArrayRegistry.ReallocateAll(
            _uniformUboAggregationStrategy);
        _uniformSsboBufferArrayRegistry.ReallocateAll(
            _uniformSsboAggregationStrategy);
        _singleBufferArrayRegistry.ReallocateAll(
            _singleAggregationStrategy);
    }

    {
        HD_TRACE_SCOPE("Copy");
        // 4. copy phase:
        //

        TF_FOR_ALL(reqIt, _pendingSources) {
            // CPU computation may not have a range. (e.g. adjacency)
            if (!reqIt->range) continue;

            // CPU computation may result in an empty buffer source
            // (e.g. GPU quadrangulation table could be empty for quad only
            // mesh)
            if (reqIt->range->GetNumElements() == 0) continue;

            // Note that for staticArray in interleavedVBO,
            // it's possible range->GetNumElements() != srcIt->GetNumElements().
            // (range->GetNumElements() should always be 1, but srcIt
            //  (vtBufferSource) could have a VtArray with arraySize entries).

            TF_FOR_ALL(srcIt, reqIt->sources) {
                // execute copy
                reqIt->range->CopyData(*srcIt);

                // also copy daisy chains
                if ((*srcIt)->HasChainedBuffer()) {
                    HdBufferSourceSharedPtr src = (*srcIt)->GetChainedBuffer();
                    while(src) {
                        reqIt->range->CopyData(src);
                        src = src->GetChainedBuffer();
                    }
                }
            }

            if (TfDebug::IsEnabled(HD_BUFFER_ARRAY_RANGE_CLEANED)) {
                std::stringstream ss;
                ss << *reqIt->range;
                TF_DEBUG(HD_BUFFER_ARRAY_RANGE_CLEANED).Msg("CLEAN: %s\n", 
                                                            ss.str().c_str());
            }
        }
    }

    {
        // HD_TRACE_SCOPE("Flush");
        // 5. flush phase:
        //
        // flush cosolidated buffer updates
    }

    {
        HD_TRACE_SCOPE("GpuComputation Execute");
        // 6. execute GPU computations
        //
        // note: GPU computations have to be executed in the order that
        // they are registered.
        //   e.g. smooth normals -> quadrangulation.
        //
        TF_FOR_ALL(it, _pendingComputations) {
            it->computation->Execute(it->range);

            HD_PERF_COUNTER_INCR(HdPerfTokens->computationsCommited);
        }
    }

    // release sources
    WorkParallelForEach(_pendingSources.begin(), _pendingSources.end(),
                        [](_PendingSource &ps) {
                            ps.range.reset();
                            ps.sources.clear();
                        });

    _pendingSources.clear();
    _numBufferSourcesToResolve = 0;
    _pendingComputations.clear();
}

void
HdResourceRegistry::GarbageCollect()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    HD_PERF_COUNTER_INCR(HdPerfTokens->garbageCollected);

    // cleanup instance registries
    size_t numMeshTopology = _meshTopologyRegistry.GarbageCollect();
    size_t numBasisCurvesTopology = _basisCurvesTopologyRegistry.GarbageCollect();
    size_t numVertexAdjacency = _vertexAdjacencyRegistry.GarbageCollect();

    // reset instance perf counters
    HD_PERF_COUNTER_SET(HdPerfTokens->instMeshTopology, numMeshTopology);
    HD_PERF_COUNTER_SET(HdPerfTokens->instBasisCurvesTopology, numBasisCurvesTopology);
    HD_PERF_COUNTER_SET(HdPerfTokens->instVertexAdjacency, numVertexAdjacency);

    // index range registry has to be cleaned BEFORE buffer array,
    // since it retains shared_ptr of buffer array range which expects
    // to be expired at buffer array's garbage collection.
    size_t numIndexRange = 0;
    TF_FOR_ALL (it, _meshTopologyIndexRangeRegistry) {
        numIndexRange += it->second.GarbageCollect();
    }
    // reset index range perf counters
    HD_PERF_COUNTER_SET(HdPerfTokens->instMeshTopologyRange, numIndexRange);

    numIndexRange = 0;
    TF_FOR_ALL (it, _basisCurvesTopologyIndexRangeRegistry) {
        numIndexRange += it->second.GarbageCollect();
    }
    // reset index range perf counters
    HD_PERF_COUNTER_SET(HdPerfTokens->instBasisCurvesTopologyRange, numIndexRange);

    // cleanup buffer array
    // buffer array retains weak_ptrs of range. All unused ranges should be
    // deleted (expired) at this point.
    _nonUniformBufferArrayRegistry.GarbageCollect();
    _uniformUboBufferArrayRegistry.GarbageCollect();
    _uniformSsboBufferArrayRegistry.GarbageCollect();
    _singleBufferArrayRegistry.GarbageCollect();

    // Cleanup Shader registries
    _geometricShaderRegistry.GarbageCollect();
    _glslProgramRegistry.GarbageCollect();

    // Cleanup texture registries
    _textureResourceRegistry.GarbageCollect();

    GarbageCollectDispatchBuffers();
    GarbageCollectPersistentBuffers();
}

VtDictionary
HdResourceRegistry::GetResourceAllocation() const
{
    VtDictionary result;

    size_t gpuMemoryUsed = 0;

    // buffer array allocation

    size_t nonUniformSize   = _nonUniformBufferArrayRegistry.GetResourceAllocation(result);
    size_t uboSize          = _uniformUboBufferArrayRegistry.GetResourceAllocation(result);
    size_t ssboSize         = _uniformSsboBufferArrayRegistry.GetResourceAllocation(result);
    size_t singleBufferSize = _singleBufferArrayRegistry.GetResourceAllocation(result);

    result[HdPerfTokens->nonUniformSize]   = VtValue(nonUniformSize);
    result[HdPerfTokens->uboSize]          = VtValue(uboSize);
    result[HdPerfTokens->ssboSize]         = VtValue(ssboSize);
    result[HdPerfTokens->singleBufferSize] = VtValue(singleBufferSize);
    gpuMemoryUsed += nonUniformSize +
                     uboSize        +
                     ssboSize       +
                     singleBufferSize;

    // glsl program & ubo allocation
    TF_FOR_ALL (progIt, _glslProgramRegistry) {
        HdGLSLProgramSharedPtr const &program = progIt->second;
        if (!program) continue;
        size_t size =
            program->GetProgram().GetSize() +
            program->GetGlobalUniformBuffer().GetSize();

        // the role of program and global uniform buffer is always same.
        std::string const &role = program->GetProgram().GetRole().GetString();
        if (result.count(role)) {
            size_t currentSize = result[role].Get<size_t>();
            result[role] = VtValue(currentSize + size);
        } else {
            result[role] = VtValue(size);
        }

        gpuMemoryUsed += size;
    }

    // dispatch buffers
    TF_FOR_ALL (bufferIt, _dispatchBufferRegistry) {
        HdDispatchBufferSharedPtr buffer = (*bufferIt);
        if (!TF_VERIFY(buffer)) {
            continue;
        }

        std::string const & role = buffer->GetRole().GetString();
        size_t size = size_t(buffer->GetEntireResource()->GetSize());

        if (result.count(role)) {
            size_t currentSize = result[role].Get<size_t>();
            result[role] = VtValue(currentSize + size);
        } else {
            result[role] = VtValue(size);
        }

        gpuMemoryUsed += size;
    }

    // persistent buffers
    TF_FOR_ALL (bufferIt, _persistentBufferRegistry) {
        HdPersistentBufferSharedPtr buffer = (*bufferIt);
        if (!TF_VERIFY(buffer)) {
            continue;
        }

        std::string const & role = buffer->GetRole().GetString();
        size_t size = size_t(buffer->GetSize());

        if (result.count(role)) {
            size_t currentSize = result[role].Get<size_t>();
            result[role] = VtValue(currentSize + size);
        } else {
            result[role] = VtValue(size);
        }

        gpuMemoryUsed += size;
    }

    // textures
    size_t hydraTexturesMemory = 0;

    TF_FOR_ALL (textureResourceIt, _textureResourceRegistry) {
        HdTextureResourceSharedPtr textureResource = (textureResourceIt->second);
        if (not TF_VERIFY(textureResource)) {
            continue;
        }

        hydraTexturesMemory += textureResource->GetMemoryUsed();
    }
    result[HdPerfTokens->textureResourceMemory] = VtValue(hydraTexturesMemory);
    gpuMemoryUsed += hydraTexturesMemory;

    GlfTextureRegistry &textureReg = GlfTextureRegistry::GetInstance();
    std::vector<VtDictionary> textureInfo = textureReg.GetTextureInfos();
    size_t textureMemory = 0;
    TF_FOR_ALL (textureIt, textureInfo) {
        VtDictionary &info = (*textureIt);
        textureMemory += info["memoryUsed"].Get<size_t>();
    }
    result[HdPerfTokens->textureMemory] = VtValue(textureMemory);


    result[HdPerfTokens->gpuMemoryUsed.GetString()] = gpuMemoryUsed;

    HD_PERF_COUNTER_SET(HdPerfTokens->gpuMemoryUsed, gpuMemoryUsed);

    return result;
}

static bool _IsEnabledTopologyInstancing()
{
    // disable instancing
    static bool isTopologyInstancingEnabled =
        TfGetenvBool("HD_ENABLE_TOPOLOGY_INSTANCING", true);
    return isTopologyInstancingEnabled;
}

template <typename ID, typename T> inline
std::unique_lock<std::mutex>
_Register(ID id, HdInstanceRegistry<HdInstance<ID, T> > &registry,
          TfToken const &perfToken, HdInstance<ID, T> *instance)
{
    if (_IsEnabledTopologyInstancing()) {
        std::unique_lock<std::mutex> lock = registry.GetInstance(id, instance);

        if (instance->IsFirstInstance()) {
            HD_PERF_COUNTER_INCR(perfToken);
        }

        return lock;
    } else {
        instance->Create(id, T(), nullptr, true);
        return std::unique_lock<std::mutex>();  // Nothing actually locked
    }
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterBasisCurvesTopology(HdTopology::ID id,
                        HdInstance<HdTopology::ID, HdBasisCurvesTopologySharedPtr> *instance)
{
    return _Register(id, _basisCurvesTopologyRegistry,
                     HdPerfTokens->instBasisCurvesTopology, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterMeshTopology(HdTopology::ID id,
                        HdInstance<HdTopology::ID, HdMeshTopologySharedPtr> *instance)
{
    return _Register(id, _meshTopologyRegistry,
                     HdPerfTokens->instMeshTopology, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterVertexAdjacency(HdTopology::ID id,
                        HdInstance<HdTopology::ID, Hd_VertexAdjacencySharedPtr>  *instance)
{
    return _Register(id, _vertexAdjacencyRegistry,
                     HdPerfTokens->instVertexAdjacency, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterMeshIndexRange(HdTopology::ID id,
                        TfToken const &name,
                        HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr> *instance)
{
    return _Register(id, _meshTopologyIndexRangeRegistry[name],
                     HdPerfTokens->instMeshTopologyRange, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterBasisCurvesIndexRange(HdTopology::ID id,
                        TfToken const &name,
                        HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr> *instance)
{
    return _Register(id, _basisCurvesTopologyIndexRangeRegistry[name],
                     HdPerfTokens->instBasisCurvesTopologyRange, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterGeometricShader(HdShaderKey::ID id,
                        HdInstance<HdShaderKey::ID, Hd_GeometricShaderSharedPtr> *instance)
{
    return _geometricShaderRegistry.GetInstance(id, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterGLSLProgram(HdGLSLProgram::ID id,
                        HdInstance<HdGLSLProgram::ID, HdGLSLProgramSharedPtr> *instance)
{
    return _glslProgramRegistry.GetInstance(id, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::RegisterTextureResource(HdTextureResource::ID id,
                        HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr> *instance)
{
    return _textureResourceRegistry.GetInstance(id, instance);
}

std::unique_lock<std::mutex>
HdResourceRegistry::FindTextureResource(HdTextureResource::ID id,
                        HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr> *instance, 
                        bool *found)
{
    return _textureResourceRegistry.FindInstance(id, instance, found);
}


void HdResourceRegistry::InvalidateGeometricShaderRegistry()
{
    _geometricShaderRegistry.Invalidate();
}

std::ostream &operator <<(std::ostream &out,
                          const HdResourceRegistry& self)
{
    out << "HdResourceRegistry " << &self << " :\n";

    out << self._nonUniformBufferArrayRegistry;
    out << self._uniformUboBufferArrayRegistry;
    out << self._uniformSsboBufferArrayRegistry;
    out << self._singleBufferArrayRegistry;

    return out;
}

HdDispatchBufferSharedPtr
HdResourceRegistry::RegisterDispatchBuffer(
    TfToken const &role, int count, int commandNumUints)
{
    HdDispatchBufferSharedPtr result(
        new HdDispatchBuffer(role, count, commandNumUints));

    _dispatchBufferRegistry.push_back(result);

    return result;
}

void
HdResourceRegistry::GarbageCollectDispatchBuffers()
{
    HD_TRACE_FUNCTION();

    _dispatchBufferRegistry.erase(
        std::remove_if(
            _dispatchBufferRegistry.begin(), _dispatchBufferRegistry.end(),
            boost::bind(&HdDispatchBufferSharedPtr::unique, _1)),
        _dispatchBufferRegistry.end());
}


HdPersistentBufferSharedPtr
HdResourceRegistry::RegisterPersistentBuffer(
        TfToken const &role, size_t dataSize, void *data)
{
    HdPersistentBufferSharedPtr result(
            new HdPersistentBuffer(role, dataSize, data));

    _persistentBufferRegistry.push_back(result);

    return result;
}

void
HdResourceRegistry::GarbageCollectPersistentBuffers()
{
    HD_TRACE_FUNCTION();

    _persistentBufferRegistry.erase(
        std::remove_if(
            _persistentBufferRegistry.begin(), _persistentBufferRegistry.end(),
            boost::bind(&HdPersistentBufferSharedPtr::unique, _1)),
        _persistentBufferRegistry.end());
}
