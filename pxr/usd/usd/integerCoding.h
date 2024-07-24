//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_INTEGER_CODING_H
#define PXR_USD_USD_INTEGER_CODING_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"

#include <cstdint>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Usd_IntegerCompression
{
public:
    // Return the max compression buffer size required for \p numInts 32-bit
    // integers.
    USD_API
    static size_t GetCompressedBufferSize(size_t numInts);

    // Return the max decompression working space size required for \p numInts
    // 32-bit integers.
    USD_API
    static size_t GetDecompressionWorkingSpaceSize(size_t numInts);

    // Compress \p numInts ints from \p ints to \p compressed.  The
    // \p compressed space must point to at least
    // GetCompressedBufferSize(numInts) bytes.  Return the actual number
    // of bytes written to \p compressed.
    USD_API
    static size_t CompressToBuffer(
        int32_t const *ints, size_t numInts, char *compressed);

    // Compress \p numInts ints from \p ints to \p compressed.  The
    // \p compressed space must point to at least
    // GetCompressedBufferSize(numInts) bytes.  Return the actual number
    // of bytes written to \p compressed.
    USD_API
    static size_t CompressToBuffer(
        uint32_t const *ints, size_t numInts, char *compressed);

    // Decompress \p compressedSize bytes from \p compressed to produce
    // \p numInts 32-bit integers into \p ints.  Clients may supply
    // \p workingSpace to save allocations if several decompressions will be
    // done but it isn't required.  If supplied it must point to at least
    // GetDecompressionWorkingSpaceSize(numInts) bytes.
    USD_API
    static size_t DecompressFromBuffer(
        char const *compressed, size_t compressedSize,
        int32_t *ints, size_t numInts,
        char *workingSpace=nullptr);

    // Decompress \p compressedSize bytes from \p compressed to produce
    // \p numInts 32-bit integers into \p ints.  Clients may supply
    // \p workingSpace to save allocations if several decompressions will be
    // done but it isn't required.  If supplied it must point to at least
    // GetDecompressionWorkingSpaceSize(numInts) bytes.
    USD_API
    static size_t DecompressFromBuffer(
        char const *compressed, size_t compressedSize,
        uint32_t *ints, size_t numInts,
        char *workingSpace=nullptr);
};

class Usd_IntegerCompression64
{
public:
    // Return the max compression buffer size required for \p numInts 64-bit
    // integers.
    USD_API
    static size_t GetCompressedBufferSize(size_t numInts);

    // Return the max decompression working space size required for \p numInts
    // 64-bit integers.
    USD_API
    static size_t GetDecompressionWorkingSpaceSize(size_t numInts);

    // Compress \p numInts ints from \p ints to \p compressed.  The
    // \p compressed space must point to at least
    // GetCompressedBufferSize(numInts) bytes.  Return the actual number
    // of bytes written to \p compressed.
    USD_API
    static size_t CompressToBuffer(
        int64_t const *ints, size_t numInts, char *compressed);

    // Compress \p numInts ints from \p ints to \p compressed.  The
    // \p compressed space must point to at least
    // GetCompressedBufferSize(numInts) bytes.  Return the actual number
    // of bytes written to \p compressed.
    USD_API
    static size_t CompressToBuffer(
        uint64_t const *ints, size_t numInts, char *compressed);

    // Decompress \p compressedSize bytes from \p compressed to produce
    // \p numInts 64-bit integers into \p ints.  Clients may supply
    // \p workingSpace to save allocations if several decompressions will be
    // done but it isn't required.  If supplied it must point to at least
    // GetDecompressionWorkingSpaceSize(numInts) bytes.
    USD_API
    static size_t DecompressFromBuffer(
        char const *compressed, size_t compressedSize,
        int64_t *ints, size_t numInts,
        char *workingSpace=nullptr);

    // Decompress \p compressedSize bytes from \p compressed to produce
    // \p numInts 64-bit integers into \p ints.  Clients may supply
    // \p workingSpace to save allocations if several decompressions will be
    // done but it isn't required.  If supplied it must point to at least
    // GetDecompressionWorkingSpaceSize(numInts) bytes.
    USD_API
    static size_t DecompressFromBuffer(
        char const *compressed, size_t compressedSize,
        uint64_t *ints, size_t numInts,
        char *workingSpace=nullptr);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INTEGER_CODING_H
