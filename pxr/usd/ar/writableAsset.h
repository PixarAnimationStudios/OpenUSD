//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_WRITABLE_ASSET_H
#define PXR_USD_AR_WRITABLE_ASSET_H

/// \file ar/writableAsset.h

#include "pxr/pxr.h"

#include "pxr/usd/ar/api.h"

#include <cstdio>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArWritableAsset
///
/// Interface for writing data to an asset.
///
/// \see ArResolver::OpenAssetForWrite for how to retrieve instances of
/// this object.
class ArWritableAsset
{
public:
    AR_API
    virtual ~ArWritableAsset();

    ArWritableAsset(const ArWritableAsset&) = delete;
    ArWritableAsset& operator=(const ArWritableAsset&) = delete;

    /// Close this asset, performing any necessary finalization or commits
    /// of data that was previously written. Returns true on success, false
    /// otherwise.
    ///
    /// If successful, reads to the written asset in the same process should
    /// reflect the fully written state by the time this function returns.
    /// Also, further calls to any functions on this interface are invalid.
    virtual bool Close() = 0;

    /// Writes \p count bytes from \p buffer at \p offset from the beginning
    /// of the asset. Returns number of bytes written, or 0 on error.
    virtual size_t Write(const void* buffer, size_t count, size_t offset) = 0;

protected:
    AR_API
    ArWritableAsset();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
