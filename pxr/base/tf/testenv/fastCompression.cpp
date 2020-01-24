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
