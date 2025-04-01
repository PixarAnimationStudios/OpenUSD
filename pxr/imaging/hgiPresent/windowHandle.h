//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_WINDOWHANDLE_H
#define PXR_IMAGING_HGIPRESENT_WINDOWHANDLE_H

#include "pxr/pxr.h"

#include "pxr/base/arch/defines.h"

#include <variant>

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#endif


#if defined(PXR_X11_SUPPORT_ENABLED)
// The X11 proto header introduces a lot of macros with very generic names that
// have very high conflict risks. It's safer to use a forward declaration.
// See: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/blob/master/include/X11/X.h
// and https://gitlab.freedesktop.org/xorg/lib/libx11/-/blob/23f0352eca362515d598bfdbd8ecec070dcd1b28/include/X11/Xlib.h#L251
using Display = struct _XDisplay;
using Window = unsigned long;
#endif

#if defined(ARCH_OS_DARWIN)
// Same forward declaration as used in the Vulkan headers.
// See: https://docs.vulkan.org/spec/latest/chapters/VK_KHR_surface/wsi.html#VkMetalSurfaceCreateInfoEXT
#ifdef __OBJC__
@class CAMetalLayer;
#else
using CAMetalLayer = void;
#endif
#endif


PXR_NAMESPACE_OPEN_SCOPE


/// A "null" window handle so there is always one default
/// value available, regardless of actual supported windowing
/// systems. Using it will result in an invalid presentation.
struct HgiPresentNullWindowHandle
{
    bool operator==(const HgiPresentNullWindowHandle&) const
    {
        return true;
    }

    bool operator!=(const HgiPresentNullWindowHandle& other) const
    {
        return !(*this == other);
    }
};

#if defined(ARCH_OS_DARWIN)
/// A Metal layer handle.
struct HgiPresentMetalWindowHandle
{
    const CAMetalLayer* layer = nullptr;

    bool operator==(const HgiPresentMetalWindowHandle& other) const
    {
        return layer == other.layer;
    }

    bool operator!=(const HgiPresentMetalWindowHandle& other) const
    {
        return !(*this == other);
    }
};
#endif

#if defined(ARCH_OS_WINDOWS)
/// A WIN32 window handle.
struct HgiPresentWin32WindowHandle
{
    HINSTANCE instance = nullptr;
    HWND window = nullptr;

    bool operator==(const HgiPresentWin32WindowHandle& other) const
    {
        return instance == other.instance && window == other.window;
    }

    bool operator!=(const HgiPresentWin32WindowHandle& other) const
    {
        return !(*this == other);
    }
};
#endif

#if defined(PXR_X11_SUPPORT_ENABLED)
/// And X11 window handle.
struct HgiPresentXlibWindowHandle
{
    Display* display = nullptr;
    Window window = 0;

    bool operator==(const HgiPresentXlibWindowHandle& other) const
    {
        return display == other.display && window == other.window;
    }

    bool operator!=(const HgiPresentXlibWindowHandle& other) const
    {
        return !(*this == other);
    }
};
#endif

/// Any window handle currently supported by the system.
using HgiPresentWindowHandle = std::variant<
    HgiPresentNullWindowHandle
#if defined(ARCH_OS_DARWIN)
    , HgiPresentMetalWindowHandle
#endif
#if defined(ARCH_OS_WINDOWS)
    , HgiPresentWin32WindowHandle
#endif
#if defined(PXR_X11_SUPPORT_ENABLED)
    , HgiPresentXlibWindowHandle
#endif
>;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
