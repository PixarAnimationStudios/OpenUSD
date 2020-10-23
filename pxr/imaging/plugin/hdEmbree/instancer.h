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
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_INSTANCER_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_INSTANCER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdEmbreeInstancer
///
/// HdEmbree implements instancing by adding prototype geometry to the BVH
/// multiple times within HdEmbreeMesh::Sync(). The only instance-varying
/// attribute that HdEmbree supports is transform, so the natural
/// accessor to instancer data is ComputeInstanceTransforms(),
/// which returns a list of transforms to apply to the given prototype
/// (one instance per transform).
///
/// Nested instancing can be handled by recursion, and by taking the
/// cartesian product of the transform arrays at each nesting level, to
/// create a flattened transform array.
///
class HdEmbreeInstancer : public HdInstancer {
public:
    /// Constructor.
    ///   \param delegate The scene delegate backing this instancer's data.
    ///   \param id The unique id of this instancer.
    HdEmbreeInstancer(HdSceneDelegate* delegate, SdfPath const& id);

    /// Destructor.
    ~HdEmbreeInstancer();

    /// Computes all instance transforms for the provided prototype id,
    /// taking into account the scene delegate's instancerTransform and the
    /// instance primvars "instanceTransform", "translate", "rotate", "scale".
    /// Computes and flattens nested transforms, if necessary.
    ///   \param prototypeId The prototype to compute transforms for.
    ///   \return One transform per instance, to apply when drawing.
    VtMatrix4dArray ComputeInstanceTransforms(SdfPath const &prototypeId);

    /// Updates cached primvar data from the scene delegate.
    ///   \param sceneDelegate The scene delegate for this prim.
    ///   \param renderParam The hdEmbree render param.
    ///   \param dirtyBits The dirty bits for this instancer.
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

private:
    // Updates the cached primvars in _primvarMap based on scene delegate
    // data.  This is a helper function for Sync().
    void _SyncPrimvars(HdSceneDelegate *delegate, HdDirtyBits dirtyBits);

    // Map of the latest primvar data for this instancer, keyed by
    // primvar name. Primvar values are VtValue, an any-type; they are
    // interpreted at consumption time (here, in ComputeInstanceTransforms).
    TfHashMap<TfToken,
              HdVtBufferSource*,
              TfToken::HashFunctor> _primvarMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_PLUGIN_HD_EMBREE_INSTANCER_H
