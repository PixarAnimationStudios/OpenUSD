//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_TEXTURE_UTILS_H
#define PXR_IMAGING_HD_ST_TEXTURE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hio/image.h"

#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/types.h"

#include "pxr/base/arch/align.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
using HgiTextureHandle = HgiHandle<class HgiTexture>;

/// \class HdStTextureUtils
///
/// Helpers for loading textures.
///
class HdStTextureUtils
{
public:
    /// Converts given number of texels.
    ///
    /// Conversion can be in-place if the ends of the source and destination
    /// buffers align.
    using ConversionFunction =
        void(*)(const void * src,
                size_t numTexels,
                void * dst);

    /// Get the Hgi format suitable for a given Hio format.
    ///
    /// Premultiply alpha indicates whether a conversion function
    /// multiplying RGB with alpha should be created.
    HDST_API
    static
    HgiFormat GetHgiFormat(
        HioFormat hioFormat,
        bool premultiplyAlpha);

    /// Returns the conversion function to return a HioFormat
    /// to the corresponding HgiFormat given by GetHgiFormat.
    ///
    /// Returns nullptr if no conversion necessary.
    HDST_API
    static
    ConversionFunction GetHioToHgiConversion(
        HioFormat hioFormat,
        bool premultiplyAlpha);

    /// Get all mip levels from a file.
    HDST_API
    static
    std::vector<HioImageSharedPtr> GetAllMipImages(
        const std::string &filePath,
        HioImage::SourceColorSpace sourceColorSpace);

    // Compute dimensions so that all tiles fit into the given target memory.
    // First by traversing the given images and then by computing a mip chain
    // starting with the lowest resolution image.
    // Optionally, can also give the index of the image in mips that was used
    // to compute the dimensions.
    HDST_API
    static
    GfVec3i
    ComputeDimensionsFromTargetMemory(
        const std::vector<HioImageSharedPtr> &mips,
        HgiFormat targetFormat,
        size_t tileCount,
        size_t targetMemory,
        size_t * mipIndex = nullptr);

    // Read given HioImage and convert it to corresponding Hgi format.
    // Returns false if reading the HioImage failed.
    //
    // bufferStart is assumed to point at the beginning of a mip chain
    // with mipInfo describing what mip level of the mip chain to be
    // filled. layer gives the layer number if the mip chain is for an
    // array texture.
    HDST_API
    static
    bool
    ReadAndConvertImage(
        HioImageSharedPtr const &image,
        bool flipped,
        bool premultiplyAlpha,
        const HgiMipInfo &mipInfo,
        size_t layer,
        void * bufferStart);

    // Because the underlying graphics API may have alignment
    // restrictions we use this wrapper class to manage the
    // allocation of the returned buffer data, and expose a
    // restricted subset of underlyling pointer's access methods.
    template <typename T>
    class AlignedBuffer
    {
    public:
        AlignedBuffer()
            : AlignedBuffer(nullptr)
            { }

        T *get() const {
            return _alignedPtr.get();
        }

    private:
        friend class HdStTextureUtils;

        explicit AlignedBuffer(T * alignedPtr)
            : _alignedPtr(alignedPtr, ArchAlignedFree)
            { }

        T *release() {
            return _alignedPtr.release();
        }

        std::unique_ptr<T[], decltype(ArchAlignedFree)*> _alignedPtr;
    };

    /// Returns an unsigned byte buffer with data read back from \p texture.
    HDST_API
    static
    AlignedBuffer<uint8_t>
    HgiTextureReadback(Hgi * const hgi,
                       HgiTextureHandle const & texture,
                       size_t * bufferSize);

    /// Returns a buffer with data of type T read back from \p texture.
    template <typename T>
    static
    AlignedBuffer<T>
    HgiTextureReadback(Hgi * const hgi,
                       HgiTextureHandle const & texture,
                       size_t * bufferSize);
};

template <typename T>
HdStTextureUtils::AlignedBuffer<T>
HdStTextureUtils::HgiTextureReadback(Hgi * const hgi,
                                     HgiTextureHandle const & texture,
                                     size_t * bufferSize)
{
    HdStTextureUtils::AlignedBuffer<uint8_t> buffer =
        HdStTextureUtils::HgiTextureReadback(hgi, texture, bufferSize);

    T * typedData = reinterpret_cast<T *>(buffer.release());
    return HdStTextureUtils::AlignedBuffer<T>(typedData);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
