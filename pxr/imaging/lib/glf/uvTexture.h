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
#ifndef GLF_UVTEXTURE_H
#define GLF_UVTEXTURE_H

/// \file glf/uvTexture.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/baseTexture.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfUVTexture);

/// \class GlfUVTexture
///
/// Represents a texture object in Glf.
///
/// An GlfUVTexture is currently defined by an image file path.
/// Currently accepted image formats are png, jpg and bmp.
///
class GlfUVTexture : public GlfBaseTexture {
public:
    /// Creates a new texture instance for the image file at \p imageFilePath.
    /// If given, \p cropTop, \p cropBottom, \p cropLeft, and \p cropRight
    /// specifies the number of pixels to crop from the indicated border of
    /// the source image.
    static GlfUVTextureRefPtr New(
        TfToken const &imageFilePath,
        unsigned int cropTop    = 0,
        unsigned int cropBottom = 0,
        unsigned int cropLeft   = 0,
        unsigned int cropRight  = 0);

    static GlfUVTextureRefPtr New(
        std::string const &imageFilePath,
        unsigned int cropTop    = 0,
        unsigned int cropBottom = 0,
        unsigned int cropLeft   = 0,
        unsigned int cropRight  = 0);
    
    /// Returns true if the file at \p imageFilePath is an image that
    /// can be used with this texture object.
    static bool IsSupportedImageFile(TfToken const &imageFilePath);
    static bool IsSupportedImageFile(std::string const &imageFilePath);

    virtual VtDictionary GetTextureInfo() const;

    virtual bool IsMinFilterSupported(GLenum filter);

protected:
    GlfUVTexture(
        TfToken const &imageFilePath,
        unsigned int cropTop,
        unsigned int cropBottom,
        unsigned int cropLeft,
        unsigned int cropRight);

    virtual void _OnSetMemoryRequested(size_t targetMemory);
    virtual bool _GenerateMipmap() const;
    const TfToken& _GetImageFilePath() const;
    unsigned int _GetCropTop() const {return _cropTop;}
    unsigned int _GetCropBottom() const {return _cropBottom;}
    unsigned int _GetCropLeft() const {return _cropLeft;}
    unsigned int _GetCropRight() const {return _cropRight;}

private:
    const TfToken _imageFilePath;
    const unsigned int _cropTop;
    const unsigned int _cropBottom;
    const unsigned int _cropLeft;
    const unsigned int _cropRight;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_UVTEXTURE_H
