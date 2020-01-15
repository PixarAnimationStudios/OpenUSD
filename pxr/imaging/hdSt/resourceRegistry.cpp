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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/textureRegistry.h"

#include "pxr/base/work/loops.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/copyComputation.h"
#include "pxr/imaging/hdSt/dispatchBuffer.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/interleavedMemoryManager.h"
#include "pxr/imaging/hdSt/persistentBuffer.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/vboMemoryManager.h"
#include "pxr/imaging/hdSt/vboSimpleMemoryManager.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HDST_ENABLE_RESOURCE_INSTANCING, true,
                  "Enable instance registry deduplication of resource data");

static void
_CopyChainedBuffers(HdBufferSourceSharedPtr const&  src,
                    HdBufferArrayRangeSharedPtr const& range)
{
    if (src->HasChainedBuffer()) {
        HdBufferSourceVector chainedSrcs = src->GetChainedBuffers();
        // traverse the tree in a DFS fashion
        for(auto& c : chainedSrcs) {
            range->CopyData(c);
            _CopyChainedBuffers(c, range);
        }
    }
}

static bool
_IsEnabledResourceInstancing()
{
    static bool isResourceInstancingEnabled =
        TfGetEnvSetting(HDST_ENABLE_RESOURCE_INSTANCING);
    return isResourceInstancingEnabled;
}

template <typename ID, typename T>
HdInstance<T>
_Register(ID id, HdInstanceRegistry<T> &registry, TfToken const &perfToken)
{
    if (_IsEnabledResourceInstancing()) {
        HdInstance<T> instance = registry.GetInstance(id);

        if (instance.IsFirstInstance()) {
            HD_PERF_COUNTER_INCR(perfToken);
        }

        return instance;
    } else {
        // Return an instance that is not managed by the registry when
        // topology instancing is disabled.
        return HdInstance<T>(id);
    }
}

HdStResourceRegistry::HdStResourceRegistry()
    : _numBufferSourcesToResolve(0)
    , _nonUniformAggregationStrategy()
    , _nonUniformImmutableAggregationStrategy()
    , _uniformUboAggregationStrategy()
    , _uniformSsboAggregationStrategy()
    , _singleAggregationStrategy()
{
    // default aggregation strategies for varying (vertex, varying) primvars
    SetNonUniformAggregationStrategy(
        new HdStVBOMemoryManager());
    SetNonUniformImmutableAggregationStrategy(
        new HdStVBOMemoryManager());

    // default aggregation strategy for uniform on SSBO (for primvars)
    SetShaderStorageAggregationStrategy(
        new HdStInterleavedSSBOMemoryManager());

    // default aggregation strategy for uniform on UBO (for globals)
    SetUniformAggregationStrategy(
        new HdStInterleavedUBOMemoryManager());

    // default aggregation strategy for single buffers (for nested instancer)
    SetSingleStorageAggregationStrategy(
        new HdStVBOSimpleMemoryManager());
}

HdStResourceRegistry::~HdStResourceRegistry()
{
}

void HdStResourceRegistry::InvalidateShaderRegistry()
{
    _geometricShaderRegistry.Invalidate();
}

