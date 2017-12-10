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
#ifndef HD_TEXTURE_H
#define HD_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bprim.h"

#include "pxr/imaging/garch/gl.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef boost::shared_ptr<class HdTextureResource> HdTextureResourceSharedPtr;

///
/// Represents a Texture Buffer Prim.
/// Texture could be a uv texture or a ptex texture.
/// Multiple texture prims could represent the same texture buffer resource
/// and the scene delegate is used to get a global unique id for the texture.
/// The delegate is also used to obtain a HdTextureResource for the texture
/// represented by that id.
///
class HdTexture final : public HdBprim {
public:
    // change tracking for HdTexture
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyParams           = 1 << 0,
        DirtyTexture          = 1 << 1,
        AllDirty              = (DirtyParams
                                |DirtyTexture)
    };

    HD_API
    HdTexture(SdfPath const & id);
    HD_API
    virtual ~HdTexture();

    /// Synchronizes state from the delegate to Hydra, for example, allocating
    /// parameters into GPU memory.
    HD_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HD_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    // ---------------------------------------------------------------------- //
    /// \name Texture API
    // ---------------------------------------------------------------------- //
    
    /// Returns true if the texture should be interpreted as a PTex texture.
    HD_API
    bool IsPtex() const;

    /// Returns true if mipmaps should be generated when loading.
    HD_API
    bool ShouldGenerateMipMaps() const;

private:

    // Make sure we have a reference to the texture resource, so its
    // life time exists at least as long as this object.
    HdTextureResourceSharedPtr _textureResource;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_TEXTURE_H

