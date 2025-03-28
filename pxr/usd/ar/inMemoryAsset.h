//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_USD_AR_IN_MEMORY_ASSET_H
#define PXR_USD_AR_IN_MEMORY_ASSET_H

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/asset.h"

#include <cstdio>
#include <memory>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArInMemoryAsset
///
/// ArAsset implementation that stores asset content in a heap-allocated
/// buffer managed by this object.
class ArInMemoryAsset
    : public ArAsset
{
public:
    /// Constructs a new instance containing the entire contents of \p srcAsset.
    ///
    /// Returns nullptr if a buffer large enough to hold \p srcAsset's contents
    /// cannot be allocated or if an error occurs when reading \p srcAsset's
    /// contents into the buffer.
    AR_API
    static std::shared_ptr<ArInMemoryAsset> FromAsset(
        const ArAsset& srcAsset);

    /// Constructs a new instance sharing ownership of the given \p buffer
    /// containing \p bufferSize bytes.
    AR_API
    static std::shared_ptr<ArInMemoryAsset> FromBuffer(
        const std::shared_ptr<const char>& buffer,
        size_t bufferSize);

    /// Constructs a new instance taking ownership of the given \p buffer
    /// containing \p bufferSize bytes.
    AR_API
    static std::shared_ptr<ArInMemoryAsset> FromBuffer(
        std::shared_ptr<const char>&& buffer,
        size_t bufferSize);

    /// Destructor. Note that this may not destroy the associated buffer if
    /// a client is holding on to the result of GetBuffer().
    AR_API
    ~ArInMemoryAsset();

    /// Returns the size of the buffer managed by this object.
    AR_API
    size_t GetSize() const override;

    /// Returns the buffer managed by this object.
    AR_API
    std::shared_ptr<const char> GetBuffer() const override;

    /// Reads \p count bytes from the buffer held by this object at the
    /// given \p offset into \p buffer.
    AR_API
    size_t Read(void* buffer, size_t count, size_t offset) const override;

    /// Returns { nullptr, 0 } as this object is not associated with a file.
    AR_API
    std::pair<FILE*, size_t> GetFileUnsafe() const override;

    /// Returns a new ArInMemoryAsset instance that shares the same buffer
    /// as this object.
    AR_API
    std::shared_ptr<ArAsset> GetDetachedAsset() const override;

private:
    struct PrivateCtorTag {};
public:
    // "Private" c'tor. Must actually be public for std::make_shared,
    // but the PrivateCtorTag prevents other code from using this.
    template <class BufferSharedPtr>
    ArInMemoryAsset(
        BufferSharedPtr&& buffer,
        size_t bufferSize,
        PrivateCtorTag);

private:
    std::shared_ptr<const char> _buffer;
    size_t _bufferSize;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