VtDictionary
HdStResourceRegistry::GetResourceAllocation() const
{
    VtDictionary result;

    size_t gpuMemoryUsed = 0;

    // buffer array allocation

    size_t nonUniformSize   = 
        _nonUniformBufferArrayRegistry.GetResourceAllocation(
            _nonUniformAggregationStrategy.get(), result) +
        _nonUniformImmutableBufferArrayRegistry.GetResourceAllocation(
            _nonUniformImmutableAggregationStrategy.get(), result);
    size_t uboSize          = 
        _uniformUboBufferArrayRegistry.GetResourceAllocation(
            _uniformUboAggregationStrategy.get(), result);
    size_t ssboSize         = 
        _uniformSsboBufferArrayRegistry.GetResourceAllocation(
            _uniformSsboAggregationStrategy.get(), result);
    size_t singleBufferSize = 
        _singleBufferArrayRegistry.GetResourceAllocation(
            _singleAggregationStrategy.get(), result);

    result[HdPerfTokens->nonUniformSize]   = VtValue(nonUniformSize);
    result[HdPerfTokens->uboSize]          = VtValue(uboSize);
    result[HdPerfTokens->ssboSize]         = VtValue(ssboSize);
    result[HdPerfTokens->singleBufferSize] = VtValue(singleBufferSize);
    gpuMemoryUsed += nonUniformSize +
                     uboSize        +
                     ssboSize       +
                     singleBufferSize;

    result[HdPerfTokens->gpuMemoryUsed.GetString()] = gpuMemoryUsed;

    // Prompt derived registries to tally their resources.
    _TallyResourceAllocation(&result);

    gpuMemoryUsed =
        VtDictionaryGet<size_t>(result, HdPerfTokens->gpuMemoryUsed.GetString(),
                                VtDefault = 0);

    HD_PERF_COUNTER_SET(HdPerfTokens->gpuMemoryUsed, gpuMemoryUsed);

    return result;
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::AllocateNonUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    return _nonUniformBufferArrayRegistry.AllocateRange(
                                    _nonUniformAggregationStrategy.get(),
                                    role,
                                    bufferSpecs,
                                    usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::AllocateNonUniformImmutableBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    usageHint.bits.immutable = 1;

    return _nonUniformImmutableBufferArrayRegistry.AllocateRange(
                                _nonUniformImmutableAggregationStrategy.get(),
                                role,
                                bufferSpecs,
                                usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::AllocateUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    return _uniformUboBufferArrayRegistry.AllocateRange(
                                    _uniformUboAggregationStrategy.get(),
                                    role,
                                    bufferSpecs,
                                    usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::AllocateShaderStorageBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    return _uniformSsboBufferArrayRegistry.AllocateRange(
                                    _uniformSsboAggregationStrategy.get(),
                                    role,
                                    bufferSpecs,
                                    usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::AllocateSingleBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    return _singleBufferArrayRegistry.AllocateRange(
                                    _singleAggregationStrategy.get(),
                                    role,
                                    bufferSpecs,
                                    usageHint);
}

void
HdStResourceRegistry::AddSources(HdBufferArrayRangeSharedPtr const &range,
                               HdBufferSourceVector &sources)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

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
            if (ARCH_UNLIKELY(sources[srcNum]->HasPreChainedBuffer())) {
                AddSource(sources[srcNum]->GetPreChainedBuffer());
            }
            ++srcNum;
        } else {
            TF_RUNTIME_ERROR("Source Buffer for %s is invalid",
                             sources[srcNum]->GetName().GetText());
            
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
HdStResourceRegistry::AddSource(HdBufferArrayRangeSharedPtr const &range,
                              HdBufferSourceSharedPtr const &source)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

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
        TF_RUNTIME_ERROR("source buffer for %s is invalid",
                          source->GetName().GetText());
        return;
    }

    if (ARCH_UNLIKELY(source->HasPreChainedBuffer())) {
        AddSource(source->GetPreChainedBuffer());
    }

    _pendingSources.emplace_back(range, source);
    ++_numBufferSourcesToResolve;  // Atomic
}

void
HdStResourceRegistry::AddSource(HdBufferSourceSharedPtr const &source)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (ARCH_UNLIKELY(!source))
    {
        TF_RUNTIME_ERROR("source pointer is null");
        return;
    }

    // Buffer has to be valid
    if (ARCH_UNLIKELY(!source->IsValid()))
    {
        TF_RUNTIME_ERROR("source buffer for %s is invalid",
                         source->GetName().GetText());
        return;
    }

    if (ARCH_UNLIKELY(source->HasPreChainedBuffer())) {
        AddSource(source->GetPreChainedBuffer());
    }

    _pendingSources.emplace_back(HdBufferArrayRangeSharedPtr(), source);
    ++_numBufferSourcesToResolve; // Atomic
}

void
HdStResourceRegistry::AddComputation(HdBufferArrayRangeSharedPtr const &range,
                                   HdComputationSharedPtr const &computation)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // if the computation is buffer source computation, it will be appended
    // into pendingBufferSourceComputations, which is executed right after
    // the first buffer source transfers. Those computations produce
    // buffer sources as results of computation, so the registry also invokes
    // another transfers for such buffers. The computation isn't marked
    // as a buffer source computation will be executed at the end.

    _pendingComputations.emplace_back(range, computation);
}

