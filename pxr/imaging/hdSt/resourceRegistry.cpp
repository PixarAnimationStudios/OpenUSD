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

HdStResourceRegistry::HdStResourceRegistry()
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
    /*NOTHING*/
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

HdStDispatchBufferSharedPtr
HdStResourceRegistry::RegisterDispatchBuffer(
    TfToken const &role, int count, int commandNumUints)
{
    HdStDispatchBufferSharedPtr result(
        new HdStDispatchBuffer(role, count, commandNumUints));

    _dispatchBufferRegistry.push_back(result);

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

static bool _IsEnabledResourceInstancing()
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

HdInstance<HdSt_BasisCurvesTopologySharedPtr>
HdStResourceRegistry::RegisterBasisCurvesTopology(
        HdInstance<HdSt_BasisCurvesTopologySharedPtr>::ID id)
{
    return _Register(id, _basisCurvesTopologyRegistry,
                     HdPerfTokens->instBasisCurvesTopology);
}

HdInstance<HdSt_MeshTopologySharedPtr>
HdStResourceRegistry::RegisterMeshTopology(
        HdInstance<HdSt_MeshTopologySharedPtr>::ID id)
{
    return _Register(id, _meshTopologyRegistry,
                     HdPerfTokens->instMeshTopology);
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

void HdStResourceRegistry::InvalidateShaderRegistry()
{
    _geometricShaderRegistry.Invalidate();
}

PXR_NAMESPACE_CLOSE_SCOPE
