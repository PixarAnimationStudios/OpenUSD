//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIINTEROP_HGIINTEROPVULKAN_H
#define PXR_IMAGING_HGIINTEROP_HGIINTEROPVULKAN_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiInterop/api.h"
#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <memory>
#include <unordered_map>
#include <vector>

#if defined(ARCH_OS_WINDOWS)
#include <handleapi.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkan;
class VtValue;

/// \class HgiInteropVulkan
///
/// Provides Vulkan/GL interop.
///
class HgiInteropVulkan final
{
public:
    HGIINTEROP_API
    HgiInteropVulkan(Hgi* hgiVulkan);

    HGIINTEROP_API
    ~HgiInteropVulkan();

    /// Composite provided color (and optional depth) textures over app's
    /// framebuffer contents.
    HGIINTEROP_API
    void CompositeToInterop(
        HgiTextureHandle const &color,
        HgiTextureHandle const &depth,
        VtValue const &framebuffer,
        GfVec4i const& viewport);

private:
    HgiInteropVulkan() = delete;

    HgiVulkan* _hgiVulkan;
    uint32_t _vs;
    uint32_t _fsNoDepth;
    uint32_t _fsDepth;
    uint32_t _prgNoDepth;
    uint32_t _prgDepth;
    uint32_t _vertexBuffer;
    uint32_t _vertexArray;

    class InteropTex {
    public:
        virtual ~InteropTex() = default;
        virtual uint32_t ConvertVulkanTextureToOpenGL(
            HgiVulkan* hgiVulkan,
            HgiTextureHandle const &src,
            bool isDepth) = 0;
        virtual GLenum DesiredGLLayout() = 0;
    };

    // Texture interop for platforms with native support
    // Ex: Windows/Linux optional extensions
    class InteropTexNative : public InteropTex
    {
    public:
        InteropTexNative() = default;
        ~InteropTexNative() override;
        uint32_t ConvertVulkanTextureToOpenGL(
            HgiVulkan* hgiVulkan,
            HgiTextureHandle const &src,
            bool isDepth) override;
        GLenum DesiredGLLayout() override;
    private:
        void _Clear();
        void _Reset(
            HgiVulkan* hgiVulkan,
            GfVec3i dimensions,
            HgiFormat format,
            bool isDepth
        );
        InteropTexNative & operator=(InteropTexNative&) = delete;
        InteropTexNative(const InteropTexNative&) = delete;

        uint32_t _glTex;
        uint32_t _glMemoryObject;
        HgiTextureHandle _vkTex;
        HgiVulkan* _hgiVulkan;
#if defined(ARCH_OS_WINDOWS)
        // Need to keep handle, GL doesn't adopt
        HANDLE _handle = nullptr;
#elif defined(ARCH_OS_LINUX)
        // No fd handle necessary, adopted by GL
#elif defined(ARCH_OS_OSX)
#endif
    };

    // Texture interop for platforms where interop is implemented via readback
    // Ex: MoltenVK and Windows/Linux machines without native support
    class InteropTexEmulated : public InteropTex
    {
    public:
        InteropTexEmulated() = default;
        ~InteropTexEmulated() override;
        uint32_t ConvertVulkanTextureToOpenGL(
            HgiVulkan* hgiVulkan,
            HgiTextureHandle const &src,
            bool isDepth) override;
        GLenum DesiredGLLayout() override;
    private:
        uint32_t _glTex;
        std::vector<uint8_t> _texels;
    };

    std::unique_ptr<InteropTex> _colorTex;
    std::unique_ptr<InteropTex> _depthTex;

    // Used for sync between GL and VK
    // Currently always used by InteropTexNative, but could be optional
    // if hardware has only one hardware queue (see Metal interop as example)
    struct InteropSemaphore
    {
        InteropSemaphore(HgiVulkan* hgiVulkan);
        ~InteropSemaphore();
        InteropSemaphore & operator=(InteropSemaphore&) = delete;
        InteropSemaphore(const InteropSemaphore&) = delete;

        VkSemaphore _vkSemaphore;
        uint32_t _glSemaphore;
        HgiVulkan* _hgiVulkan;
#if defined(ARCH_OS_WINDOWS)
        // Need to keep handle, GL doesn't adopt
        HANDLE _handle = nullptr;
#elif defined(ARCH_OS_LINUX)
        // No fd handle necessary, adopted by GL
#elif defined(ARCH_OS_OSX)
#endif
    };

    std::unique_ptr<InteropSemaphore> _vkComplete;
    std::unique_ptr<InteropSemaphore> _glComplete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
