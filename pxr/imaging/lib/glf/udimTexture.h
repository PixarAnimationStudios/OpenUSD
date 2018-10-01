//
// Copyright 2018 Pixar
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
#ifndef GLF_UDIMTEXTURE_H
#define GLF_UDIMTEXTURE_H

/// \file glf/udimTexture.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"

#include "pxr/imaging/glf/texture.h"

#include <string>
#include <vector>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

/// Returns true if the file given by \p imageFilePath represents a udim file,
/// and false otherwise.
///
/// This function simply checks the existence of the <udim> tag in the
/// file name and does not otherwise guarantee that
/// the file is in any way valid for reading.
///
GLF_API bool GlfIsSupportedUdimTexture(std::string const& imageFilePath);

class GlfUdimTexture;
TF_DECLARE_WEAK_AND_REF_PTRS(GlfUdimTexture);

class GlfUdimTexture : public GlfTexture {
public:
    GLF_API
    virtual ~GlfUdimTexture();

    GLF_API
    static GlfUdimTextureRefPtr New(
        TfToken const& imageFilePath,
        GlfImage::ImageOriginLocation originLocation,
        std::vector<std::tuple<int, TfToken>>&& tiles);

    GLF_API
    GlfTexture::BindingVector GetBindings(
        TfToken const& identifier,
        GLuint samplerId) override;

    GLF_API
    VtDictionary GetTextureInfo(bool forceLoad) override;

    GLuint GetGlTextureName() {
        _ReadImage();
        return _imageArray;
    }

    GLuint GetGlLayoutName() {
        _ReadImage();
        return _layout;
    }

protected:
    GLF_API
    GlfUdimTexture(
        TfToken const& imageFilePath,
        GlfImage::ImageOriginLocation originLocation,
        std::vector<std::tuple<int, TfToken>>&& tiles);

    GLF_API
    void _FreeTextureObject();

    GLF_API
    void _ReadImage();

    GLF_API
    void _OnMemoryRequestedDirty() override;
private:
    std::vector<std::tuple<int, TfToken>> _tiles;
    unsigned int _width = 0;
    unsigned int _height = 0;
    unsigned int _depth = 0;
    unsigned int _format = 0;
    GLuint _imageArray = 0;
    GLuint _layout = 0;
    bool _loaded = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_UDIMTEXTURE_H
