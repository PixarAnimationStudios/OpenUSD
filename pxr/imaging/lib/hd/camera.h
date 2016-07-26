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
#ifndef HD_CAMERA_H
#define HD_CAMERA_H

#include "pxr/imaging/hd/version.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/vt/dictionary.h"

#include <boost/shared_ptr.hpp>

class HdSceneDelegate;
typedef boost::shared_ptr<class HdCamera> HdCameraSharedPtr;

/// A camera model, used in conjunction with HdRenderPass.

class HdCamera {
public:
    typedef std::vector<GfVec4d> ClipPlanesVector;

    HdCamera(HdSceneDelegate* delegate, SdfPath const & id);
    ~HdCamera();  // note: not virtual (for now)

    /// Returns the HdSceneDelegate which backs this texture.
    HdSceneDelegate* GetDelegate() const { return _delegate; }

    /// Returns the identifer by which this light is known. This
    /// identifier is a common associative key used by the SceneDelegate,
    /// RenderIndex, and for binding to the light
    SdfPath const& GetID() const { return _id; }

    /// Synchronizes state from the delegate to Hydra, for example, allocating
    /// parameters into GPU memory.
    void Sync();

    // ---------------------------------------------------------------------- //
    /// \name Camera API
    // ---------------------------------------------------------------------- //

    VtValue Get(TfToken const &name);

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;

    TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _cameraValues;
};

#endif //HD_CAMERA_H
