//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/usd/ar/inMemoryAsset.h"

#include "pxr/base/tf/diagnostic.h"

#include <cstring>
#include <new>

PXR_NAMESPACE_OPEN_SCOPE

std::shared_ptr<ArInMemoryAsset>
ArInMemoryAsset::FromBuffer(
    const std::shared_ptr<const char>& buffer, 
    size_t bufferSize)
{
    return std::make_shared<ArInMemoryAsset>(
        buffer, bufferSize, PrivateCtorTag());
}

std::shared_ptr<ArInMemoryAsset>
ArInMemoryAsset::FromBuffer(
    std::shared_ptr<const char>&& buffer, 
    size_t bufferSize)
{
    return std::make_shared<ArInMemoryAsset>(
        std::move(buffer), bufferSize, PrivateCtorTag());
}

template <class BufferSharedPtr>
ArInMemoryAsset::ArInMemoryAsset(
    BufferSharedPtr&& buffer,
    size_t bufferSize,
    PrivateCtorTag)
    : _buffer(std::forward<BufferSharedPtr>(buffer))
    , _bufferSize(bufferSize)
{
}

ArInMemoryAsset::~ArInMemoryAsset() = default;

std::shared_ptr<ArInMemoryAsset>
ArInMemoryAsset::FromAsset(
    const ArAsset& srcAsset)
{
    const size_t bufferSize = srcAsset.GetSize();
    std::shared_ptr<char> buffer;
    try {
        buffer.reset(new char[bufferSize], std::default_delete<char[]>());
    } 
    catch (const std::bad_alloc&) {
        TF_RUNTIME_ERROR(
            "Failed to allocate buffer of %zu bytes for asset.", bufferSize);
        return nullptr;
    }

    const size_t bytesRead = srcAsset.Read(buffer.get(), bufferSize, 0);
    if (bytesRead != bufferSize) {
        TF_RUNTIME_ERROR(
            "Failed to read asset into memory. Expected %zu bytes, read %zu.",
            bufferSize, bytesRead);
        return nullptr;
    }
     
    return FromBuffer(std::move(buffer), bufferSize);
}

size_t
ArInMemoryAsset::GetSize() const
{
    return _bufferSize;
}

std::shared_ptr<const char>
ArInMemoryAsset::GetBuffer() const
{
    return _buffer;
}

size_t
ArInMemoryAsset::Read(
    void* buffer, size_t count, size_t offset) const
{
    if (offset + count > _bufferSize) {
        return 0;
    }

    memcpy(buffer, _buffer.get() + offset, count);
    return count;
}

std::pair<FILE*, size_t>
ArInMemoryAsset::GetFileUnsafe() const
{
    return { nullptr, 0 };
}

std::shared_ptr<ArAsset>
ArInMemoryAsset::GetDetachedAsset() const
{
    return FromBuffer(_buffer, _bufferSize);
}

PXR_NAMESPACE_CLOSE_SCOPE
