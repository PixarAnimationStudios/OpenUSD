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
#ifndef HD_LIGHT_H
#define HD_LIGHT_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"

#include "pxr/imaging/glf/simpleLight.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/value.h"

#include <boost/shared_ptr.hpp>

#include <vector>

class HdSceneDelegate;
typedef boost::shared_ptr<class HdLight> HdLightSharedPtr;
typedef std::vector<HdLightSharedPtr> HdLightSharedPtrVector;

/// \class HdLight
///
/// A light model, used in conjunction with HdRenderPass.
///
class HdLight {
public:
    HdLight(HdSceneDelegate* delegate, SdfPath const & id);
    ~HdLight(); // note: not virtual (for now)

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
    /// \name Light API
    // ---------------------------------------------------------------------- //

    /// Returns the transform for the light.
    const GfMatrix4d &GetTransform() const { return _transform; }

    /// Returns the light parameters
    const GlfSimpleLight &GetParams() const { return _params; }

    /// Returns shadow parameters for the light as a VtValue.
    // XXX: Promote shadow to Hd (maybe even first class citizen in Hd?)
    const VtValue &GetShadowParams() const { return _shadowParams; }

    /// Returns the collection of prims that cast a shadow using this light
    const HdRprimCollection &GetShadowCollection() const
    {
        return _shadowCollection;
    }

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;

    GfMatrix4d        _transform;
    GlfSimpleLight    _params;

    // XXX: Promote shadow to Hd (maybe even first class citizen in Hd?)
    VtValue           _shadowParams;
    HdRprimCollection _shadowCollection;

};

#endif  // HD_LIGHT_H
