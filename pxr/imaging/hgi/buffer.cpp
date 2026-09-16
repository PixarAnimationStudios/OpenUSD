//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiBuffer::HgiBuffer(HgiBufferDesc const& desc)
    : _descriptor(desc)
{
}

HgiBuffer::~HgiBuffer() = default;

HgiBufferDesc const&
HgiBuffer::GetDescriptor() const
{
    return _descriptor;
}

void
HgiBuffer::SetKeepalive(std::shared_ptr<void> const& keepalive)
{
    _keepalive = keepalive;
}

std::shared_ptr<void> const&
HgiBuffer::GetKeepalive() const
{
    return _keepalive;
}

bool operator==(
    const HgiBufferDesc& lhs,
    const HgiBufferDesc& rhs)
{
    return lhs.debugName == rhs.debugName &&
           lhs.usage == rhs.usage &&
           lhs.byteSize == rhs.byteSize
           // Omitted. Only used tmp during creation of buffer.
           // lhs.initialData == rhs.initialData &&
    ;
}

bool operator!=(
    const HgiBufferDesc& lhs,
    const HgiBufferDesc& rhs)
{
    return !(lhs == rhs);
}



PXR_NAMESPACE_CLOSE_SCOPE
