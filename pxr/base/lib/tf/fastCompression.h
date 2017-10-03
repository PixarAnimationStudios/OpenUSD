//`
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

#ifndef TF_FASTCOMPRESSION_H
#define TF_FASTCOMPRESSION_H

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

#endif // TF_FASTCOMPRESSION_H


