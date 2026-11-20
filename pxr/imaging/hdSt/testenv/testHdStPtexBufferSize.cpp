//
// Copyright 2026 Contributors to the OpenUSD Project
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/ptexMipmapTextureLoaderSizing.h"

#include "pxr/base/tf/diagnostic.h"

#include <cstddef>
#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

static void
_Check(
    int bytesPerPixel,
    int width,
    int height,
    size_t pages,
    HdStPtexBufferSizeResult expectedResult,
    size_t expectedStride,
    size_t expectedTotal)
{
    size_t stride = 0;
    size_t total = 0;
    const HdStPtexBufferSizeResult result =
        HdStComputePtexBufferSize(
            bytesPerPixel, width, height, pages, &stride, &total);
    TF_AXIOM(result == expectedResult);
    TF_AXIOM(stride == expectedStride);
    TF_AXIOM(total == expectedTotal);
}

int
main()
{
    _Check(
        4, 1024, 1024, 2,
        HdStPtexBufferSizeResult::Success,
        4194304, 8388608);

    // Captured /island/isBeach values.
    _Check(
        3, 772, 514, 2741,
        HdStPtexBufferSizeResult::ExceedsSupportedSize,
        1190424, 3262952184ULL);

    // Captured /island/isDunesB values. The old signed-int expression
    // wrapped this total to 302846656, the exact GDB _memoryUsage value.
    _Check(
        3, 12292, 8194, 200,
        HdStPtexBufferSizeResult::ExceedsSupportedSize,
        302161944, 60432388800ULL);

    size_t result = 0;
    TF_AXIOM(!HdStCheckedMultiplySize(
        std::numeric_limits<size_t>::max(), 2, &result));
}
