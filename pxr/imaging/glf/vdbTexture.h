//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_GLF_VDB_TEXTURE_H
#define PXR_IMAGING_GLF_VDB_TEXTURE_H

/// \file glf/vdbTexture.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/baseTexture.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfVdbTextureContainer);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfVdbTexture);

/// \class GlfVdbTexture
///
/// Represents a 3-dimensional texture read from grid in an OpenVDB file.
///
/// This texture is supposed to be held by a GlfVdbTextureContainer which
/// tells this texture also what OpenVDB file to read.
///
class GlfVdbTexture : public GlfBaseTexture {
public:
    /// Creates a new texture instance for the grid named \gridName in
    /// the OpenVDB file opened by \p textureContainer.
    GLF_API
    static GlfVdbTextureRefPtr New(
        GlfVdbTextureContainerRefPtr const &textureContainer,
        TfToken const &gridName);

    /// Returns the transform of the grid in the OpenVDB file as well as the
    /// bounding box of the samples in the corresponding OpenVDB tree.
    ///
    /// This pair of information is encoded as GfBBox3d.
    GLF_API
    const GfBBox3d &GetBoundingBox();

    int GetNumDimensions() const override;
    
    GLF_API
    VtDictionary GetTextureInfo(bool forceLoad) override;

    GLF_API
    bool IsMinFilterSupported(GLenum filter) override;

protected:
    GLF_API
    GlfVdbTexture(
        GlfVdbTextureContainerRefPtr const &textureContainer,
        TfToken const &gridName);

    GLF_API
    void _ReadTexture() override;

    GLF_API
    bool _GenerateMipmap() const;

private:
    GlfVdbTextureContainerRefPtr const _textureContainer;
    const TfToken _gridName;

    GfBBox3d _boundingBox;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_VDBTEXTURE_H
