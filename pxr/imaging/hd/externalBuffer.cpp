//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/externalBuffer.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

std::ostream &
operator<<(std::ostream &out, HdExternalBufferPtr const &ptr)
{
    // Deliberately says nothing about the buffer itself: hd has only a
    // forward declaration, and the useful facts (size, backend, native
    // handle) belong to whoever can dereference it.
    if (ptr.buffer.expired()) {
        return out << "HdExternalBufferPtr(expired)";
    }
    return out << "HdExternalBufferPtr(live)";
}

PXR_NAMESPACE_CLOSE_SCOPE
