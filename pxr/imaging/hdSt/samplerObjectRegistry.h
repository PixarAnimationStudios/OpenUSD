//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HD_ST_SAMPLER_OBJECT_REGISTRY_H
#define PXR_IMAGING_HD_ST_SAMPLER_OBJECT_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSamplerParameters;
using HdStTextureObjectSharedPtr =
    std::shared_ptr<class HdStTextureObject>;
using HdStSamplerObjectSharedPtr =
    std::shared_ptr<class HdStSamplerObject>;
class HdStResourceRegistry;

/// \class HdSt_SamplerObjectRegistry
///
/// A simple registry for GPU samplers and GL texture sampler handles
/// (for bindless textures).
///
/// The registry makes no attempt at de-duplication. But construction
/// is dispatched by texture type returing a matching sampler (e.g.,
/// HdStFieldSamplerObject for a HdStFieldTextureObject or
/// HdStPtexSamplerObject for the (not yet existing)
/// HdStPtexTextureObject). Also, it keeps a shared pointer to a sampler
/// around until garbage collection so that clients can safely drop their
/// shared pointers from different threads.
///
class HdSt_SamplerObjectRegistry final
{
public:
    HDST_API
    explicit HdSt_SamplerObjectRegistry(HdStResourceRegistry * registry);

    HDST_API 
    ~HdSt_SamplerObjectRegistry();

    /// Create new sampler object matching the given texture object.
    ///
    /// If createBindlessHandle, also creates a texture sampler handle
    /// (for bindless textures). The associated GPU resource is
    /// created immediately and the call is not thread-safe.
    HDST_API 
    HdStSamplerObjectSharedPtr AllocateSampler(
        HdStTextureObjectSharedPtr const &texture,
        HdSamplerParameters const &samplerParameters,
        bool createBindlessHandle);

    /// Delete samplers no longer used by a client.
    HDST_API 
    void GarbageCollect();

    HDST_API
    void MarkGarbageCollectionNeeded();

    /// Get resource registry
    ///
    HDST_API
    HdStResourceRegistry * GetResourceRegistry() const;

private:
    std::vector<HdStSamplerObjectSharedPtr> _samplerObjects;
    
    bool _garbageCollectionNeeded;
    HdStResourceRegistry *_resourceRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
