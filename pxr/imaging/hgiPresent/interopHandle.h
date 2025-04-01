//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_INTEROPHANDLE_H
#define PXR_IMAGING_HGIPRESENT_INTEROPHANDLE_H

#include "pxr/pxr.h"

#include "pxr/base/arch/defines.h"

#include <cstdint>
#include <variant>


PXR_NAMESPACE_OPEN_SCOPE


/// A "null" interop handle so there is always one default
/// value available, regardless of actual supported rendering
/// APIs. Using it will result in an invalid presentation.
struct HgiPresentNullInteropHandle
{
    bool operator==(const HgiPresentNullInteropHandle&) const
    {
        return true;
    }

    bool operator!=(const HgiPresentNullInteropHandle& other) const
    {
        return !(*this == other);
    }
};

#if defined(PXR_GL_SUPPORT_ENABLED)
/// Interop to OpenGL
struct HgiPresentGLInteropHandle
{
    /// OpenGL framebuffer "name", or 0 for the currently bound framebuffer.
    uint32_t fboName{};

    bool operator==(const HgiPresentGLInteropHandle& other) const
    {
        return fboName == other.fboName;
    }

    bool operator!=(const HgiPresentGLInteropHandle& other) const
    {
        return !(*this == other);
    }
};
#endif

using HgiPresentInteropHandle = std::variant<
    HgiPresentNullInteropHandle
#if defined(PXR_GL_SUPPORT_ENABLED)
    , HgiPresentGLInteropHandle
#endif
>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
