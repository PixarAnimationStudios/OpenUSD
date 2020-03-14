//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_GLF_TEXTURE_CONTAINER_H
#define PXR_IMAGING_GLF_TEXTURE_CONTAINER_H

/// \file glf/textureContainer.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"

#include "pxr/base/tf/declarePtrs.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfTextureHandle);
TF_DECLARE_REF_PTRS(GlfTexture);

/// \class GlfTextureContainer
///
/// A base class for texture containers, e.g., for a movie file where a frame
/// corresponds to a texture, for an exr file where a subimage corresponds
/// to a texture, or for an OpenVDB file where a grid corresponds to a texture.
///
/// Templated since for, e.g., a movie we would key the container by frame
/// number but for an exr file by subimage name.
///
/// A note on garbage collection: texture containers are registered with and
/// will be garbage collected by the texture registry. Thus, a texture in a
/// container has to hold on to a ref ptr to the container's handle so that
/// the registry won't delete the container while any of the textures in the
/// container is in use. See GarbageCollect for more details.
///
template<typename Identifier>
class GlfTextureContainer : public GlfTexture {
public:
   
    /// Get texture handle for a particular frame, subimage, grid, ...
    GLF_API
    GlfTextureHandlePtr GetTextureHandle(Identifier const &identifier);

    /// Implements the garbage collection of textures in this container.
    ///
    /// When the Glf clients give up all their references to all textures in
    /// this container, garbage collection will happen in two steps:
    /// the container will note that there are no other references and
    /// deletes the textures which references the container.
    /// Thus, the texture registry is having the only remaining reference to
    /// the container and will delete the container.
    GLF_API
    void GarbageCollect() override;

protected:
    // Create texture for a particular frame, subimage, grid, ...
    virtual GlfTextureRefPtr _CreateTexture(Identifier const &identifier) = 0;

    // Texture handles for frames, subimage, grids, ...
    std::map<Identifier, GlfTextureHandleRefPtr> _textureHandles;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
    
