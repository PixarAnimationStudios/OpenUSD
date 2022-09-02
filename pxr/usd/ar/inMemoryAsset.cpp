//
// Copyright 2022 Pixar
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
