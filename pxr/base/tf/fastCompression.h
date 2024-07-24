//`
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_BASE_TF_FAST_COMPRESSION_H
#define PXR_BASE_TF_FAST_COMPRESSION_H

/// \file tf/fastCompression.h
/// Simple fast data compression/decompression routines.

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

class TfFastCompression
{
public:
    /// Return the largest input buffer size that can be compressed with these
    /// functions.  Guaranteed to be at least 200 GB.
    TF_API static size_t
    GetMaxInputSize();
    
    /// Return the largest possible compressed size for the given \p inputSize
    /// in the worst case (input is not compressible).  This is larger than
    /// \p inputSize.  If inputSize is larger than GetMaxInputSize(), return 0.
    TF_API static size_t
    GetCompressedBufferSize(size_t inputSize);

    /// Compress \p inputSize bytes in \p input and store the result in
    /// \p compressed.  The \p compressed buffer must point to at least
    /// GetCompressedBufferSize(uncompressedSize) bytes.  Return the number of
    /// bytes written to the \p compressed buffer.  Issue a runtime error and
    /// return ~0 in case of an error.
    TF_API static size_t
    CompressToBuffer(char const *input, char *compressed, size_t inputSize);
                           
    /// Decompress \p compressedSize bytes in \p compressed and store the
    /// result in \p output.  No more than \p maxOutputSize bytes will be
    /// written to \p output.
    TF_API static size_t
    DecompressFromBuffer(char const *compressed, char *output,
                         size_t compressedSize, size_t maxOutputSize);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_FAST_COMPRESSION_H


