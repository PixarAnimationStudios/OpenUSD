//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/present.h"

#include "pxr/imaging/hgiPresent/presentImpl.h"

#include "pxr/imaging/hgiPresent/noOp.h"

#if defined(PXR_GL_SUPPORT_ENABLED)
#include "pxr/imaging/hgiPresent/glInterop.h"
#endif

#if defined(PXR_METAL_SUPPORT_ENABLED)
#include "pxr/imaging/hgiPresent/metal.h"
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiPresent/vulkan.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE


HgiPresent::HgiPresent(HgiPresentImpl* implementation)
 : _hgiPresentImpl(implementation)
{}

HgiPresent::HgiPresent(HgiPresent&&) noexcept = default;

HgiPresent& HgiPresent::operator=(HgiPresent&&) noexcept = default;

HgiPresent::~HgiPresent() = default;

bool
HgiPresent::IsFormatSupported(HgiFormat colorFormat) const
{
    if (!TF_VERIFY(_hgiPresentImpl)) {
        return false;
    }

    return _hgiPresentImpl->IsFormatSupported(colorFormat);
}

bool
HgiPresent::IsValid() const
{
    if (!TF_VERIFY(_hgiPresentImpl)) {
        return false;
    }

    return _hgiPresentImpl->IsValid();
}

void
HgiPresent::Present(
    HgiTextureHandle const &srcColor,
    HgiTextureHandle const &srcDepth)
{
    if (!TF_VERIFY(_hgiPresentImpl)) {
        return;
    }

    _hgiPresentImpl->Present(srcColor, srcDepth);
}

/*static*/ HgiPresent
HgiPresent::Create(Hgi* hgi,
    const HgiPresentDestinationParams& params)
{
    return HgiPresent{
        std::visit([hgi](const auto& params) -> HgiPresentImpl* {
            using Params = std::decay_t<decltype(params)>;
            if constexpr (std::is_same_v<Params, HgiPresentInteropParams>) {
                return std::visit([hgi, &params](const auto& destination) -> HgiPresentImpl* {
                    using Destination = std::decay_t<decltype(destination)>;
#if defined(PXR_GL_SUPPORT_ENABLED)
                    if constexpr (std::is_same_v<Destination, HgiPresentGLInteropHandle>) {
                        return new HgiPresentGLInterop{ hgi, destination, params.composition };
                    } else
#endif
                    {
                        static_assert(std::is_same_v<Destination, HgiPresentNullInteropHandle>);
                        return new HgiPresentNoOp{ hgi, {false} };
                    }
                }, params.destination);
            } else if constexpr (std::is_same_v<Params, HgiPresentWindowParams>) {
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
                if (const auto hgiVulkan = dynamic_cast<HgiVulkan*>(hgi)) {
                    return new HgiPresentWindowVulkan{ hgiVulkan, params };
                }
#endif
#if defined(PXR_METAL_SUPPORT_ENABLED)
                if (const auto hgiMetal = DynamicCastHgiMetal(hgi)) {
                    return new HgiPresentWindowMetal{ hgiMetal, params };
                }
#endif
                TF_WARN("Unsupported Hgi: presentation is disabled");
                return new HgiPresentNoOp{ hgi, {false} };
            } else {
                static_assert(std::is_same_v<Params, HgiPresentNoOpParams>);
                return new HgiPresentNoOp{ hgi, params };
            }
        }, params)
    };
}


PXR_NAMESPACE_CLOSE_SCOPE
