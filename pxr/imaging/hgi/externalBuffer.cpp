//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/externalBufferArena.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiExternalBuffer::HgiExternalBuffer(
    HgiExternalBufferArena *arena,
    size_t byteSize)
    : _arena(arena)
    , _byteSize(byteSize)
{
}

HgiExternalBuffer::~HgiExternalBuffer() = default;

Hgi *
HgiExternalBuffer::GetHgi() const
{
    return _arena ? _arena->GetHgi() : nullptr;
}

void
HgiExternalBuffer::SetKeepalive(std::shared_ptr<void> keepalive)
{
    _keepalive = std::move(keepalive);
}

std::shared_ptr<void> const &
HgiExternalBuffer::GetKeepalive() const
{
    return _keepalive;
}

PXR_NAMESPACE_CLOSE_SCOPE
