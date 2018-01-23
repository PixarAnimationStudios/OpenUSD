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
#ifndef HDST_TEXTURE_H
#define HDST_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/texture.h"

#include "pxr/imaging/garch/gl.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef std::shared_ptr<class HdTextureResource> HdTextureResourceSharedPtr;

///
/// Represents a Texture Buffer Prim.
/// Texture could be a uv texture or a ptex texture.
/// Multiple texture prims could represent the same texture buffer resource
/// and the scene delegate is used to get a global unique id for the texture.
/// The delegate is also used to obtain a HdStSimpleTextureResource for the texture
/// represented by that id.
///
class HdStTexture : public HdTexture {
public:
    HdStTexture(SdfPath const & id);
    virtual ~HdStTexture();

protected:
    virtual HdTextureResourceSharedPtr _GetTextureResource(
        HdSceneDelegate *sceneDelegate,
        const SdfPath &sceneId,
        HdTextureResource::ID texID) const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_TEXTURE_H

