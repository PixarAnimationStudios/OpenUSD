//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/fastCompression.h"

#include "pxr/base/arch/defines.h"

#include <algorithm>
#include <cstdlib>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

static char values[] = { 'a', 'b', 'c', 'd' };

static bool testRoundTrip(size_t sz)
{
    // Create some data to compress.
    std::unique_ptr<char []> src(new char[sz]);
    for (size_t i = 0; i != sz; ++i) { src[i] = values[(i ^ (i >> 3)) & 3]; }

    // Make a buffer to house compressed data.
    std::unique_ptr<char []> compressed(
        new char[TfFastCompression::GetCompressedBufferSize(sz)]);

    TfErrorMark m;
    
    // Compress.
    size_t compressedSize =
        TfFastCompression::CompressToBuffer(src.get(), compressed.get(), sz);
    printf("Compressed %zu bytes to %zu\n", sz, compressedSize);

    // Decompress.
    std::unique_ptr<char []> decomp(new char[sz]);
    size_t decompressedSize = TfFastCompression::DecompressFromBuffer(
        compressed.get(), decomp.get(), compressedSize, sz);

    printf("Decompressed %zu bytes to %zu\n", compressedSize, decompressedSize);
    TF_AXIOM(sz == decompressedSize);

    // Validate equality.
    TF_AXIOM(std::equal(src.get(), src.get() + sz, decomp.get()));

    return m.IsClean();
}

static bool
Test_TfFastCompression()
{
    size_t sizes[] = {
        3,                   3 + 2,
        3*1024,              3*1024 + 2267,
        3*1024*1024,         3*1024*1024 + 514229,
        7*1024*1024,         7*1024*1024 + 514229,
        2008*1024*1024,      2008*1024*1024 + 514229,
        3*1024*1024*1024ull, 3*1024*1024*1024ull + 178656871
    };

    for (auto sz: sizes) {
        if (!testRoundTrip(sz)) {
            TF_FATAL_ERROR("Failed to (de)compress size %s\n",
                           TfStringify(sz).c_str());
        }
    }
    return true;
}

TF_ADD_REGTEST(TfFastCompression);