HdStDispatchBufferSharedPtr
HdStResourceRegistry::RegisterDispatchBuffer(
    TfToken const &role, int count, int commandNumUints)
{
    HdStDispatchBufferSharedPtr result(
        new HdStDispatchBuffer(role, count, commandNumUints));

    _dispatchBufferRegistry.push_back(result);

    return result;
}

HdStPersistentBufferSharedPtr
HdStResourceRegistry::RegisterPersistentBuffer(
        TfToken const &role, size_t dataSize, void *data)
{
    HdStPersistentBufferSharedPtr result(
            new HdStPersistentBuffer(role, dataSize, data));

    _persistentBufferRegistry.push_back(result);

    return result;
}

void
HdStResourceRegistry::GarbageCollectDispatchBuffers()
{
    HD_TRACE_FUNCTION();

    _dispatchBufferRegistry.erase(
        std::remove_if(
            _dispatchBufferRegistry.begin(), _dispatchBufferRegistry.end(),
            std::bind(&HdStDispatchBufferSharedPtr::unique,
                      std::placeholders::_1)),
        _dispatchBufferRegistry.end());
}

void
HdStResourceRegistry::GarbageCollectPersistentBuffers()
{
    HD_TRACE_FUNCTION();

    _persistentBufferRegistry.erase(
        std::remove_if(
            _persistentBufferRegistry.begin(), _persistentBufferRegistry.end(),
            std::bind(&HdStPersistentBufferSharedPtr::unique,
                      std::placeholders::_1)),
        _persistentBufferRegistry.end());
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::MergeBufferArrayRange(
    HdAggregationStrategy *strategy,
    HdBufferArrayRegistry &bufferArrayRegistry,
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayUsageHint newUsageHint,
    HdBufferArrayRangeSharedPtr const &range)
{
    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(range)) return HdBufferArrayRangeSharedPtr();

    // get existing buffer specs
    HdBufferSpecVector oldBufferSpecs;
    range->GetBufferSpecs(&oldBufferSpecs);

    HdBufferArrayUsageHint oldUsageHint = range->GetUsageHint();

    // immutable ranges should always be migrated, otherwise compare bufferspec
    if (range->IsImmutable() ||
        !HdBufferSpec::IsSubset(newBufferSpecs, oldBufferSpecs) ||
        newUsageHint.value != oldUsageHint.value) {
        // create / moveto the new buffer array.

        HdComputationVector computations;

        // existing content has to be transferred.
        TF_FOR_ALL(it, oldBufferSpecs) {
            if (std::find(newBufferSpecs.begin(), newBufferSpecs.end(), *it)
                == newBufferSpecs.end()) {

                // migration computation
                computations.push_back(
                    HdComputationSharedPtr(new HdStCopyComputationGPU(
                                               /*src=*/range, it->name)));
            }
        }
        // new buffer array should have a union of
        // new buffer specs and exsiting buffer specs.
        HdBufferSpecVector bufferSpecs = HdBufferSpec::ComputeUnion(
            newBufferSpecs, oldBufferSpecs);

        // allocate new range.
        HdBufferArrayRangeSharedPtr result = bufferArrayRegistry.AllocateRange(
            strategy, role, bufferSpecs, newUsageHint);

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
HdStResourceRegistry::MergeNonUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayUsageHint newUsageHint,
    HdBufferArrayRangeSharedPtr const &range)
{
    return MergeBufferArrayRange(_nonUniformAggregationStrategy.get(),
                                 _nonUniformBufferArrayRegistry,
                                 role, newBufferSpecs, newUsageHint, range);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::MergeNonUniformImmutableBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayUsageHint newUsageHint,
    HdBufferArrayRangeSharedPtr const &range)
{
    newUsageHint.bits.immutable = 1;

    return MergeBufferArrayRange(_nonUniformImmutableAggregationStrategy.get(),
                                 _nonUniformImmutableBufferArrayRegistry,
                                 role, newBufferSpecs, newUsageHint, range);
}
HdBufferArrayRangeSharedPtr
HdStResourceRegistry::MergeUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayUsageHint newUsageHint,
    HdBufferArrayRangeSharedPtr const &range)
{
    return MergeBufferArrayRange(_uniformUboAggregationStrategy.get(),
                                 _uniformUboBufferArrayRegistry,
                                 role, newBufferSpecs, newUsageHint, range);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::MergeShaderStorageBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &newBufferSpecs,
    HdBufferArrayUsageHint newUsageHint,
    HdBufferArrayRangeSharedPtr const &range)
{
    return MergeBufferArrayRange(_uniformSsboAggregationStrategy.get(),
                                 _uniformSsboBufferArrayRegistry,
                                 role, newBufferSpecs, newUsageHint, range);
}

