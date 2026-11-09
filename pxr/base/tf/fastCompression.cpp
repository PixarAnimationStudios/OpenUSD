//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fastCompression.h"

// XXX: Need to isolate symbols here?
#include "pxrLZ4/lz4.h"

#include <algorithm>
#include <cstring>

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
            LZ4_compressBound(inputSize));
    } else {
        size_t nWholeChunks = inputSize / LZ4_MAX_INPUT_SIZE;
        size_t partChunkSz = inputSize % LZ4_MAX_INPUT_SIZE;
        *compressed++ = nWholeChunks + (partChunkSz ? 1 : 0);
        auto writeChunk = [](char const *&input, char *&output, size_t size) {
            char *o = output;
            output += sizeof(int32_t);
            int32_t n = LZ4_compress_default(
                input, output, size, LZ4_compressBound(size));
            std::memcpy(o, &n, sizeof(n));
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
    if (compressedSize == 0) {
        TF_RUNTIME_ERROR("Cannot decompress an empty buffer");
        return 0;
    }
    char const * const end = compressed + compressedSize;

    // Check first byte for # chunks.  Read it unsigned so values above 127 are
    // not negative where char is signed.
    unsigned int nChunks = static_cast<unsigned char>(*compressed++);

    if (nChunks == 0) {
        // Just one.  LZ4 takes int sizes; no valid chunk is larger than
        // LZ4_MAX_INPUT_SIZE compressed or uncompressed.
        if (end - compressed > LZ4_compressBound(LZ4_MAX_INPUT_SIZE)) {
            TF_RUNTIME_ERROR("Failed to decompress data: compressed size %zu "
                             "is too large", compressedSize);
            return 0;
        }
        int nDecompressed = LZ4_decompress_safe(
            compressed, output, end - compressed,
            std::min<size_t>(LZ4_MAX_INPUT_SIZE, maxOutputSize));
        if (nDecompressed < 0) {
            TF_RUNTIME_ERROR("Failed to decompress data, possibly corrupt? "
                             "LZ4 error code: %d", nDecompressed);
            return 0;
        }
        return nDecompressed;
    } else {
        // Do each chunk.
        size_t totalDecompressed = 0;
        for (unsigned int i = 0; i != nChunks; ++i) {
            int32_t chunkSize = 0;
            if (end - compressed < ptrdiff_t(sizeof(chunkSize))) {
                TF_RUNTIME_ERROR("Failed to decompress data: chunk %u of %u "
                                 "truncated", i + 1, nChunks);
                return 0;
            }
            std::memcpy(&chunkSize, compressed, sizeof(chunkSize));
            compressed += sizeof(chunkSize);
            if (chunkSize < 0 || chunkSize > end - compressed) {
                TF_RUNTIME_ERROR("Failed to decompress data: chunk %u of %u "
                                 "has invalid size %d with %td bytes "
                                 "remaining", i + 1, nChunks, chunkSize,
                                 end - compressed);
                return 0;
            }
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
}

PXR_NAMESPACE_CLOSE_SCOPE
