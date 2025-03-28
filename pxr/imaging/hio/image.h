//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_IMAGE_H
#define PXR_IMAGING_HIO_IMAGE_H

/// \file hio/image.h

#include "pxr/pxr.h"
#include "pxr/imaging/hio/api.h"
#include "pxr/imaging/hio/types.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


using HioImageSharedPtr = std::shared_ptr<class HioImage>;

/// \class HioImage
///
/// A base class for reading and writing texture image data.
///
/// The class allows basic access to texture image file data.
///
/// Texture paths are UTF-8 strings, resolvable by AR. Texture system dispatch
/// is driven by extension, with [A-Z] (and no other characters) case folded.
class HioImage
{
public:

    /// Specifies whether to treat the image origin as the upper-left corner
    /// or the lower left
    enum ImageOriginLocation
    {
        OriginUpperLeft, 
        OriginLowerLeft
    }; 

    /// Specifies the source color space in which the texture is encoded, with 
    /// "Auto" indicating the texture reader should determine color space based 
    /// on hints from the image (e.g. file type, number of channels, image 
    /// metadata)
    enum SourceColorSpace
    {
        Raw, 
        SRGB,
        Auto
    }; 
   
    /// \class StorageSpec
    ///
    /// Describes the memory layout and storage of a texture image
    ///
    class StorageSpec
    {
    public:
        StorageSpec()
            : width(0), height(0), depth(0)
            , format(HioFormatInvalid)
            , flipped(false)
            , data(0) { }

        int width, height, depth;
        HioFormat format;
        bool flipped;
        void * data;
    };

public:
    HioImage() = default;

    HIO_API
    virtual ~HioImage();

    // Disallow copies
    HioImage(const HioImage&) = delete;
    HioImage& operator=(const HioImage&) = delete;

    /// Returns whether \a filename opened as a texture image.
    HIO_API
    static bool IsSupportedImageFile(std::string const & filename);

    /// \name Reading
    /// {@

    /// Opens \a filename for reading from the given \a subimage at mip level
    /// \a mip, using \a sourceColorSpace to help determine the color space
    /// with which to interpret the texture
    HIO_API
    static HioImageSharedPtr OpenForReading(std::string const & filename,
                                            int subimage = 0,
                                            int mip = 0,
                                            SourceColorSpace sourceColorSpace = 
                                                SourceColorSpace::Auto,
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
    HIO_API
    static HioImageSharedPtr OpenForWriting(std::string const & filename);

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

    /// Returns the destination HioFormat.
    virtual HioFormat GetFormat() const = 0;

    /// Returns the number of bytes per pixel.
    virtual int GetBytesPerPixel() const = 0;

    /// Returns the number of mips available.
    virtual int GetNumMipLevels() const = 0;

    /// Returns whether the image is in the sRGB color space.
    virtual bool IsColorSpaceSRGB() const = 0;

    /// \name Metadata
    /// {@
    template <typename T>
    bool GetMetadata(TfToken const & key, T * value) const;

    virtual bool GetMetadata(TfToken const & key, VtValue * value) const = 0;

    virtual bool GetSamplerMetadata(HioAddressDimension dim,
                                    HioAddressMode * param) const = 0;

    /// }@

protected:
    virtual bool _OpenForReading(std::string const & filename,
                                 int subimage,
                                 int mip,
                                 SourceColorSpace sourceColorSpace,
                                 bool suppressErrors) = 0;

    virtual bool _OpenForWriting(std::string const & filename) = 0;
};

template <typename T>
bool
HioImage::GetMetadata(TfToken const & key, T * value) const
{
    VtValue any;
    if (!GetMetadata(key, &any) || !any.IsHolding<T>()) {
        return false;
    }
    *value = any.UncheckedGet<T>();
    return true;
}

class HIO_API HioImageFactoryBase : public TfType::FactoryBase {
public:
    virtual HioImageSharedPtr New() const = 0;
};

template <class T>
class HioImageFactory : public HioImageFactoryBase {
public:
    virtual HioImageSharedPtr New() const
    {
        return HioImageSharedPtr(new T);
    }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HIO_IMAGE_H
