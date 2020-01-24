//
// Copyright 2019 Pixar
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
#ifndef EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H
#define EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H

#include "pxr/pxr.h"
#include "hdPrman/context.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanInstancer : public HdInstancer {
public:
    HdPrmanInstancer(HdSceneDelegate* delegate, SdfPath const& id,
                      SdfPath const &parentInstancerId);

    /// Destructor.
    ~HdPrmanInstancer();

    /// Computes all instance transforms for the provided prototype id,
    /// taking into account the scene delegate's instancerTransform and the
    /// instance primvars "instanceTransform", "translate", "rotate", "scale".
    /// Computes and flattens nested transforms, if necessary.
    ///   \param prototypeId The prototype to compute transforms for.
    ///   \return One transform per instance, to apply when drawing.
    VtMatrix4dArray ComputeInstanceTransforms(SdfPath const &prototypeId);

    /// Sample the instance transforms for the given prototype.
    void SampleInstanceTransforms(
        SdfPath const& prototypeId,
        VtIntArray const& instanceIndices,
        HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> *sa);

    /// Convert instance-rate primvars to Riley attributes, using
    /// the instance to index into the array.
    void GetInstancePrimvars(
        SdfPath const& prototypeId,
        size_t instanceIndex,
        RtParamList& attrs);

    // Checks the change tracker to determine whether instance primvars are
    // dirty, and if so pulls them. Since primvars can only be pulled once,
    // and are cached, this function is not re-entrant. However, this function
    // is called by ComputeInstanceTransforms, which is called (potentially)
    // by HdPrmanMesh::Sync(), which is dispatched in parallel, so it needs
    // to be guarded by _instanceLock.
    //
    // Pulled primvars are cached in _primvarMap.
    void SyncPrimvars();

private:

    // Mutex guard for SyncPrimvars().
    std::mutex _instanceLock;

    // Map of the latest primvar data for this instancer, keyed by primvar name
    struct _PrimvarValue {
        HdPrimvarDescriptor desc;
        VtValue value;
    };
    TfHashMap<TfToken,
              _PrimvarValue,
              TfToken::HashFunctor> _primvarMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H
