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
#include "pxr/base/work/loops.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/copyComputation.h"
#include "pxr/imaging/hdSt/dispatchBuffer.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/interleavedMemoryManager.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/vboMemoryManager.h"
#include "pxr/imaging/hdSt/vboSimpleMemoryManager.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/textureHandleRegistry.h"
#include "pxr/imaging/hdSt/textureObjectRegistry.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hgi/hgi.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDST_ENABLE_RESOURCE_INSTANCING, true,
                  "Enable instance registry deduplication of resource data");

TF_DEFINE_PRIVATE_TOKENS(
    _perfTokens,

    (numberOfTextureObjects)
    (numberOfTextureHandles)
);

static void
_CopyChainedBuffers(HdBufferSourceSharedPtr const&  src,
                    HdBufferArrayRangeSharedPtr const& range)
{
    if (src->HasChainedBuffer()) {
        HdBufferSourceSharedPtrVector chainedSrcs = src->GetChainedBuffers();
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

HdStResourceRegistry::HdStResourceRegistry(Hgi * const hgi)
    : _hgi(hgi)
    , _numBufferSourcesToResolve(0)
    // default aggregation strategies for varying (vertex, varying) primvars
    , _nonUniformAggregationStrategy(
        std::make_unique<HdStVBOMemoryManager>(this))
    , _nonUniformImmutableAggregationStrategy(
        std::make_unique<HdStVBOMemoryManager>(this))
    // default aggregation strategy for uniform on UBO (for globals)
    , _uniformUboAggregationStrategy(
        std::make_unique<HdStInterleavedUBOMemoryManager>(this))
    // default aggregation strategy for uniform on SSBO (for primvars)
    , _uniformSsboAggregationStrategy(
        std::make_unique<HdStInterleavedSSBOMemoryManager>(this))
    // default aggregation strategy for single buffers (for nested instancer)
    , _singleAggregationStrategy(
        std::make_unique<HdStVBOSimpleMemoryManager>(this))
    , _textureHandleRegistry(std::make_unique<HdSt_TextureHandleRegistry>(this))
{
}

HdStResourceRegistry::~HdStResourceRegistry()
{
    // XXX Ideally all the HdInstanceRegistry would get destroy here and
    // they cleanup all GPU resources. Since that mechanism isn't in place
    // yet, we call GarbageCollect to emulate this behavior.
    GarbageCollect();
}

void HdStResourceRegistry::InvalidateShaderRegistry()
{
    _geometricShaderRegistry.Invalidate();
    _glslfxFileRegistry.Invalidate();
}


void HdStResourceRegistry::ReloadResource(TfToken const& resourceType,
                                          std::string const& path) 
{
    // find the file and invalidate it 
    if (resourceType == HdResourceTypeTokens->shaderFile) {

        size_t pathHash = TfHash()(path);
        HdInstance<HioGlslfxSharedPtr> glslfxInstance = 
                                                RegisterGLSLFXFile(pathHash);

        // Reload the glslfx file.
        HioGlslfxSharedPtr glslfxSharedPtr = glslfxInstance.GetValue();
        glslfxSharedPtr.reset(new HioGlslfx(path));
        glslfxInstance.SetValue(glslfxSharedPtr);
    } else if (resourceType == HdResourceTypeTokens->texture) {
        HdSt_TextureObjectRegistry *const reg = 
            _textureHandleRegistry->GetTextureObjectRegistry();
        reg->MarkTextureFilePathDirty(TfToken(path));
    }
}

VtDictionary
HdStResourceRegistry::GetResourceAllocation() const
{
    VtDictionary result;

    size_t gpuMemoryUsed = 0;

    // buffer array allocation

    const size_t nonUniformSize   = 
        _nonUniformBufferArrayRegistry.GetResourceAllocation(
            _nonUniformAggregationStrategy.get(), result) +
        _nonUniformImmutableBufferArrayRegistry.GetResourceAllocation(
            _nonUniformImmutableAggregationStrategy.get(), result);
    const size_t uboSize          = 
        _uniformUboBufferArrayRegistry.GetResourceAllocation(
            _uniformUboAggregationStrategy.get(), result);
    const size_t ssboSize         = 
        _uniformSsboBufferArrayRegistry.GetResourceAllocation(
            _uniformSsboAggregationStrategy.get(), result);
    const size_t singleBufferSize = 
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

Hgi*
HdStResourceRegistry::GetHgi()
{
    return _hgi;
}

/// ------------------------------------------------------------------------
/// BAR allocation API
/// ------------------------------------------------------------------------
HdBufferArrayRangeSharedPtr
HdStResourceRegistry::AllocateNonUniformBufferArrayRange(
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    return _AllocateBufferArrayRange(
                _nonUniformAggregationStrategy.get(),
                _nonUniformBufferArrayRegistry,
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

    return _AllocateBufferArrayRange(
                _nonUniformImmutableAggregationStrategy.get(),
                _nonUniformImmutableBufferArrayRegistry,
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
    return _AllocateBufferArrayRange(
                _uniformUboAggregationStrategy.get(),
                _uniformUboBufferArrayRegistry,
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
    return _AllocateBufferArrayRange(
                _uniformSsboAggregationStrategy.get(),
                _uniformSsboBufferArrayRegistry,
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
    return _AllocateBufferArrayRange(
                _singleAggregationStrategy.get(),
                _singleBufferArrayRegistry,
                role,
                bufferSpecs,
                usageHint);
}

/// ------------------------------------------------------------------------
/// BAR allocation/migration/update API
/// ------------------------------------------------------------------------
HdBufferArrayRangeSharedPtr
HdStResourceRegistry::UpdateNonUniformBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint)
{
    return _UpdateBufferArrayRange(
        _nonUniformAggregationStrategy.get(),
        _nonUniformBufferArrayRegistry,
        role,
        curRange,
        updatedOrAddedSpecs, removedSpecs, usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::UpdateNonUniformImmutableBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint)
{
    usageHint.bits.immutable = 1;

    return _UpdateBufferArrayRange(
        _nonUniformImmutableAggregationStrategy.get(),
        _nonUniformImmutableBufferArrayRegistry,
        role,
        curRange,
        updatedOrAddedSpecs, removedSpecs, usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::UpdateUniformBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint)
{
    return _UpdateBufferArrayRange(
        _uniformUboAggregationStrategy.get(),
        _uniformUboBufferArrayRegistry,
        role,
        curRange,
        updatedOrAddedSpecs, removedSpecs, usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::UpdateShaderStorageBufferArrayRange(
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint)
{
    return _UpdateBufferArrayRange(
        _uniformSsboAggregationStrategy.get(),
        _uniformSsboBufferArrayRegistry,
        role,
        curRange,
        updatedOrAddedSpecs, removedSpecs, usageHint);
}

/// ------------------------------------------------------------------------
/// Resource update & computation queuing API
/// ------------------------------------------------------------------------
void
HdStResourceRegistry::AddSources(HdBufferArrayRangeSharedPtr const &range,
                                 HdBufferSourceSharedPtrVector &&sources)
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
        _numBufferSourcesToResolve += sources.size();
        _pendingSources.emplace_back(
            range, std::move(sources));

        TF_VERIFY(range.use_count() >=2);
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
                                   HdComputationSharedPtr const &computation,
                                   HdStComputeQueue const queue)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (TF_VERIFY(queue < HdStComputeQueueCount)) {
        _pendingComputations[queue].emplace_back(range, computation);
    }
}

/// ------------------------------------------------------------------------
/// Dispatch & misc buffer API
/// ------------------------------------------------------------------------
HdStDispatchBufferSharedPtr
HdStResourceRegistry::RegisterDispatchBuffer(
    TfToken const &role, int count, int commandNumUints)
{
    HdStDispatchBufferSharedPtr const result =
        std::make_shared<HdStDispatchBuffer>(
            this, role, count, commandNumUints);

    _dispatchBufferRegistry.push_back(result);

    return result;
}

HdStBufferResourceSharedPtr
HdStResourceRegistry::RegisterBufferResource(
    TfToken const &role, 
    HdTupleType tupleType)
{
    HdStBufferResourceSharedPtr const result =
        std::make_shared<HdStBufferResource>(
            role, tupleType, /*offset*/ 0, /*stride*/ 0);

    size_t byteSize = HdDataSizeOfTupleType(tupleType);

    HgiBufferDesc bufDesc;
    bufDesc.usage= HgiBufferUsageUniform;
    bufDesc.byteSize= byteSize;
    HgiBufferHandle newId = _hgi->CreateBuffer(bufDesc);

    result->SetAllocation(newId, byteSize);

    _bufferResourceRegistry.push_back(result);

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
HdStResourceRegistry::GarbageCollectBufferResources()
{
    HD_TRACE_FUNCTION();

    _bufferResourceRegistry.erase(
        std::remove_if(
            _bufferResourceRegistry.begin(), _bufferResourceRegistry.end(),
            std::bind(&HdStBufferResourceSharedPtr::unique,
                      std::placeholders::_1)),
        _bufferResourceRegistry.end());
}

/// ------------------------------------------------------------------------
/// Instance Registries
/// ------------------------------------------------------------------------
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

HdInstance<HioGlslfxSharedPtr>
HdStResourceRegistry::RegisterGLSLFXFile(
        HdInstance<HioGlslfxSharedPtr>::ID id)
{
    return _glslfxFileRegistry.GetInstance(id);
}

HdInstance<HgiResourceBindingsSharedPtr>
HdStResourceRegistry::RegisterResourceBindings(
    HdInstance<HgiResourceBindingsSharedPtr>::ID id)
{
    return _resourceBindingsRegistry.GetInstance(id);
}

HdInstance<HgiGraphicsPipelineSharedPtr>
HdStResourceRegistry::RegisterGraphicsPipeline(
    HdInstance<HgiGraphicsPipelineSharedPtr>::ID id)
{
    return _graphicsPipelineRegistry.GetInstance(id);
}

HdInstance<HgiComputePipelineSharedPtr>
HdStResourceRegistry::RegisterComputePipeline(
    HdInstance<HgiComputePipelineSharedPtr>::ID id)
{
    return _computePipelineRegistry.GetInstance(id);
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

HgiBlitCmds*
HdStResourceRegistry::GetGlobalBlitCmds()
{
    if (!_blitCmds) {
        _blitCmds = _hgi->CreateBlitCmds();
    }
    return _blitCmds.get();
}

HgiComputeCmds*
HdStResourceRegistry::GetGlobalComputeCmds()
{
    if (!_computeCmds) {
        _computeCmds = _hgi->CreateComputeCmds();
    }
    return _computeCmds.get();
}

void
HdStResourceRegistry::SubmitBlitWork(HgiSubmitWaitType wait)
{
    if (_blitCmds) {
        _hgi->SubmitCmds(_blitCmds.get(), wait);
        _blitCmds.reset();
    }
}

void
HdStResourceRegistry::SubmitComputeWork(HgiSubmitWaitType wait)
{
    if (_computeCmds) {
        _hgi->SubmitCmds(_computeCmds.get(), wait);
        _computeCmds.reset();
    }
}

void
HdStResourceRegistry::_CommitTextures()
{
    HdStShaderCode::ResourceContext ctx(this);

    const std::set<HdStShaderCodeSharedPtr> shaderCodes =
        _textureHandleRegistry->Commit();

    // Give assoicated HdStShaderCode objects a chance to add buffer
    // sources that rely on texture sampler handles (bindless) or
    // texture metadata (e.g., sampling transform for volume fields).
    for (HdStShaderCodeSharedPtr const & shaderCode : shaderCodes) {
        shaderCode->AddResourcesFromTextures(ctx);
    }

    // MipMap generation for textures requires us to submit blit work.
    SubmitBlitWork();
}

void
HdStResourceRegistry::_Commit()
{
    // Process textures first before resolving buffer sources since
    // some computation buffer sources need meta-data from textures
    // (such as the grid transform for an OpenVDB file) or texture
    // handles (for bindless textures).
    _CommitTextures();

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
        for (_PendingComputationList& compVec : _pendingComputations) {
            for (_PendingComputation &pendingComp : compVec) {
                HdComputationSharedPtr const &comp = pendingComp.computation;
                HdBufferArrayRangeSharedPtr &dstRange = pendingComp.range;
                if (dstRange) {
                    // ask the size of destination buffer of the gpu computation
                    int numElements = comp->GetNumOutputElements();
                    if (numElements > 0) {
                        // We call BufferArray->Reallocate() later so that
                        // the reallocation happens only once per BufferArray.
                        //
                        // if the range is already larger than the current one,
                        // leave it as it is (there is a possibilty that GPU
                        // computation generates less data than it was).
                        int currentNumElements = dstRange->GetNumElements();
                        if (currentNumElements < numElements) {
                            dstRange->Resize(numElements);
                        }
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

        for (_PendingSource &pendingSource : _pendingSources) {
            HdBufferArrayRangeSharedPtr &dstRange = pendingSource.range;
            // CPU computation may not have a range. (e.g. adjacency)
            if (!dstRange) continue;

            // CPU computation may result in an empty buffer source
            // (e.g. GPU quadrangulation table could be empty for quad only
            // mesh)
            if (dstRange->GetNumElements() == 0) continue;

            for (auto const& src : pendingSource.sources) {// execute copy
                dstRange->CopyData(src);

                // also copy any chained buffers
                _CopyChainedBuffers(src, dstRange);
            }

            if (TfDebug::IsEnabled(HD_BUFFER_ARRAY_RANGE_CLEANED)) {
                std::stringstream ss;
                ss << *dstRange;
                TF_DEBUG(HD_BUFFER_ARRAY_RANGE_CLEANED).Msg("CLEAN: %s\n", 
                                                            ss.str().c_str());
            }
        }
    }

    {
        HD_TRACE_SCOPE("Flush");
        // 5. flush phase:
        //
        // flush consolidated / staging buffer updates

        _nonUniformAggregationStrategy->Flush();
        _nonUniformImmutableAggregationStrategy->Flush();
        _uniformUboAggregationStrategy->Flush();
        _uniformSsboAggregationStrategy->Flush();
        _singleAggregationStrategy->Flush();

        // Make sure the writes are visible to computations that follow
        if (_blitCmds) {
            _blitCmds->MemoryBarrier(HgiMemoryBarrierAll);
        }
        SubmitBlitWork();
    }

    {
        HD_TRACE_SCOPE("GpuComputation Execute");
        // 6. execute GPU computations
        //
        // note: GPU computations have to be executed in the order that
        // they are registered.
        //   e.g. smooth normals -> quadrangulation.
        //
        for (_PendingComputationList& compVec : _pendingComputations) {
            for (_PendingComputation &pendingComp : compVec) {
                HdComputationSharedPtr const &comp = pendingComp.computation;
                HdBufferArrayRangeSharedPtr &dstRange = pendingComp.range;
                comp->Execute(dstRange, this);
                HD_PERF_COUNTER_INCR(HdPerfTokens->computationsCommited);
            }

            // Submit Hgi work between each computation queue to feed GPU.
            // Some computations may use BlitCmds (CopyComputation) so we must
            // submit blit and compute work.
            // We must ensure that shader writes are visible to computations
            // in the next queue by setting a memory barrier.
            if (_blitCmds) {
                _blitCmds->MemoryBarrier(HgiMemoryBarrierAll);
                SubmitBlitWork();
            }
            if (_computeCmds) {
                _computeCmds->MemoryBarrier(HgiMemoryBarrierAll);
                SubmitComputeWork();
            }
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
    for (_PendingComputationList& compVec : _pendingComputations) {
        compVec.clear();
    }
}

void
HdStResourceRegistry::_GarbageCollect()
{
    // The sequence in which we run garbage collection is significant.
    // We want to clean objects first which might be holding references
    // to other objects which will be subsequently cleaned up.

    GarbageCollectDispatchBuffers();
    GarbageCollectBufferResources();

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
    _glslfxFileRegistry.GarbageCollect();

    // Cleanup Hgi resources bindings and pipelines
    _resourceBindingsRegistry.GarbageCollect();
    _graphicsPipelineRegistry.GarbageCollect();
    _computePipelineRegistry.GarbageCollect();

    // cleanup buffer array
    // buffer array retains weak_ptrs of range. All unused ranges should be
    // deleted (expired) at this point.
    _nonUniformBufferArrayRegistry.GarbageCollect();
    _nonUniformImmutableBufferArrayRegistry.GarbageCollect();
    _uniformUboBufferArrayRegistry.GarbageCollect();
    _uniformSsboBufferArrayRegistry.GarbageCollect();
    _singleBufferArrayRegistry.GarbageCollect();

    // Garbage collection may reallocate buffers, so we must submit blit work.
    SubmitBlitWork();
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::_AllocateBufferArrayRange(
    HdAggregationStrategy *strategy,
    HdBufferArrayRegistry &bufferArrayRegistry,
    TfToken const &role,
    HdBufferSpecVector const &bufferSpecs,
    HdBufferArrayUsageHint usageHint)
{
    return bufferArrayRegistry.AllocateRange(
                                    strategy,
                                    role,
                                    bufferSpecs,
                                    usageHint);
}

HdBufferArrayRangeSharedPtr
HdStResourceRegistry::_UpdateBufferArrayRange(
        HdAggregationStrategy *strategy,
        HdBufferArrayRegistry &bufferArrayRegistry,
        TfToken const &role,
        HdBufferArrayRangeSharedPtr const& curRange,
        HdBufferSpecVector const &updatedOrAddedSpecs,
        HdBufferSpecVector const& removedSpecs,
        HdBufferArrayUsageHint usageHint)
{
    HD_TRACE_FUNCTION();

    if (!curRange || !curRange->IsValid()) {
        if (!removedSpecs.empty()) {
            TF_CODING_ERROR("Non-empty removed specs during BAR allocation\n");
        }

        // Allocate a new BAR and return it.
        return _AllocateBufferArrayRange(strategy, bufferArrayRegistry, role,
                    updatedOrAddedSpecs, usageHint);
    }

    HdBufferSpecVector curBufferSpecs;
    curRange->GetBufferSpecs(&curBufferSpecs);

    // Determine if the BAR needs reallocation + migration
    {
        bool haveBuffersToUpdate = !updatedOrAddedSpecs.empty();
        bool dataUpdateForImmutableBar = curRange->IsImmutable() &&
                                        haveBuffersToUpdate;
        bool usageHintChanged = curRange->GetUsageHint().value !=
                                usageHint.value;
        
        bool needsMigration =
            dataUpdateForImmutableBar ||
            usageHintChanged ||
            // buffer removal or addition
            !removedSpecs.empty() ||
            !HdBufferSpec::IsSubset(updatedOrAddedSpecs, curBufferSpecs);

        if (!needsMigration) {
            // The existing BAR can be used to queue any updates.
            return curRange;
        }
    }

    // Create new BAR ...
    HdBufferSpecVector newBufferSpecs =
        HdBufferSpec::ComputeUnion(updatedOrAddedSpecs,
            HdBufferSpec::ComputeDifference(curBufferSpecs, removedSpecs));
    
    HdBufferArrayRangeSharedPtr newRange = _AllocateBufferArrayRange(
        strategy, bufferArrayRegistry, role, newBufferSpecs, usageHint);

    // ... and migrate relevant buffers that haven't changed.
    // (skip the dirty sources, since new data needs to be copied over)
    HdBufferSpecVector migrateSpecs = HdBufferSpec::ComputeDifference(
        newBufferSpecs, updatedOrAddedSpecs);
    for (const auto& spec : migrateSpecs) {
        AddComputation(/*dstRange*/newRange,
                       std::make_shared<HdStCopyComputationGPU>(
                           /*src=*/curRange, spec.name),
                       /*CopyComp queue*/HdStComputeQueueZero);
    }

    // Increment version of the underlying bufferArray to notify
    // all batches pointing to the range to be rebuilt.
    curRange->IncrementVersion();
    
    // XXX: The existing range may no longer used. Currently, the caller is 
    // expected to flag garbage collection to reclaim its resources.
    
    HD_PERF_COUNTER_INCR(HdPerfTokens->bufferArrayRangeMigrated);

    return newRange;
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

    // misc buffers
    for (auto const & buffer: _bufferResourceRegistry) {
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

        HgiShaderProgramHandle const& prgHandle =  program->GetProgram();
        size_t size = prgHandle ? prgHandle->GetByteSizeOfResource() : 0;

        // the role of program and global uniform buffer is always same.
        std::string const &role = program->GetRole().GetString();
        (*result)[role] = VtDictionaryGet<size_t>(*result, role,
                                                  VtDefault = 0) + size;

        gpuMemoryUsed += size;
    }

    // Texture Memory and other texture information
    {
        HdSt_TextureObjectRegistry *const textureObjectRegistry =
            _textureHandleRegistry->GetTextureObjectRegistry();

        const size_t textureMemory =
            textureObjectRegistry->GetTotalTextureMemory();

        (*result)[HdPerfTokens->textureMemory] = VtValue(textureMemory);
        gpuMemoryUsed += textureMemory;

        const size_t numTexObjects =
            textureObjectRegistry->GetNumberOfTextureObjects();
        (*result)[_perfTokens->numberOfTextureObjects] = VtValue(numTexObjects);

        const size_t numTexHandles =
            _textureHandleRegistry->GetNumberOfTextureHandles();
        (*result)[_perfTokens->numberOfTextureHandles] = VtValue(numTexHandles);
            
    }

    (*result)[HdPerfTokens->gpuMemoryUsed.GetString()] = gpuMemoryUsed;
}

HdStTextureHandleSharedPtr
HdStResourceRegistry::AllocateTextureHandle(
        HdStTextureIdentifier const &textureId,
        const HdTextureType textureType,
        HdSamplerParameters const &samplerParams,
        const size_t memoryRequest,
        const bool createBindlessHandle,
        HdStShaderCodePtr const &shaderCode)
{
    return _textureHandleRegistry->AllocateTextureHandle(
        textureId, textureType,
        samplerParams, memoryRequest, createBindlessHandle,
        shaderCode);
}

HdStTextureObjectSharedPtr
HdStResourceRegistry::AllocateTextureObject(
        HdStTextureIdentifier const &textureId,
        const HdTextureType textureType)
{
    HdSt_TextureObjectRegistry * const reg = 
        _textureHandleRegistry->GetTextureObjectRegistry();
        
    return reg->AllocateTextureObject(
        textureId, textureType);
            
}    

void
HdStResourceRegistry::SetMemoryRequestForTextureType(
    const HdTextureType textureType,
    const size_t memoryRequest)
{
    _textureHandleRegistry->SetMemoryRequestForTextureType(
        textureType, memoryRequest);
}

PXR_NAMESPACE_CLOSE_SCOPE
