//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_ASSET_H
#define PXR_USD_AR_ASSET_H

/// \file ar/asset.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/ar.h"
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
    virtual size_t GetSize() const = 0;

    /// Returns a pointer to a buffer with the contents of the asset,
    /// with size given by GetSize(). Returns an invalid std::shared_ptr 
    /// if the contents could not be retrieved.
    ///
    /// The data in the returned buffer must remain valid while there are 
    /// outstanding copies of the returned std::shared_ptr. Note that the 
    /// deleter stored in the std::shared_ptr may contain additional data 
    /// needed to maintain the buffer's validity.
    AR_API
    virtual std::shared_ptr<const char> GetBuffer() const = 0;

    /// Read \p count bytes at \p offset from the beginning of the asset
    /// into \p buffer. Returns number of bytes read, or 0 on error.
    ///
    /// Implementers should range-check calls and return zero for out-of-bounds
    /// reads.
    AR_API
    virtual size_t Read(void* buffer, size_t count, size_t offset) const = 0;
        
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
    virtual std::pair<FILE*, size_t> GetFileUnsafe() const = 0;

    /// Returns an ArAsset with the contents of this asset detached from
    /// from this asset's serialized data. External changes to the serialized
    /// data must not have any effect on the ArAsset returned by this function.
    ///
    /// The default implementation returns a new instance of an ArInMemoryAsset
    /// that reads the entire contents of this asset into a heap-allocated
    /// buffer.
    AR_API
    virtual std::shared_ptr<ArAsset> GetDetachedAsset() const;

protected:
    AR_API
    ArAsset();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_ASSET_H
