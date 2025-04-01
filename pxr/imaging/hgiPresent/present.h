//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT_HGIPRESENT_H
#define PXR_IMAGING_HGIPRESENT_HGIPRESENT_H

#include "pxr/pxr.h"

#include "pxr/base/gf/rect2i.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/vt/value.h"

#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiPresent/api.h"
#include "pxr/imaging/hgiPresent/interopHandle.h"
#include "pxr/imaging/hgiPresent/windowHandle.h"

#include "pxr/imaging/hgiInterop/hgiInterop.h"

#include <variant>


PXR_NAMESPACE_OPEN_SCOPE


class Hgi;
class HgiTexture;

/// Configure the presentation to simply do nothing.
/// This is the default presentation since it doesn't
/// require any external resources.
struct HgiPresentNoOpParams
{
    /// Should HgiPresent::IsValid return true or false?
    /// Set to false when an instance of HgiPresent can't
    /// be created and an error state is needed.
    bool isValid{true};

    bool operator==(const HgiPresentNoOpParams& other) const
    {
        return isValid == other.isValid;
    }

    bool operator!=(const HgiPresentNoOpParams& other) const
    {
        return !(*this == other);
    }
};

/// Composition parameters for presentations that "present"
/// into an external framebuffer. Provides a way to "merge"
/// the rendered content into existing framebuffer contents.
/// The actual supported composition options dependent on the
/// interop backend.
using HgiPresentCompositionParams = HgiInteropCompositionParams;

/// "Present" to an externally managed framebuffer.
struct HgiPresentInteropParams
{
    /// A handle to an externally managed framebuffer.
    HgiPresentInteropHandle destination{};
    /// See \struct HgiCompositionParams.
    /// All composition options are supported by OpenGL interop.
    HgiPresentCompositionParams composition;

    bool operator==(const HgiPresentInteropParams& other) const
    {
        return destination == other.destination &&
            composition == other.composition;
    }

    bool operator!=(const HgiPresentInteropParams& other) const
    {
        return !(*this == other);
    }
};

/// Present to a UI window.
struct HgiPresentWindowParams
{
    /// A handle to an externally managed window.
    HgiPresentWindowHandle window{};
    /// Source texture color space. Not all values are supported,
    /// this is system dependent.
    TfToken srcColorSpace{GfColorSpaceNames->LinearRec709};
    /// Source preferred format. Not all values are supported. If no exact
    /// match is possible then a "best fit" is performed.
    HgiFormat preferredSurfaceFormat{HgiFormatUNorm8Vec4};
    /// Must be the same as srcColorSpace, except when srcColorSpace is
    /// "LinearRec709", then it can be "SRGBRec709", in which case the
    /// sRGB transfer function is applied in hardware before presentation.
    TfToken surfaceColorSpace{GfColorSpaceNames->SRGBRec709};
    /// Try to enable display refresh rate synchronization (aka v-sync).
    bool wantVsync{true};

    bool operator==(const HgiPresentWindowParams& other) const
    {
        return window == other.window &&
            srcColorSpace == other.srcColorSpace &&
            preferredSurfaceFormat == other.preferredSurfaceFormat &&
            surfaceColorSpace == other.surfaceColorSpace &&
            wantVsync == other.wantVsync;
    }

    bool operator!=(const HgiPresentWindowParams& other) const
    {
        return !(*this == other);
    }
};

using HgiPresentDestinationParams = std::variant<
    HgiPresentNoOpParams,
    HgiPresentWindowParams,
    HgiPresentInteropParams
>;

class HgiPresentImpl;

///
/// \class HgiPresent
///
/// Enables presenting a color AOV to some externally managed
/// surface. This could be a UI window, or an offscreen framebuffer.
/// Capabilities are very destination dependent, see the
/// HgiPresentDestinationParams options for more info.
///
class HgiPresent final
{
public:
    HGIPRESENT_API
    HgiPresent(HgiPresent&&) noexcept;

    HGIPRESENT_API
    HgiPresent& operator=(HgiPresent&&) noexcept;

    HgiPresent(const HgiPresent&) = delete;
    HgiPresent& operator=(const HgiPresent&) = delete;

    HGIPRESENT_API
    ~HgiPresent();

    /// Check if an AOV format can be presented. This only
    /// checks the binary format, it ignores the color space.
    HGIPRESENT_API
    bool IsFormatSupported(HgiFormat colorFormat) const;

    /// Check if the presentation is currently valid. A presentation might
    /// be initially invalid until the first present call. A valid
    /// presentation might become invalid at any point due to
    /// changes in the external resources. An invalid presentation
    /// will not present anything, and might need to be recreated.
    HGIPRESENT_API
    bool IsValid() const;

    /// Present a color AOV. If composition is supported and
    /// destination depth is available, then a depth AOV can
    /// also be merged into the destination depth according
    /// to the depth comparison function. Otherwise, it's ignored.
    HGIPRESENT_API
    void Present(
        HgiTextureHandle const &srcColor,
        HgiTextureHandle const &srcDepth);

    /// Create an instance of HgiPresent from the given parameters.
    /// While this always succeeds, the resulting presentation might
    /// not always be functional, and might become non-functional at
    /// any point due to changes in the external resources.
    HGIPRESENT_API
    static HgiPresent Create(Hgi* hgi,
        const HgiPresentDestinationParams& params);

private:
    explicit HgiPresent(HgiPresentImpl* implementation);

    std::unique_ptr<HgiPresentImpl> _hgiPresentImpl;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
