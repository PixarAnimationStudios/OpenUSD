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
#ifndef HDX_CAMERA_H
#define HDX_CAMERA_H

#include "pxr/imaging/hdx/api.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/dictionary.h"

#include <boost/shared_ptr.hpp>

#define HDX_CAMERA_TOKENS                       \
    (clipPlanes)                                \
    (cameraFrustum)                             \
    (worldToViewMatrix)                         \
    (worldToViewInverseMatrix)                  \
    (projectionMatrix)                          \
    (windowPolicy)

TF_DECLARE_PUBLIC_TOKENS(HdxCameraTokens, HDXLIB_API, HDX_CAMERA_TOKENS);

class HdSceneDelegate;
typedef boost::shared_ptr<class HdxCamera> HdxCameraSharedPtr;

/// \class HdxCamera
///
/// A camera model, used in conjunction with HdRenderPass.
///
class HdxCamera : public HdSprim {
public:
    typedef std::vector<GfVec4d> ClipPlanesVector;

    HDXLIB_API
    HdxCamera(HdSceneDelegate* delegate, SdfPath const & id);

    HDXLIB_API
    ~HdxCamera();  // note: not virtual (for now)

    // change tracking for HdxLight
    enum DirtyBits {
        Clean                 = 0,
        DirtyParams           = 1 << 0,
        DirtyWindowPolicy     = 1 << 1,
        DirtyClipPlanes       = 1 << 2,
        AllDirty              = (DirtyParams
                                 |DirtyWindowPolicy
                                 |DirtyClipPlanes)
    };

    /// Synchronizes state from the delegate to this object.
    virtual void Sync();

    /// Accessor for tasks to get the parameters cached in this object.
    virtual VtValue Get(TfToken const &token) const;

private:
    TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _cameraValues;
};

#endif //HDX_CAMERA_H
