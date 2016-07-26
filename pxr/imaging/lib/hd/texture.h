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

#include "pxr/imaging/hd/version.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

class HdSceneDelegate;

typedef boost::shared_ptr<class HdTexture> HdTextureSharedPtr;
typedef boost::shared_ptr<class HdTextureResource> HdTextureResourceSharedPtr;

/// XXX: Docs
class HdTexture {
public:
    HdTexture(HdSceneDelegate* delegate, SdfPath const & id);
    virtual ~HdTexture();

    /// Returns the HdSceneDelegate which backs this texture.
    HdSceneDelegate* GetDelegate() const { return _delegate; }

    /// Returns the identifer by which this surface texture is known. This
    /// identifier is a common associative key used by the SceneDelegate,
    /// RenderIndex, and for binding to the Rprim.
    SdfPath const& GetID() const { return _id; }

    /// Synchronizes state from the delegate to Hydra, for example, allocating
    /// parameters into GPU memory.
    void Sync();

    // ---------------------------------------------------------------------- //
    /// \name Texture API
    // ---------------------------------------------------------------------- //
    
    /// Returns the binary data for the texture.
    HdTextureResourceSharedPtr GetTextureData() const;

    /// Returns true if the texture should be interpreted as a PTex texture.
    bool IsPtex() const;

    /// Returns true if mipmaps should be generated when loading.
    bool ShouldGenerateMipMaps() const;

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;

    HdTextureResourceSharedPtr _textureResource;
};

#endif //HD_TEXTURE_H

