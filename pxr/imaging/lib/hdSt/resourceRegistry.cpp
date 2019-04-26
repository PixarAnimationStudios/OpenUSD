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

#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/copyComputation.h"
#include "pxr/imaging/hdSt/dispatchBuffer.h"
#include "pxr/imaging/hdSt/persistentBuffer.h"
#include "pxr/imaging/hdSt/interleavedMemoryManager.h"
#include "pxr/imaging/hdSt/vboMemoryManager.h"
#include "pxr/imaging/hdSt/vboSimpleMemoryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


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
    GarbageCollectDispatchBuffers();
    GarbageCollectPersistentBuffers();

    // Cleanup Shader registries
    _geometricShaderRegistry.GarbageCollect();
    _glslProgramRegistry.GarbageCollect();
}

void
HdStResourceRegistry::_TallyResourceAllocation(VtDictionary *result) const
{
    size_t gpuMemoryUsed =
        VtDictionaryGet<size_t>(*result,
                                HdPerfTokens->gpuMemoryUsed.GetString(),
                                VtDefault = 0);

    // dispatch buffers
    TF_FOR_ALL (bufferIt, _dispatchBufferRegistry) {
        HdStDispatchBufferSharedPtr buffer = (*bufferIt);
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
    TF_FOR_ALL (bufferIt, _persistentBufferRegistry) {
        HdStPersistentBufferSharedPtr buffer = (*bufferIt);
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
    TF_FOR_ALL (progIt, _glslProgramRegistry) {
        HdStGLSLProgramSharedPtr const &program = progIt->second;
        if (!program) continue;
        size_t size =
            program->GetProgram().GetSize() +
            program->GetGlobalUniformBuffer().GetSize();

        // the role of program and global uniform buffer is always same.
        std::string const &role = program->GetProgram().GetRole().GetString();
        (*result)[role] = VtDictionaryGet<size_t>(*result, role,
                                                  VtDefault = 0) + size;

        gpuMemoryUsed += size;
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

std::unique_lock<std::mutex>
HdStResourceRegistry::RegisterGeometricShader(HdStShaderKey::ID id,
                        HdInstance<HdStShaderKey::ID, HdSt_GeometricShaderSharedPtr> *instance)
{
    return _geometricShaderRegistry.GetInstance(id, instance);
}

std::unique_lock<std::mutex>
HdStResourceRegistry::RegisterGLSLProgram(HdStGLSLProgram::ID id,
              HdInstance<HdStGLSLProgram::ID, HdStGLSLProgramSharedPtr> *instance)
{
    return _glslProgramRegistry.GetInstance(id, instance);
}

void HdStResourceRegistry::InvalidateShaderRegistry()
{
    _geometricShaderRegistry.Invalidate();
}

PXR_NAMESPACE_CLOSE_SCOPE
