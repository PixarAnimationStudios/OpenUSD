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
#ifndef PXR_IMAGING_HD_ST_TEXTURE_UTILS_H
#define PXR_IMAGING_HD_ST_TEXTURE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hio/types.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hgi/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStTextureUtils
///
/// Helpers for loading textures.
///
class HdStTextureUtils
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
    HDST_API
    static
    HgiFormat GetHgiFormat(
        HioFormat hioFormat,
        bool premultiplyAlpha,
        bool avoidThreeComponentFormats,
        ConversionFunction * conversionFunction);

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
