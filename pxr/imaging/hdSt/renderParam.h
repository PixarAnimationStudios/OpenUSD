//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_PLUGIN_HD_ST_RENDER_PARAM_H
#define PXR_IMAGING_PLUGIN_HD_ST_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hdSt/api.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdStRenderParam
///
/// The render delegate can create an object of type HdRenderParam, to pass
/// to each prim during Sync(). Storm uses this class to house global
/// counters amd flags that assist invalidation of draw batch caches.
/// 
class HdStRenderParam final : public HdRenderParam
{
public:
    HdStRenderParam();
    ~HdStRenderParam() override;

    // ---------------------------------------------------------------------- //
    /// Draw items cache and batch invalidation
    // ---------------------------------------------------------------------- //
    /// Marks all batches dirty, meaning they need to be validated and
    /// potentially rebuilt.
    HDST_API
    void MarkDrawBatchesDirty();

    HDST_API
    unsigned int GetDrawBatchesVersion() const;

    /// Marks material tags dirty, meaning that the draw items associated with
    /// the collection of a render pass need to be re-gathered.
    HDST_API
    void MarkMaterialTagsDirty();

    HDST_API
    unsigned int GetMaterialTagsVersion() const;

    // ---------------------------------------------------------------------- //
    /// Garbage collection tracking
    // ---------------------------------------------------------------------- //
    void SetGarbageCollectionNeeded() {
        _needsGarbageCollection = true;
    }

    void ClearGarbageCollectionNeeded() {
        _needsGarbageCollection = false;
    }

    bool IsGarbageCollectionNeeded() const {
        return _needsGarbageCollection;
    }
    
private:
    std::atomic_uint _drawBatchesVersion;
    std::atomic_uint _materialTagsVersion;
    bool _needsGarbageCollection; // Doesn't need to be atomic since parallel
                                  // sync might only set it (and not clear).
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_ST_RENDER_PARAM_H
