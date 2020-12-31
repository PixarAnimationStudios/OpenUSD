//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_TEXTURE_UTILS_H
#define PXR_IMAGING_LOFI_TEXTURE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/hio/types.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hgi/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// \class LoFiTextureUtils
///
/// Helpers for loading textures.
///
class LoFiTextureUtils
{
public:
    /// Converts given number of texels. src and dst are pointers to
    /// the source and destination buffer which can be equal for conversion
    /// in place.
    using ConversionFunction =
        void(*)(const void * src,
                size_t numTexels,
                void * dst);

    /// Get the Hgi format suitable for a given Hio format. Also
    /// return the conversion function if necessary.
    ///
    /// Premultiply alpha indicates whether a conversion function
    /// multiplying RGB with alpha should be created.
    /// If avoidThreeComponentFormats is true, never return a type
    /// with three components.
    LOFI_API
    static
    HgiFormat GetHgiFormat(
        HioFormat hioFormat,
        bool premultiplyAlpha,
        bool avoidThreeComponentFormats,
        ConversionFunction * conversionFunction);

    /// Get all mip levels from a file.
    LOFI_API
    static
    std::vector<HioImageSharedPtr> GetAllMipImages(
        const std::string &filePath,
        HioImage::SourceColorSpace sourceColorSpace);

    // Compute dimensions so that all tiles fit into the given target memory.
    // First by traversing the given images and then by computing a mip chain
    // starting with the lowest resolution image.
    // Optionally, can also give the index of the image in mips that was used
    // to compute the dimensions.
    LOFI_API
    static
    GfVec3i
    ComputeDimensionsFromTargetMemory(
        const std::vector<HioImageSharedPtr> &mips,
        HgiFormat targetFormat,
        size_t tileCount,
        size_t targetMemory,
        size_t * mipIndex = nullptr);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
