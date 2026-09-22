//
// Copyright 2026 Contributors to the OpenUSD Project
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDST_PTEX_MIPMAP_TEXTURE_LOADER_SIZING_H
#define PXR_IMAGING_HDST_PTEX_MIPMAP_TEXTURE_LOADER_SIZING_H

#include "pxr/pxr.h"

#include <cstddef>
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

enum class HdStPtexBufferSizeResult
{
    Success,
    InvalidDimension,
    ArithmeticOverflow,
    ExceedsSupportedSize
};

inline const char *
HdStGetPtexBufferSizeResultName(HdStPtexBufferSizeResult result)
{
    switch (result) {
    case HdStPtexBufferSizeResult::Success:
        return "success";
    case HdStPtexBufferSizeResult::InvalidDimension:
        return "invalid dimension";
    case HdStPtexBufferSizeResult::ArithmeticOverflow:
        return "arithmetic overflow";
    case HdStPtexBufferSizeResult::ExceedsSupportedSize:
        return "exceeds signed-32-bit texel-buffer support boundary";
    }
    return "unknown";
}

inline bool
HdStCheckedMultiplySize(size_t lhs, size_t rhs, size_t *result)
{
    if (rhs != 0 && lhs > std::numeric_limits<size_t>::max() / rhs) {
        return false;
    }
    *result = lhs * rhs;
    return true;
}

inline HdStPtexBufferSizeResult
HdStComputePtexBufferSize(
    int bytesPerPixel,
    int pageWidth,
    int pageHeight,
    size_t numPages,
    size_t *pageStride,
    size_t *totalSize)
{
    *pageStride = 0;
    *totalSize = 0;
    if (bytesPerPixel <= 0 || pageWidth <= 0 ||
        pageHeight <= 0 || numPages == 0) {
        return HdStPtexBufferSizeResult::InvalidDimension;
    }

    if (!HdStCheckedMultiplySize(
            static_cast<size_t>(bytesPerPixel),
            static_cast<size_t>(pageWidth), pageStride) ||
        !HdStCheckedMultiplySize(
            *pageStride, static_cast<size_t>(pageHeight), pageStride) ||
        !HdStCheckedMultiplySize(*pageStride, numPages, totalSize)) {
        return HdStPtexBufferSizeResult::ArithmeticOverflow;
    }

    if (*totalSize >
        static_cast<size_t>(std::numeric_limits<int>::max())) {
        return HdStPtexBufferSizeResult::ExceedsSupportedSize;
    }
    return HdStPtexBufferSizeResult::Success;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
