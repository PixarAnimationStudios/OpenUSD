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
#ifndef GLF_ARRAYTEXTURE_H
#define GLF_ARRAYTEXTURE_H

/// \file glf/arrayTexture.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/uvTexture.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfArrayTexture);

/// \class GlfArrayTexture
///
/// Represents an array of texture objects in Glf
///
/// An GlfArrayTexture is defined by a set of image file paths.
/// Currently accepted image formats are png, jpg and bmp.
///

class GlfArrayTexture : public GlfUVTexture {
public:

    typedef GlfUVTexture Parent;
    typedef GlfArrayTexture This;
    
    /// Creates a new texture instance for the image file at \p imageFilePath.
    /// If given, \p cropTop, \p cropBottom, \p cropLeft, and \p cropRight
    /// specifies the number of pixels to crop from the indicated border of
    /// the source image.
    static GlfArrayTextureRefPtr New(
        TfTokenVector const &imageFilePaths,
        unsigned int arraySize     ,
        unsigned int cropTop    = 0,
        unsigned int cropBottom = 0,
        unsigned int cropLeft   = 0,
        unsigned int cropRight  = 0);

    static GlfArrayTextureRefPtr New(
        std::vector<std::string> const &imageFilePaths,
        unsigned int arraySize     ,
        unsigned int cropTop    = 0,
        unsigned int cropBottom = 0,
        unsigned int cropLeft   = 0,
        unsigned int cropRight  = 0);

    
    static bool IsSupportedImageFile(TfToken const &imageFilePath);

    // GlfBaseTexture overrides
    virtual BindingVector GetBindings(TfToken const & identifier,
                                      GLuint samplerName) const;

protected:
    GlfArrayTexture(
        TfTokenVector const &imageFilePaths,
        unsigned int arraySize,
        unsigned int cropTop,
        unsigned int cropBottom,
        unsigned int cropLeft,
        unsigned int cropRight);

    virtual void _OnSetMemoryRequested(size_t targetMemory);
    const TfToken& _GetImageFilePath(size_t index) const;
    using GlfUVTexture::_GetImageFilePath;

    void _CreateTexture(GlfBaseTextureDataConstRefPtrVector texDataVec,
                        bool const generateMipmap);

private:

    TfTokenVector _imageFilePaths;
    const unsigned int _arraySize;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_ARRAYTEXTURE_H
