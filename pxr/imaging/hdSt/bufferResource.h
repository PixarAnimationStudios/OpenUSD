//
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
//
#ifndef PXR_IMAGING_HD_ST_BUFFER_RESOURCE_H
#define PXR_IMAGING_HD_ST_BUFFER_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hgi/buffer.h"

#include "pxr/base/tf/token.h"

#include <memory>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using HdStBufferResourceSharedPtr =
    std::shared_ptr<class HdStBufferResource>;

using HdStBufferResourceNamedPair =
    std::pair<TfToken, HdStBufferResourceSharedPtr>;
using HdStBufferResourceNamedList =
    std::vector<HdStBufferResourceNamedPair>;

/// \class HdStBufferResource
///
/// A GPU resource contained within an underlying HgiBuffer.
///
class HdStBufferResource final
{
public:
    HDST_API
    HdStBufferResource(TfToken const &role,
                       HdTupleType tupleType,
                       int offset,
                       int stride);

    HDST_API
    ~HdStBufferResource();

    /// Returns the role of the data in this resource.
    TfToken const &GetRole() const { return _role; }

    /// Returns the size (in bytes) of the data.
    size_t GetSize() const { return _size; }

    /// Data type and count
    HdTupleType GetTupleType() const { return _tupleType; }

    /// Returns the interleaved offset (in bytes) of the data.
    int GetOffset() const { return _offset; }

    /// Returns the stride (in bytes) between data elements.
    int GetStride() const { return _stride; }

    /// Sets the HgiBufferHandle for this resource and its size.
    HDST_API
    void SetAllocation(HgiBufferHandle const &handle, size_t size);

    /// Returns the HgiBufferHandle for this GPU resource.
    HgiBufferHandle &GetHandle() { return _handle; }

private:
    HdStBufferResource(const HdStBufferResource &) = delete;
    HdStBufferResource &operator=(const HdStBufferResource &) = delete;

    HgiBufferHandle _handle;
    size_t _size;

    TfToken const _role;
    HdTupleType const _tupleType;
    int const _offset;
    int const _stride;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_BUFFER_RESOURCE_H