HdInstance<HdSt_MeshTopologySharedPtr>
HdStResourceRegistry::RegisterMeshTopology(
        HdInstance<HdSt_MeshTopologySharedPtr>::ID id)
{
    return _Register(id, _meshTopologyRegistry,
                     HdPerfTokens->instMeshTopology);
}

HdInstance<HdSt_BasisCurvesTopologySharedPtr>
HdStResourceRegistry::RegisterBasisCurvesTopology(
        HdInstance<HdSt_BasisCurvesTopologySharedPtr>::ID id)
{
    return _Register(id, _basisCurvesTopologyRegistry,
                     HdPerfTokens->instBasisCurvesTopology);
}

HdInstance<Hd_VertexAdjacencySharedPtr>
HdStResourceRegistry::RegisterVertexAdjacency(
        HdInstance<Hd_VertexAdjacencySharedPtr>::ID id)
{
    return _Register(id, _vertexAdjacencyRegistry,
                     HdPerfTokens->instVertexAdjacency);
}

HdInstance<HdBufferArrayRangeSharedPtr>
HdStResourceRegistry::RegisterMeshIndexRange(
        HdInstance<HdBufferArrayRangeSharedPtr>::ID id, TfToken const &name)
{
    return _Register(id, _meshTopologyIndexRangeRegistry[name],
                     HdPerfTokens->instMeshTopologyRange);
}

HdInstance<HdBufferArrayRangeSharedPtr>
HdStResourceRegistry::RegisterBasisCurvesIndexRange(
        HdInstance<HdBufferArrayRangeSharedPtr>::ID id, TfToken const &name)
{
    return _Register(id, _basisCurvesTopologyIndexRangeRegistry[name],
                     HdPerfTokens->instBasisCurvesTopologyRange);
}

HdInstance<HdBufferArrayRangeSharedPtr>
HdStResourceRegistry::RegisterPrimvarRange(
        HdInstance<HdBufferArrayRangeSharedPtr>::ID id)
{
    return _Register(id, _primvarRangeRegistry,
                     HdPerfTokens->instPrimvarRange);
}

HdInstance<HdBufferArrayRangeSharedPtr>
HdStResourceRegistry::RegisterExtComputationDataRange(
        HdInstance<HdBufferArrayRangeSharedPtr>::ID id)
{
    return _Register(id, _extComputationDataRangeRegistry,
                     HdPerfTokens->instExtComputationDataRange);
}

HdInstance<HdStTextureResourceSharedPtr>
HdStResourceRegistry::RegisterTextureResource(TextureKey id)
{
    return _textureResourceRegistry.GetInstance(id);
}

HdInstance<HdStTextureResourceSharedPtr>
HdStResourceRegistry::FindTextureResource(TextureKey id, bool *found)
{
    return _textureResourceRegistry.FindInstance(id, found);
}


HdInstance<HdSt_GeometricShaderSharedPtr>
HdStResourceRegistry::RegisterGeometricShader(
        HdInstance<HdSt_GeometricShaderSharedPtr>::ID id)
{
    return _geometricShaderRegistry.GetInstance(id);
}

HdInstance<HdStGLSLProgramSharedPtr>
HdStResourceRegistry::RegisterGLSLProgram(
        HdInstance<HdStGLSLProgramSharedPtr>::ID id)
{
    return _glslProgramRegistry.GetInstance(id);
}

