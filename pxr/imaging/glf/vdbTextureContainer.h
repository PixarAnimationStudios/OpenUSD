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
#ifndef PXR_IMAGING_GLF_VDB_TEXTURE_CONTAINER_H
#define PXR_IMAGING_GLF_VDB_TEXTURE_CONTAINER_H

/// \file glf/vdbTextureContainer.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/texture.h"
#include "pxr/imaging/glf/textureContainer.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfTextureHandle);

TF_DECLARE_WEAK_AND_REF_PTRS(GlfVdbTextureContainer);

/// \class GlfVdbTextureContainer
///
/// A container for 3-dimension textures read from the grids in an OpenVDB file
///
class GlfVdbTextureContainer : public GlfTextureContainer<TfToken> {
public:
    /// Creates a new container for the OpenVDB file \p filePath
    GLF_API
    static GlfVdbTextureContainerRefPtr New(TfToken const &filePath);

    /// Creates a new container for the OpenVDB file \p filePath
    GLF_API
    static GlfVdbTextureContainerRefPtr New(std::string const &filePath);

    GLF_API
    ~GlfVdbTextureContainer() override;

    /// Returns invalid texture name.
    /// 
    /// Clients are supposed to get texture information from the GlfVdbTexture
    /// returned by GlfVdbTextureContainer::GetTextureHandle() 
    GLF_API
    GLuint GetGlTextureName() override;

    
    /// Returns empty vector.
    /// 
    /// Clients are supposed to get texture information from the GlfVdbTexture
    /// returned by GlfVdbTextureContainer::GetTextureHandle() 
    GLF_API
    BindingVector GetBindings(TfToken const & identifier,
                              GLuint samplerName) override;

    /// Returns empty dict.
    /// 
    /// Clients are supposed to get texture information from the GlfVdbTexture
    /// returned by GlfVdbTextureContainer::GetTextureHandle() 
    GLF_API
    VtDictionary GetTextureInfo(bool forceLoad) override;

    /// The file path of the OpenVDB file
    ///
    GLF_API
    TfToken const &GetFilePath() const { return _filePath; }

private:
    GlfVdbTextureContainer(TfToken const &filePath);

    GlfTextureRefPtr _CreateTexture(TfToken const &identifier) override;

private:
    const TfToken _filePath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
