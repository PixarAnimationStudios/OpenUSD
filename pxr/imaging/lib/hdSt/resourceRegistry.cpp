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
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/tokens.h"
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
        new HdStVBOMemoryManager(/*isImmutable=*/false));
    SetNonUniformImmutableAggregationStrategy(
        new HdStVBOMemoryManager(/*isImmutable=*/true));

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

PXR_NAMESPACE_CLOSE_SCOPE
