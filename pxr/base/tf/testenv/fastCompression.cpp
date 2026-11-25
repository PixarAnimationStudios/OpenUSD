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
#include <vector>

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

// Decompress each malformed buffer and expect an error and a return of 0.
// Each buffer is copied to exactly-sized heap storage so memory checkers can
// detect reads past its end.
static void
testMalformed()
{
    std::vector<std::vector<unsigned char>> const buffers = {
        // Empty.
        { },
        // Single chunk with a corrupt LZ4 block.
        { 0, 1, 2, 3, 4, 5 },
        // Chunk counts with no chunks following.
        { 128 },
        { 255, 0 },
        // First chunk's size truncated.
        { 2, 0, 0, 0 },
        // Second chunk's size truncated, after a valid one-byte first chunk.
        { 2, 1, 0, 0, 0, 0, 0 },
        // Chunk size larger than the remaining data.
        { 1, 8, 0, 0, 0 },
        // Negative chunk size.
        { 1, 0xff, 0xff, 0xff, 0xff },
    };

    char out[64];
    for (auto const &buf: buffers) {
        std::unique_ptr<char []> compressed(new char[buf.size()]);
        std::copy(buf.begin(), buf.end(), compressed.get());
        TfErrorMark m;
        size_t decompressedSize = TfFastCompression::DecompressFromBuffer(
            compressed.get(), out, buf.size(), sizeof(out));
        TF_AXIOM(decompressedSize == 0);
        TF_AXIOM(!m.IsClean());
        m.Clear();
    }
}

static bool
Test_TfFastCompression()
{
    testMalformed();

    size_t sizes[] = {
        0,
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
