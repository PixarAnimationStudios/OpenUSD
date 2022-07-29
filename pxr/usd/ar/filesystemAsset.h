//
// Copyright 2021 Pixar
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
#ifndef PXR_USD_AR_FILESYSTEM_ASSET_H
#define PXR_USD_AR_FILESYSTEM_ASSET_H

/// \file ar/filesystemAsset.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/timestamp.h"

#include <cstdio>
#include <memory>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class ArResolvedPath;

/// \class ArFilesystemAsset
///
/// ArAsset implementation for asset represented by a file on a filesystem.
class ArFilesystemAsset
    : public ArAsset
{
public:
    /// Constructs a new ArFilesystemAsset for the file at \p resolvedPath.
    /// Returns a null pointer if the file could not be opened.
    AR_API
    static std::shared_ptr<ArFilesystemAsset> Open(
        const ArResolvedPath& resolvedPath);

    /// Returns an ArTimestamp holding the mtime of the file at \p resolvedPath.
    /// Returns an invalid ArTimestamp if the mtime could not be retrieved.
    AR_API
    static ArTimestamp GetModificationTimestamp(
        const ArResolvedPath& resolvedPath);

    /// Constructs an ArFilesystemAsset for the given \p file. 
    /// The ArFilesystemAsset object takes ownership of \p file and will
    /// close the file handle on destruction.
    AR_API
    explicit ArFilesystemAsset(FILE* file);

    /// Closes the file owned by this object.
    AR_API
    ~ArFilesystemAsset();

    /// Returns the size of the file held by this object.
    AR_API
    virtual size_t GetSize() const override;

    /// Creates a read-only memory map for the file held by this object
    /// and returns a pointer to the start of the mapped contents.
    AR_API
    virtual std::shared_ptr<const char> GetBuffer() const override;
    
    /// Reads \p count bytes from the file held by this object at the
    /// given \p offset into \p buffer.
    AR_API
    virtual size_t Read(
        void* buffer, size_t count, size_t offset) const override;

    /// Returns the FILE* handle this object was created with and an offset
    /// of 0, since the asset's contents are located at the beginning of the
    /// file.
    AR_API        
    virtual std::pair<FILE*, size_t> GetFileUnsafe() const override;

private:
    FILE* _file;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_FILESYSTEM_ASSET_H
