//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// A simple registry for GPU samplers.
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
    /// The associated GPU resource is created immediately and
    /// the call is not thread-safe.
    HDST_API 
    HdStSamplerObjectSharedPtr AllocateSampler(
        HdStTextureObjectSharedPtr const &texture,
        HdSamplerParameters const &samplerParameters);

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
