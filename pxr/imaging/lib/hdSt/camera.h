//
// Copyright 2017 Pixar
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
#ifndef HDST_CAMERA_H
#define HDST_CAMERA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/gf/matrix4d.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


#define HDST_CAMERA_TOKENS                      \
    (clipPlanes)                                \
    (worldToViewMatrix)                         \
    (worldToViewInverseMatrix)                  \
    (projectionMatrix)                          \
    (windowPolicy)

TF_DECLARE_PUBLIC_TOKENS(HdStCameraTokens, HDST_API, HDST_CAMERA_TOKENS);

class HdSceneDelegate;

/// \class HdStCamera
///
/// A camera model, used in conjunction with HdRenderPass.
///
class HdStCamera final : public HdSprim {
public:
    typedef std::vector<GfVec4d> ClipPlanesVector;

    HDST_API
    HdStCamera(SdfPath const & id);
    HDST_API
    virtual ~HdStCamera();

    // change tracking for HdStCamera
    enum DirtyBits {
        Clean                 = 0,
        DirtyViewMatrix       = 1 << 0,
        DirtyProjMatrix       = 1 << 1,
        DirtyWindowPolicy     = 1 << 2,
        DirtyClipPlanes       = 1 << 3,
        AllDirty              = (DirtyViewMatrix
                                |DirtyProjMatrix
                                |DirtyWindowPolicy
                                |DirtyClipPlanes)
    };

    /// Synchronizes state from the delegate to this object.
    HDST_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    /// Accessor for tasks to get the parameters cached in this object.
    HDST_API
    virtual VtValue Get(TfToken const &token) const override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDST_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _cameraValues;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_CAMERA_H
