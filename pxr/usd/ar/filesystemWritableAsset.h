//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_USD_AR_FILESYSTEM_WRITABLE_ASSET_H
#define PXR_USD_AR_FILESYSTEM_WRITABLE_ASSET_H

/// \file ar/filesystemWritableAsset.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/writableAsset.h"

#include "pxr/base/tf/safeOutputFile.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArFilesystemWritableAsset
///
/// ArWritableAsset implementation for asset represented by a file on a
/// filesystem.
///
/// This implementation uses TfSafeOutputFile; in the case where the asset
/// has been opened for replacement, data will be written to a temporary 
/// file which will be renamed over the destination file when this object
/// is destroyed. See documentation for TfSafeOutputFile for more details.
class ArFilesystemWritableAsset
    : public ArWritableAsset
{
public:
    /// Constructs a new ArFilesystemWritableAsset for the file at
    /// \p resolvedPath with the given \p writeMode. Returns a null pointer
    /// if the file could not be opened.
    AR_API
    static std::shared_ptr<ArFilesystemWritableAsset> Create(
        const ArResolvedPath& resolvedPath,
        ArResolver::WriteMode writeMode);

    /// Constructs an ArFilesystemWritableAsset for the given \p file.
    /// The ArFilesystemWritableAsset takes ownership of \p file.
    AR_API
    explicit ArFilesystemWritableAsset(TfSafeOutputFile&& file);

    AR_API
    virtual ~ArFilesystemWritableAsset();

    /// Closes the file owned by this asset. If the TfSafeOutputFile was
    /// opened for replacement, the temporary file that was being written
    /// to be will be renamed over the destination file.
    AR_API
    virtual bool Close() override;

    /// Writes \p count bytes from \p buffer at \p offset from the beginning
    /// of the file held by this object. Returns number of bytes written, or
    /// 0 on error.
    AR_API
    virtual size_t Write(
        const void* buffer, size_t count, size_t offset) override;

private:
    TfSafeOutputFile _file;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
