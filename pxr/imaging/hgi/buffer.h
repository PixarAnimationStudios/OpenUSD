//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HGI_BUFFER_H
#define PXR_IMAGING_HGI_BUFFER_H

#include <string>
#include <vector>

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"


PXR_NAMESPACE_OPEN_SCOPE

struct HgiBufferDesc;


///
/// \class HgiBuffer
///
/// Represents a graphics platform independent GPU buffer resource.
/// Buffers should be created via Hgi::CreateBuffer.
/// The fill the buffer with data you supply `initialData` in the descriptor.
/// To update the data in a buffer later, use a blitEncoder.
///
/// Base class for Hgi buffers.
/// To the client (HdSt) buffer resources are referred to via
/// opaque, stateless handles (HgBufferHandle).
///
class HgiBuffer {
public:
    HGI_API
    virtual ~HgiBuffer();

protected:
    HGI_API
    HgiBuffer(HgiBufferDesc const& desc);

private:
    HgiBuffer() = delete;
    HgiBuffer & operator=(const HgiBuffer&) = delete;
    HgiBuffer(const HgiBuffer&) = delete;
};

typedef HgiBuffer* HgiBufferHandle;
typedef std::vector<HgiBufferHandle> HgiBufferHandleVector;


/// \struct HgiBufferDesc
///
/// Describes the properties needed to create a GPU buffer.
///
/// <ul>
/// <li>debugName:
///   This label can be applied as debug label for gpu debugging.</li>
/// <li>usage:
///   Bits describing the intended usage and properties of the buffer.</li>
/// <li>byteSize:
///   Length of buffer in bytes</li>
/// <li>initialData:
///   CPU pointer to initialization data of buffer.
///   The memory is consumed immediately during the creation of the HgiBuffer.
///   The application may alter or free this memory as soon as the constructor
///   of the HgiBuffer has returned.</li>
/// </ul>
///
struct HgiBufferDesc {
    HGI_API
    HgiBufferDesc()
    : usage(HgiBufferUsageUniform)
    , byteSize(0)
    , initialData(nullptr)
    {}

    std::string debugName;
    HgiBufferUsage usage;
    size_t byteSize;
    void const* initialData;
};

HGI_API
bool operator==(
    const HgiBufferDesc& lhs,
    const HgiBufferDesc& rhs);

HGI_API
inline bool operator!=(
    const HgiBufferDesc& lhs,
    const HgiBufferDesc& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
