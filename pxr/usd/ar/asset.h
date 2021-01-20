//
// Copyright 2018 Pixar
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
#ifndef PXR_USD_AR_ASSET_H
#define PXR_USD_AR_ASSET_H

/// \file ar/asset.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"

#include <cstdio>
#include <memory>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArAsset
///
/// Interface for accessing the contents of an asset.
///
/// \see ArResolver::OpenAsset for how to retrieve instances of this object.
class ArAsset {
public:
    AR_API
    virtual ~ArAsset();

    ArAsset(const ArAsset&) = delete;

    ArAsset& operator=(const ArAsset&) = delete;

    /// Returns size of the asset.
    AR_API
    virtual size_t GetSize() = 0;

    /// Returns a pointer to a buffer with the contents of the asset,
    /// with size given by GetSize(). Returns an invalid std::shared_ptr 
    /// if the contents could not be retrieved.
    ///
    /// The data in the returned buffer must remain valid while there are 
    /// outstanding copies of the returned std::shared_ptr. Note that the 
    /// deleter stored in the std::shared_ptr may contain additional data 
    /// needed to maintain the buffer's validity.
    AR_API
    virtual std::shared_ptr<const char> GetBuffer() = 0;

    /// Read \p count bytes at \p offset from the beginning of the asset
    /// into \p buffer. Returns number of bytes read, or 0 on error.
    ///
    /// Implementers should range-check calls and return zero for out-of-bounds
    /// reads.
    AR_API
    virtual size_t Read(void* buffer, size_t count, size_t offset) = 0;
        
    /// Returns a read-only FILE* handle and offset for this asset if
    /// available, or (nullptr, 0) otherwise.
    ///
    /// The returned handle must remain valid for the lifetime of this 
    /// ArAsset object. The returned offset is the offset from the beginning 
    /// of the FILE* where the asset's contents begins.
    ///
    /// This function is marked unsafe because the handle may wind up
    /// being used in multiple threads depending on the underlying
    /// resolver implementation. For instance, a resolver may cache
    /// and return ArAsset objects with the same FILE* to multiple 
    /// threads.
    ///
    /// Clients MUST NOT use this handle with functions that cannot be
    /// called concurrently on the same file descriptor, e.g. read, 
    /// fread, fseek, etc. See ArchPRead for a function that can be used
    /// to read data from this handle safely.
    AR_API
    virtual std::pair<FILE*, size_t> GetFileUnsafe() = 0;

protected:
    AR_API
    ArAsset();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_ASSET_H