HdInstance<HdStTextureResourceHandleSharedPtr>
HdStResourceRegistry::RegisterTextureResourceHandle(
        HdInstance<HdStTextureResourceHandleSharedPtr>::ID id)
{
    return _textureResourceHandleRegistry.GetInstance(id);
}

HdInstance<HdStTextureResourceHandleSharedPtr>
HdStResourceRegistry::FindTextureResourceHandle(
        HdInstance<HdStTextureResourceHandleSharedPtr>::ID id, bool *found)
{
    return _textureResourceHandleRegistry.FindInstance(id, found);
}

std::ostream &operator <<(
    std::ostream &out,
    const HdStResourceRegistry& self)
{
    out << "HdStResourceRegistry " << &self << " :\n";

    out << self._nonUniformBufferArrayRegistry;
    out << self._nonUniformImmutableBufferArrayRegistry;
    out << self._uniformUboBufferArrayRegistry;
    out << self._uniformSsboBufferArrayRegistry;
    out << self._singleBufferArrayRegistry;

    return out;
}

void
HdStResourceRegistry::_Commit()
{
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
            // thread in all tested scenarios, disabling until we can
            // figure out what's going on here.
//#pragma omp parallel for
            for (int i = 0; i < numThreads; ++i) {
                // iterate over all pending sources
                for (_PendingSource const& req: _pendingSources) {
                    for (HdBufferSourceSharedPtr const& source: req.sources) {
                        // execute computation.
                        // call IsResolved first since Resolve is virtual and
                        // could be costly.
                        if (!source->IsResolved()) {
                            if (source->Resolve()) {
                                TF_VERIFY(source->IsResolved(), 
                                "Name = %s", source->GetName().GetText());

                                ++numBufferSourcesResolved;

                                // call resize if it's the first in sources.
                                if (req.range &&
                                    source == *req.sources.begin()) {
                                    req.range->Resize(
                                        source->GetNumElements());
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
            _nonUniformAggregationStrategy.get());
        _nonUniformImmutableBufferArrayRegistry.ReallocateAll(
            _nonUniformImmutableAggregationStrategy.get());
        _uniformUboBufferArrayRegistry.ReallocateAll(
            _uniformUboAggregationStrategy.get());
        _uniformSsboBufferArrayRegistry.ReallocateAll(
            _uniformSsboAggregationStrategy.get());
        _singleBufferArrayRegistry.ReallocateAll(
            _singleAggregationStrategy.get());
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

                // also copy any chained buffers
                _CopyChainedBuffers(*srcIt, reqIt->range);
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
            it->computation->Execute(it->range, this);

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
HdStResourceRegistry::_GarbageCollect()
{
    // The sequence in which we run garbage collection is significant.
    // We want to clean objects first which might be holding references
    // to other objects which will be subsequently cleaned up.

    GarbageCollectDispatchBuffers();
    GarbageCollectPersistentBuffers();

    {
        size_t count = _meshTopologyRegistry.GarbageCollect();
        HD_PERF_COUNTER_SET(HdPerfTokens->instMeshTopology, count);
    }

    {
        size_t count = _basisCurvesTopologyRegistry.GarbageCollect();
        HD_PERF_COUNTER_SET(HdPerfTokens->instBasisCurvesTopology, count);
    }

    {
        size_t count = _vertexAdjacencyRegistry.GarbageCollect();
        HD_PERF_COUNTER_SET(HdPerfTokens->instVertexAdjacency, count);
    }

    {
        size_t count = 0;
        for (auto & it: _meshTopologyIndexRangeRegistry) {
            count += it.second.GarbageCollect();
        }
        HD_PERF_COUNTER_SET(HdPerfTokens->instMeshTopologyRange, count);
    }

    {
        size_t count = 0;
        for (auto & it: _basisCurvesTopologyIndexRangeRegistry) {
            count += it.second.GarbageCollect();
        }
        HD_PERF_COUNTER_SET(HdPerfTokens->instBasisCurvesTopologyRange, count);
    }

    {
        size_t count = _primvarRangeRegistry.GarbageCollect();
        HD_PERF_COUNTER_SET(HdPerfTokens->instPrimvarRange, count);
    }

    {
        size_t count = _extComputationDataRangeRegistry.GarbageCollect();
        HD_PERF_COUNTER_SET(HdPerfTokens->instExtComputationDataRange, count);
    }

    // Cleanup Shader registries
    _geometricShaderRegistry.GarbageCollect();
    _glslProgramRegistry.GarbageCollect();
    _textureResourceHandleRegistry.GarbageCollect();

    // cleanup buffer array
    // buffer array retains weak_ptrs of range. All unused ranges should be
    // deleted (expired) at this point.
    _nonUniformBufferArrayRegistry.GarbageCollect();
    _nonUniformImmutableBufferArrayRegistry.GarbageCollect();
    _uniformUboBufferArrayRegistry.GarbageCollect();
    _uniformSsboBufferArrayRegistry.GarbageCollect();
    _singleBufferArrayRegistry.GarbageCollect();
}

void
HdStResourceRegistry::_GarbageCollectBprims()
{
    // Cleanup texture registries
    _textureResourceRegistry.GarbageCollect();
}

void
HdStResourceRegistry::_TallyResourceAllocation(VtDictionary *result) const
{
    size_t gpuMemoryUsed =
        VtDictionaryGet<size_t>(*result,
                                HdPerfTokens->gpuMemoryUsed.GetString(),
                                VtDefault = 0);

    // dispatch buffers
    for (auto const & buffer: _dispatchBufferRegistry) {
        if (!TF_VERIFY(buffer)) {
            continue;
        }

        std::string const & role = buffer->GetRole().GetString();
        size_t size = size_t(buffer->GetEntireResource()->GetSize());

        (*result)[role] = VtDictionaryGet<size_t>(*result, role,
                                                  VtDefault = 0) + size;

        gpuMemoryUsed += size;
    }

    // persistent buffers
    for (auto const & buffer: _persistentBufferRegistry) {
        if (!TF_VERIFY(buffer)) {
            continue;
        }

        std::string const & role = buffer->GetRole().GetString();
        size_t size = size_t(buffer->GetSize());

        (*result)[role] = VtDictionaryGet<size_t>(*result, role,
                                                  VtDefault = 0) + size;

        gpuMemoryUsed += size;
    }

    // glsl program & ubo allocation
    for (auto const & it: _glslProgramRegistry) {
        HdStGLSLProgramSharedPtr const & program = it.second.value;
        // In the event of a compile or link error, programs can be null
        if (!program) {
            continue;
        }
        size_t size =
            program->GetProgram().GetSize() +
            program->GetGlobalUniformBuffer().GetSize();

        // the role of program and global uniform buffer is always same.
        std::string const &role = program->GetProgram().GetRole().GetString();
        (*result)[role] = VtDictionaryGet<size_t>(*result, role,
                                                  VtDefault = 0) + size;

        gpuMemoryUsed += size;
    }

    // Texture Resources
    {
        size_t textureResourceMemory = 0;

        for (auto const & it: _textureResourceRegistry) {
            HdStTextureResourceSharedPtr const & texResource = it.second.value;
            // In the event of an asset error, texture resources can be null
            if (!texResource) {
                continue;
            }

            textureResourceMemory += texResource->GetMemoryUsed();
        }
        (*result)[HdPerfTokens->textureResourceMemory] = VtValue(
                                                textureResourceMemory);
        gpuMemoryUsed += textureResourceMemory;
    }

    // Texture registry
    {
        GlfTextureRegistry &textureReg = GlfTextureRegistry::GetInstance();
        std::vector<VtDictionary> textureInfo = textureReg.GetTextureInfos();
        size_t textureMemory = 0;
        TF_FOR_ALL (textureIt, textureInfo) {
            VtDictionary &info = (*textureIt);
            textureMemory += info["memoryUsed"].Get<size_t>();
        }
        (*result)[HdPerfTokens->textureMemory] = VtValue(textureMemory);
    }

    (*result)[HdPerfTokens->gpuMemoryUsed.GetString()] = gpuMemoryUsed;
}

PXR_NAMESPACE_CLOSE_SCOPE
