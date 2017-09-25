//
// Copyright 2017 Pixar
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

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fastCompression.h"

// XXX: Need to isolate symbols here?
#include "pxrLZ4/lz4.h"

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_lz4;

size_t
TfFastCompression::GetMaxInputSize()
{
    return 127 * static_cast<size_t>(LZ4_MAX_INPUT_SIZE);
}

size_t
TfFastCompression::GetCompressedBufferSize(size_t inputSize)
{
    if (inputSize > GetMaxInputSize())
        return 0;
    
    // If it fits in one chunk then it's just the compress bound plus 1.
    if (inputSize <= LZ4_MAX_INPUT_SIZE) {
        return LZ4_compressBound(inputSize) + 1;
    }
    size_t nWholeChunks = inputSize / LZ4_MAX_INPUT_SIZE;
    size_t partChunkSz = inputSize % LZ4_MAX_INPUT_SIZE;
    size_t sz = 1 + nWholeChunks *
        (LZ4_compressBound(LZ4_MAX_INPUT_SIZE) + sizeof(int32_t));
    if (partChunkSz)
        sz += LZ4_compressBound(partChunkSz) + sizeof(int32_t);
    return sz;
}

size_t
TfFastCompression::CompressToBuffer(
    char const *input, char *compressed, size_t inputSize)
{
    if (inputSize > GetMaxInputSize()) {
        TF_CODING_ERROR("Attempted to compress a buffer of %zu bytes, "
                        "more than the maximum supported %zu",
                        inputSize, GetMaxInputSize());
        return 0;
    }
    
    // If it fits in one chunk, just do it.
    char const * const origCompressed = compressed;
    if (inputSize <= LZ4_MAX_INPUT_SIZE) {
        compressed[0] = 0; // < zero byte means one chunk.
        compressed += 1 + LZ4_compress_default(
            input, compressed + 1, inputSize,
            GetCompressedBufferSize(inputSize));
    } else {
        size_t nWholeChunks = inputSize / LZ4_MAX_INPUT_SIZE;
        size_t partChunkSz = inputSize % LZ4_MAX_INPUT_SIZE;
        *compressed++ = nWholeChunks + (partChunkSz ? 1 : 0);
        auto writeChunk = [](char const *&input, char *&output, size_t size) {
            char *o = output;
            output += sizeof(int32_t);
            int32_t n = LZ4_compress_default(
                input, output, size, LZ4_compressBound(size));
            memcpy(o, &n, sizeof(n));
            output += n;
            input += size;
        };
        for (size_t chunk = 0; chunk != nWholeChunks; ++chunk) {
            writeChunk(input, compressed, LZ4_MAX_INPUT_SIZE);
        }
        if (partChunkSz) {
            writeChunk(input, compressed, partChunkSz);
        }
    }

    return compressed - origCompressed;
}    

size_t
TfFastCompression::DecompressFromBuffer(
    char const *compressed, char *output,
    size_t compressedSize, size_t maxOutputSize)
{
    // Check first byte for # chunks.
    int nChunks = *compressed++;
    if (nChunks == 0) {
        // Just one.
        int nDecompressed = LZ4_decompress_safe(
            compressed, output, compressedSize-1, maxOutputSize);
        if (nDecompressed < 0) {
            TF_RUNTIME_ERROR("Failed to decompress data, possibly corrupt? "
                             "LZ4 error code: %d", nDecompressed);
            return 0;
        }
        return nDecompressed;
    } else {
        // Do each chunk.
        size_t totalDecompressed = 0;
        for (int i = 0; i != nChunks; ++i) {
            int32_t chunkSize = 0;
            memcpy(&chunkSize, compressed, sizeof(chunkSize));
            compressed += sizeof(chunkSize);
            int nDecompressed = LZ4_decompress_safe(
                compressed, output, chunkSize,
                std::min<size_t>(LZ4_MAX_INPUT_SIZE, maxOutputSize));
            if (nDecompressed < 0) {
                TF_RUNTIME_ERROR("Failed to decompress data, possibly corrupt? "
                                 "LZ4 error code: %d", nDecompressed);
                return 0;
            }
            compressed += chunkSize;
            output += nDecompressed;
            maxOutputSize -= nDecompressed;
            totalDecompressed += nDecompressed;
        }
        return totalDecompressed;
    }
    // unreachable.
}                

PXR_NAMESPACE_CLOSE_SCOPE
