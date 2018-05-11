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
#ifndef GLF_IMAGE_H
#define GLF_IMAGE_H

/// \file glf/image.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/garch/gl.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class GlfImage> GlfImageSharedPtr;

/// \class GlfImage
///
/// A base class for reading and writing texture image data.
///
/// The class allows basic access to texture image file data.
///
class GlfImage : public boost::noncopyable {
public:
    /// \class StorageSpec
    ///
    /// Describes the memory layout and storage of a texture image
    ///
    class StorageSpec {
    public:
        StorageSpec()
            : width(0), height(0)
            , format(GL_NONE)
            , type(GL_NONE)
            , flipped(false)
            , data(0) { }

        int width, height, depth;
        GLenum format, type;
        bool flipped;
        void * data;
    };

public:
    GLF_API
    virtual ~GlfImage();

    /// Returns whether \a filename opened as a texture image.
    GLF_API
    static bool IsSupportedImageFile(std::string const & filename);

    /// \name Reading
    /// {@

    /// Opens \a filename for reading from the given \a subimage.
    GLF_API
    static GlfImageSharedPtr OpenForReading(std::string const & filename,
                                            int subimage = 0,
                                            bool suppressErrors = false);

    /// Reads the image file into \a storage.
    virtual bool Read(StorageSpec const & storage) = 0;

    /// Reads the cropped sub-image into \a storage.
    virtual bool ReadCropped(int const cropTop,
                             int const cropBottom,
                             int const cropLeft,
                             int const cropRight,
                             StorageSpec const & storage) = 0;

    /// }@

    /// \name Writing
    /// {@

    /// Opens \a filename for writing from the given \a storage.
    GLF_API
    static GlfImageSharedPtr OpenForWriting(std::string const & filename);

    /// Writes the image with \a metadata.
    virtual bool Write(StorageSpec const & storage,
                       VtDictionary const & metadata = VtDictionary()) = 0;

    /// }@

    /// Returns the image filename.
    virtual std::string const & GetFilename() const = 0;

    /// Returns the image width.
    virtual int GetWidth() const = 0;

    /// Returns the image height.
    virtual int GetHeight() const = 0;

    /// Returns the image format.
    virtual GLenum GetFormat() const = 0;

    /// Returns the image type.
    virtual GLenum GetType() const = 0;

    /// Returns the number of bytes per pixel.
    virtual int GetBytesPerPixel() const = 0;

    /// Returns the number of mips available.
    virtual int GetNumMipLevels() const = 0;

    /// Returns whether the iamge is in the sRGB color space.
    virtual bool IsColorSpaceSRGB() const = 0;

    /// \name Metadata
    /// {@
    template <typename T>
    bool GetMetadata(TfToken const & key, T * value) const;

    virtual bool GetMetadata(TfToken const & key, VtValue * value) const = 0;

    template <typename T>
    bool GetSamplerMetadata(GLenum pname, T * param) const;

    virtual bool GetSamplerMetadata(GLenum pname, VtValue * param) const = 0;

    /// }@

protected:
    virtual bool _OpenForReading(std::string const & filename,
                                 int subimage,
                                 bool suppressErrors) = 0;

    virtual bool _OpenForWriting(std::string const & filename) = 0;
};

template <typename T>
bool
GlfImage::GetMetadata(TfToken const & key, T * value) const
{
    VtValue any;
    if (!GetMetadata(key, &any) || !any.IsHolding<T>()) {
        return false;
    }
    *value = any.UncheckedGet<T>();
    return true;
}

template <typename T>
bool
GlfImage::GetSamplerMetadata(GLenum pname, T * param) const
{
    VtValue any;
    if (!GetSamplerMetadata(pname, &any) || !any.IsHolding<T>()) {
        return false;
    }
    *param = any.UncheckedGet<T>();
    return true;
}

class GlfImageFactoryBase : public TfType::FactoryBase {
public:
    virtual GlfImageSharedPtr New() const = 0;
};

template <class T>
class GlfImageFactory : public GlfImageFactoryBase {
public:
    virtual GlfImageSharedPtr New() const
    {
        return GlfImageSharedPtr(new T);
    }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // GLF_IMAGE_H
