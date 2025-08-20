//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/pxr.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiInterop/vulkan.h"
#include "pxr/imaging/hgiVulkan/blitCmds.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/texture.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

static const char* _vertexFullscreen120 =
    "#version 120\n"
    "attribute vec4 position;\n"
    "attribute vec2 uvIn;\n"
    "varying vec2 uv;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position = position;\n"
    "    uv = uvIn;\n"
    "}\n";

static const char* _vertexFullscreen140 =
    "#version 140\n"
    "in vec4 position;\n"
    "in vec2 uvIn;\n"
    "out vec2 uv;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position = position;\n"
    "    uv = uvIn;\n"
    "}\n";

static const char* _fragmentNoDepthFullscreen120 =
    "#version 120\n"
    "varying vec2 uv;\n"
    "uniform sampler2D colorIn;\n"
    "void main(void)\n"
    "{\n"
    "    gl_FragColor = texture2D(colorIn, uv);\n"
    "}\n";

static const char* _fragmentNoDepthFullscreen140 =
    "#version 140\n"
    "in vec2 uv;\n"
    "out vec4 colorOut;\n"
    "uniform sampler2D colorIn;\n"
    "void main(void)\n"
    "{\n"
    "    colorOut = texture(colorIn, uv);\n"
    "}\n";

static const char* _fragmentDepthFullscreen120 =
    "#version 120\n"
    "varying vec2 uv;\n"
    "uniform sampler2D colorIn;\n"
    "uniform sampler2D depthIn;\n"
    "void main(void)\n"
    "{\n"
    "    float depth = texture2D(depthIn, uv).r;\n"
    "    gl_FragColor = texture2D(colorIn, uv);\n"
    "    gl_FragDepth = depth;\n"
    "}\n";

static const char* _fragmentDepthFullscreen140 =
    "#version 140\n"
    "in vec2 uv;\n"
    "out vec4 colorOut;\n"
    "uniform sampler2D colorIn;\n"
    "uniform sampler2D depthIn;\n"
    "void main(void)\n"
    "{\n"
    "    colorOut = texture(colorIn, uv);\n"
    "    gl_FragDepth = texture(depthIn, uv).r;\n"
    "}\n";

static GLenum
_VKLayoutToGLLayout(VkImageLayout vkLayout)
{
    // Switch case version of Table 4.4 from:
    // https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects.txt
    switch (vkLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return GL_NONE;
        case VK_IMAGE_LAYOUT_GENERAL:
            return GL_LAYOUT_GENERAL_EXT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return GL_LAYOUT_COLOR_ATTACHMENT_EXT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return GL_LAYOUT_SHADER_READ_ONLY_EXT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return GL_LAYOUT_TRANSFER_SRC_EXT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return GL_LAYOUT_TRANSFER_DST_EXT;
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
            return GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
            return GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT;
        default:
            TF_CODING_ERROR("Unknown VKLayout Supplied,"
                "not compatible with GL: %u", vkLayout);
            return GL_NONE;
    }
}

static GLenum
_HgiFormatToInteropGLFormat(HgiFormat hgiFormat)
{
    // Using a seperate table to HgiGL's conversions to limit accepted interop
    // types
    switch (hgiFormat) {
        case HgiFormatFloat32Vec4: return GL_RGBA32F;
        case HgiFormatFloat16Vec4: return GL_RGBA16F;
        case HgiFormatUNorm8Vec4: return GL_RGBA8;
        case HgiFormatFloat32: return GL_R32F;
        default: 
            TF_CODING_ERROR("HgiFormat unable to interop: %i", hgiFormat);
            return GL_RGBA32F;
    }
}

static void
_ProcessShaderCompilationErrors(uint32_t shaderId)
{
    int logSize = 0;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logSize);
    std::string errors;
    errors.resize(logSize + 1);
    glGetShaderInfoLog(shaderId, logSize, nullptr, errors.data());
    TF_VERIFY(false, "Failed to compile shader: %s", errors.c_str());
}

static uint32_t
_CompileShader(const char* src, GLenum stage)
{
    const uint32_t shaderId = glCreateShader(stage);
    glShaderSource(shaderId, 1, &src, nullptr);
    glCompileShader(shaderId);
    GLint status;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        _ProcessShaderCompilationErrors(shaderId);
    }

    return shaderId;
}

static uint32_t
_LinkProgram(uint32_t vs, uint32_t fs)
{
    const uint32_t programId = glCreateProgram();
    glAttachShader(programId, vs);
    glAttachShader(programId, fs);
    glLinkProgram(programId);
    GLint status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    TF_VERIFY(status == GL_TRUE);
    return programId;
}

static uint32_t
_CreateVertexBuffer()
{
    static const float vertices[] = { 
        /* position        uv */
        -1,  3, -1, 1,    0, 2,
        -1, -1, -1, 1,    0, 0,
         3, -1, -1, 1,    2, 0 };
    uint32_t vertexBuffer = 0;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vertexBuffer;
}

static uint32_t
_CreateVertexArray()
{
    uint32_t vertexArray = 0;
    glGenVertexArrays(1, &vertexArray);
    return vertexArray;
}

uint32_t
HgiInteropVulkan::InteropTexNative::ConvertVulkanTextureToOpenGL(
            HgiVulkan* hgiVulkan,
            HgiTextureHandle const &src,
            bool isDepth)
{
    GfVec3i interopDims = _vkTex ?
        _vkTex->GetDescriptor().dimensions : GfVec3i(0);
    const GfVec3i& srcDims = src->GetDescriptor().dimensions;
    if (srcDims != interopDims) {
        _Reset(hgiVulkan,
            srcDims,
            src->GetDescriptor().format,
            isDepth);
    }

    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan->CreateBlitCmds();
    static_cast<HgiVulkanBlitCmds*>(blitCmds.get())->BlitTexture(src, _vkTex);

    hgiVulkan->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeNoWait);
    return _glTex;
}

GLenum
HgiInteropVulkan::InteropTexNative::DesiredGLLayout()
{
    return _vkTex ? _VKLayoutToGLLayout(
        static_cast<HgiVulkanTexture*>(_vkTex.Get())->GetImageLayout())
        : GL_NONE;
}

void
HgiInteropVulkan::InteropTexNative::_Reset(
            HgiVulkan* hgiVulkan,
            GfVec3i dimensions,
            HgiFormat format,
            bool isDepth)
{
    _Clear();
    _hgiVulkan = hgiVulkan;

    GLenum glFormat = _HgiFormatToInteropGLFormat(format);

    GLint tilingCount = 0;
    glGetInternalformativ(GL_TEXTURE_2D, glFormat,
        GL_NUM_TILING_TYPES_EXT, 1, &tilingCount);
    TF_VERIFY(tilingCount >= 1, "GL Tiling types is empty!");
    std::vector<GLint> tilingTypes(tilingCount);
    glGetInternalformativ(GL_TEXTURE_2D, glFormat,
        GL_TILING_TYPES_EXT, tilingCount, tilingTypes.data());

    HgiTextureDesc desc;
    desc.format = format;
    desc.debugName = "InteropTexVK";
    desc.dimensions = dimensions;
    desc.usage =
        (isDepth ? HgiTextureUsageBitsDepthTarget : 0)
        | HgiTextureUsageBitsShaderRead;

    // GL spec says returned array will always have optimal first if supported
    _vkTex = _hgiVulkan->CreateTextureForInterop(desc,
        tilingTypes[0] == GL_OPTIMAL_TILING_EXT);

    HgiVulkanTexture* vkDestCast = static_cast<HgiVulkanTexture*>(_vkTex.Get());
    VmaAllocationInfo2 allocInfo = vkDestCast->GetAllocationInfo();
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    _handle = _hgiVulkan->GetPrimaryDevice()
        ->GetWin32HandleForMemory(allocInfo.allocationInfo.deviceMemory);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkMemoryGetFdInfoKHR getInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR };
    getInfo.memory = allocInfo.allocationInfo.deviceMemory;
    getInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    
    int fd;
    HGIVULKAN_VERIFY_VK_RESULT(
        _hgiVulkan->GetPrimaryDevice()->vkGetMemoryFdKHR(
            _hgiVulkan->GetPrimaryDevice()->GetVulkanDevice(),
            &getInfo,
            &fd));
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#endif

    glCreateMemoryObjectsEXT(1, &_glMemoryObject);

    GLint isDedicated = allocInfo.dedicatedMemory;
    glMemoryObjectParameterivEXT(_glMemoryObject,
            GL_DEDICATED_MEMORY_OBJECT_EXT, &isDedicated);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    glImportMemoryWin32HandleEXT(
        _glMemoryObject,
        allocInfo.blockSize,
        GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
        _handle);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    glImportMemoryFdEXT(
        _glMemoryObject,
        allocInfo.blockSize,
        GL_HANDLE_TYPE_OPAQUE_FD_EXT,
        fd); // GL takes ownership of fd, don't need to close
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#endif
    glGenTextures(1, &_glTex);
    glBindTexture(GL_TEXTURE_2D, _glTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, tilingTypes[0]);

    glTexStorageMem2DEXT(
        GL_TEXTURE_2D,
        desc.mipLevels,
        glFormat,
        desc.dimensions[0],
        desc.dimensions[1],
        _glMemoryObject,
        allocInfo.allocationInfo.offset);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    // Verify there were no gl errors coming in.
    {
        const GLenum error = glGetError();
        TF_VERIFY(error == GL_NO_ERROR, "OpenGL error: 0x%04x", error);
    }
}

void
HgiInteropVulkan::InteropTexNative::_Clear()
{
    if (_vkTex) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        CloseHandle(_handle);
#endif
        glDeleteTextures(1, &_glTex);
        glDeleteMemoryObjectsEXT(1, &_glMemoryObject);
        _hgiVulkan->DestroyTexture(&_vkTex);
    }
}

HgiInteropVulkan::InteropTexNative::~InteropTexNative()
{
    _Clear();
}

GLenum
HgiInteropVulkan::InteropTexEmulated::DesiredGLLayout()
{
    return GL_NONE;
}

uint32_t
HgiInteropVulkan::InteropTexEmulated::ConvertVulkanTextureToOpenGL(
            HgiVulkan* hgiVulkan,
            HgiTextureHandle const &src,
            bool isDepth)
{
    HgiTextureDesc const& texDesc = src->GetDescriptor();
    const size_t byteSize = src->GetByteSizeOfResource();
    _texels.resize(byteSize);

    HgiTextureGpuToCpuOp readBackOp;
    readBackOp.cpuDestinationBuffer = _texels.data();
    readBackOp.destinationBufferByteSize = byteSize;
    readBackOp.destinationByteOffset = 0;
    readBackOp.gpuSourceTexture = src;
    readBackOp.mipLevel = 0;
    readBackOp.sourceTexelOffset = GfVec3i(0);

    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan->CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(readBackOp);
    hgiVulkan->SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    if (_glTex == 0) {
        glGenTextures(1, &_glTex);
        glBindTexture(GL_TEXTURE_2D, _glTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glBindTexture(GL_TEXTURE_2D, _glTex);
    }

    const int32_t width = texDesc.dimensions[0];
    const int32_t height = texDesc.dimensions[1];

    if (texDesc.format == HgiFormatFloat32Vec4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                     GL_FLOAT, _texels.data());
    } else if (texDesc.format == HgiFormatFloat16Vec4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
                     GL_HALF_FLOAT, _texels.data());
    } else if (texDesc.format == HgiFormatUNorm8Vec4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, _texels.data());
    } else if (texDesc.format == HgiFormatFloat32) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED,
                     GL_FLOAT, _texels.data());
    } else {
        TF_WARN("Unsupported texture format for Vulkan-GL interop");
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return _glTex;
}

HgiInteropVulkan::InteropTexEmulated::~InteropTexEmulated()
{
    if (_glTex) {
        glDeleteTextures(1, &_glTex);
    }
}

HgiInteropVulkan::InteropSemaphore::InteropSemaphore(HgiVulkan* hgiVulkan)
 : _hgiVulkan(hgiVulkan)
{
    glGenSemaphoresEXT(1, &_glSemaphore);
    VkExportSemaphoreCreateInfo exportInfo =
        { VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO };
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    TF_CODING_ERROR("Native MoltenVK interop not supported");
#endif
    HgiVulkanDevice* device = hgiVulkan->GetPrimaryDevice();

    VkSemaphoreCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.pNext = &exportInfo;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateSemaphore(device->GetVulkanDevice(), &createInfo,
            HgiVulkanAllocator(), &_vkSemaphore));

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkSemaphoreGetWin32HandleInfoKHR getInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR };
    getInfo.semaphore = _vkSemaphore;
    getInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    device->vkGetSemaphoreWin32HandleKHR(
        device->GetVulkanDevice(), &getInfo, &_handle);

    glImportSemaphoreWin32HandleEXT(
        _glSemaphore, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, _handle);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    int fd;
    VkSemaphoreGetFdInfoKHR getInfo =
        { VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR };
    getInfo.semaphore = _vkSemaphore;
    getInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;

    device->vkGetSemaphoreFdKHR(device->GetVulkanDevice(), &getInfo, &fd);

    glImportSemaphoreFdEXT(_glSemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#endif
}

HgiInteropVulkan::InteropSemaphore::~InteropSemaphore()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    CloseHandle(_handle);
#endif
    HgiVulkanDevice* device = _hgiVulkan->GetPrimaryDevice();
    device->WaitForIdle();
    glDeleteSemaphoresEXT(1, &_glSemaphore);
    vkDestroySemaphore(device->GetVulkanDevice(),
        _vkSemaphore, HgiVulkanAllocator());
}

HgiInteropVulkan::HgiInteropVulkan(Hgi* hgiVulkan)
    : _hgiVulkan(static_cast<HgiVulkan*>(hgiVulkan))
    , _vs(0)
    , _fsNoDepth(0)
    , _fsDepth(0)
    , _prgNoDepth(0)
    , _prgDepth(0)
    , _vertexBuffer(0)
    , _vertexArray(0)
{
    GarchGLApiLoad();
    _vs = _CompileShader(
        GARCH_GL_VERSION_3_1 ? _vertexFullscreen140 :
                               _vertexFullscreen120,
        GL_VERTEX_SHADER);
    _fsNoDepth = _CompileShader(
        GARCH_GL_VERSION_3_1 ? _fragmentNoDepthFullscreen140 :
                               _fragmentNoDepthFullscreen120,
        GL_FRAGMENT_SHADER);
    _fsDepth = _CompileShader(
        GARCH_GL_VERSION_3_1 ? _fragmentDepthFullscreen140 :
                               _fragmentDepthFullscreen120,
        GL_FRAGMENT_SHADER);
    _prgNoDepth = _LinkProgram(_vs, _fsNoDepth);
    _prgDepth = _LinkProgram(_vs, _fsDepth);
    _vertexBuffer = _CreateVertexBuffer();
    if (GARCH_GL_VERSION_3_0) {
        _vertexArray = _CreateVertexArray();
    }

    // Only supporting single and matching device interop between GL and VK to
    // satisfy semaphore interop requirements in GL_EXT_external_objects.
    bool onSameDevice = true;
    GLint uuidSize = 0;
    glGetIntegerv(GL_NUM_DEVICE_UUIDS_EXT, &uuidSize);
    if (uuidSize != 1) {
        onSameDevice = false;
    } else if (_hgiVulkan->GetCapabilities()->supportsNativeInterop) {
        GLubyte uuidDeviceGL[GL_UUID_SIZE_EXT];
        glGetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT, 0, uuidDeviceGL);
        GLubyte uuidDriverGL[GL_UUID_SIZE_EXT];
        glGetUnsignedBytevEXT(GL_DRIVER_UUID_EXT, uuidDriverGL);

        const VkPhysicalDeviceIDPropertiesKHR& vkPhysicalDeviceIdProperties =
            _hgiVulkan->GetCapabilities()->vkPhysicalDeviceIdProperties;

        onSameDevice = memcmp(uuidDeviceGL,
            vkPhysicalDeviceIdProperties.deviceUUID, VK_UUID_SIZE) == 0
            && memcmp(uuidDriverGL,
            vkPhysicalDeviceIdProperties.driverUUID, VK_UUID_SIZE) == 0;
    }

    if (onSameDevice
        && _hgiVulkan->GetCapabilities()->supportsNativeInterop
        && GARCH_GLAPI_HAS(EXT_memory_object)
        && GARCH_GLAPI_HAS(EXT_semaphore)
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        && GARCH_GLAPI_HAS(EXT_memory_object_win32)
        && GARCH_GLAPI_HAS(EXT_semaphore_win32)) {
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        && GARCH_GLAPI_HAS(EXT_memory_object_fd)
        && GARCH_GLAPI_HAS(EXT_semaphore_fd)) {
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        // To be added, either through MoltenVK adding GL interop,
        // or a later change if necessary
        && false) {
#endif
        _vkComplete = std::make_unique<InteropSemaphore>(_hgiVulkan);
        _glComplete = std::make_unique<InteropSemaphore>(_hgiVulkan);
        _colorTex = std::make_unique<InteropTexNative>();
        _depthTex = std::make_unique<InteropTexNative>();
    } else {
        _colorTex = std::make_unique<InteropTexEmulated>();
        _depthTex = std::make_unique<InteropTexEmulated>();
    }

    const GLenum error = glGetError();
    TF_VERIFY(error == GL_NO_ERROR, "OpenGL error: 0x%04x", error);
}

HgiInteropVulkan::~HgiInteropVulkan()
{
    glDeleteShader(_vs);
    glDeleteShader(_fsNoDepth);
    glDeleteShader(_fsDepth);
    glDeleteProgram(_prgNoDepth);
    glDeleteProgram(_prgDepth);
    glDeleteBuffers(1, &_vertexBuffer);
    if (_vertexArray) {
        glDeleteVertexArrays(1, &_vertexArray);
    }

    const GLenum error = glGetError();
    TF_VERIFY(error == GL_NO_ERROR, "OpenGL error: 0x%04x", error);
}

void
HgiInteropVulkan::CompositeToInterop(
    HgiTextureHandle const &color,
    HgiTextureHandle const &depth,
    VtValue const &framebuffer,
    GfVec4i const &compRegion)
{
    if (!ARCH_UNLIKELY(color)) {
        TF_WARN("No valid color texture provided");
        return;
    }

    // Verify there were no gl errors coming in.
    {
        const GLenum error = glGetError();
        TF_VERIFY(error == GL_NO_ERROR, "OpenGL error: 0x%04x", error);
    }

    GLint restoreDrawFramebuffer = 0;
    bool doRestoreDrawFramebuffer = false;

    if (!framebuffer.IsEmpty()) {
        if (framebuffer.IsHolding<uint32_t>()) {
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                          &restoreDrawFramebuffer);
            doRestoreDrawFramebuffer = true;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                              framebuffer.UncheckedGet<uint32_t>());
        } else {
            TF_CODING_ERROR(
                "dstFramebuffer must hold uint32_t when targeting OpenGL");
        }
    }

    // Convert textures from Vulkan to GL
    uint32_t colorInterop = 
        _colorTex->ConvertVulkanTextureToOpenGL(
            _hgiVulkan, color, /*isDepth=*/false);

    uint32_t depthInterop = 0;
    if (depth) {
        depthInterop =
            _depthTex->ConvertVulkanTextureToOpenGL(
                _hgiVulkan, depth, /*isDepth=*/true);
    }

    if (!ARCH_UNLIKELY(colorInterop)) {
        TF_CODING_ERROR("A valid color texture handle is required.\n");
        return;
    }

    GLuint glTexs[2] = { colorInterop, depthInterop };
    GLenum glLayouts[2] = { _colorTex->DesiredGLLayout(),
        depth ? _depthTex->DesiredGLLayout() : GL_NONE };
    HgiVulkanCommandQueue* commandQueue =
        _hgiVulkan->GetPrimaryDevice()->GetCommandQueue();
    if (_vkComplete) {
        // Manually submit before to signal the semaphore
        VkSubmitInfo submitInfoBefore = {};
        submitInfoBefore.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfoBefore.pSignalSemaphores = &_vkComplete->_vkSemaphore;
        submitInfoBefore.signalSemaphoreCount = 1;

        HGIVULKAN_VERIFY_VK_RESULT(
            vkQueueSubmit(commandQueue->GetVulkanGraphicsQueue(),
                1, &submitInfoBefore, VK_NULL_HANDLE));

        glWaitSemaphoreEXT(_vkComplete->_glSemaphore, 0, nullptr,
            2, glTexs, glLayouts);
    }

    {
        const GLenum error = glGetError();
        TF_VERIFY(error == GL_NO_ERROR, "OpenGL error: 0x%04x", error);
    }

#if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, "Interop");
    }
#endif

    GLint restoreActiveTexture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &restoreActiveTexture);

    // Setup shader program
    const uint32_t prg = color && depth ? _prgDepth : _prgNoDepth;
    glUseProgram(prg);

    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorInterop);
        const GLint loc = glGetUniformLocation(prg, "colorIn");
        glUniform1i(loc, 0);
    }

    // Depth is optional
    if (depth) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthInterop);
        const GLint loc = glGetUniformLocation(prg, "depthIn");
        glUniform1i(loc, 1);
    }

    // Get the current array buffer binding state
    GLint restoreArrayBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &restoreArrayBuffer);

    if (_vertexArray) {
        glBindVertexArray(_vertexArray);
    }

    // Vertex attributes
    const GLint locPosition = glGetAttribLocation(prg, "position");
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glVertexAttribPointer(locPosition, 4, GL_FLOAT, GL_FALSE,
            sizeof(float)*6, 0);
    glEnableVertexAttribArray(locPosition);

    const GLint locUv = glGetAttribLocation(prg, "uvIn");
    glVertexAttribPointer(locUv, 2, GL_FLOAT, GL_FALSE,
            sizeof(float)*6, reinterpret_cast<void*>(sizeof(float)*4));
    glEnableVertexAttribArray(locUv);

    // Since we want to composite over the application's framebuffer contents,
    // we need to honor depth testing if we have a valid depth texture.
    const GLboolean restoreDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean restoreDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &restoreDepthMask);
    GLint restoreDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &restoreDepthFunc);
    if (depth) {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        // Note: Use LEQUAL and not LESS to ensure that fragments with only
        // translucent contribution (that don't update depth) are composited.
        glDepthFunc(GL_LEQUAL);
    } else {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }

    // Enable blending to composite correctly over framebuffer contents.
    // Use pre-multiplied alpha scaling factors.
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glEnable(GL_BLEND);
    GLint restoreColorSrcFnOp, restoreAlphaSrcFnOp;
    GLint restoreColorDstFnOp, restoreAlphaDstFnOp;
    glGetIntegerv(GL_BLEND_SRC_RGB, &restoreColorSrcFnOp);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &restoreAlphaSrcFnOp);
    glGetIntegerv(GL_BLEND_DST_RGB, &restoreColorDstFnOp);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &restoreAlphaDstFnOp);
    glBlendFuncSeparate(/*srcColor*/GL_ONE,
                        /*dstColor*/GL_ONE_MINUS_SRC_ALPHA,
                        /*srcAlpha*/GL_ONE,
                        /*dstAlpha*/GL_ONE_MINUS_SRC_ALPHA);
    GLint restoreColorOp, restoreAlphaOp;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &restoreColorOp);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &restoreAlphaOp);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

    // Disable alpha to coverage (we want to composite the pixels as-is)
    GLboolean restoreAlphaToCoverage;
    glGetBooleanv(GL_SAMPLE_ALPHA_TO_COVERAGE, &restoreAlphaToCoverage);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    int32_t restoreVp[4];
    glGetIntegerv(GL_VIEWPORT, restoreVp);
    glViewport(compRegion[0], compRegion[1], compRegion[2], compRegion[3]);

    // Draw fullscreen triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Restore state and verify gl errors
    glDisableVertexAttribArray(locPosition);
    glDisableVertexAttribArray(locUv);
    if (_vertexArray) {
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, restoreArrayBuffer);
    
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
    glBlendFuncSeparate(restoreColorSrcFnOp, restoreColorDstFnOp, 
                        restoreAlphaSrcFnOp, restoreAlphaDstFnOp);
    glBlendEquationSeparate(restoreColorOp, restoreAlphaOp);

    if (!restoreDepthEnabled) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(restoreDepthMask);
    glDepthFunc(restoreDepthFunc);
    
    if (restoreAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    glViewport(restoreVp[0], restoreVp[1], restoreVp[2], restoreVp[3]);

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

#if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPopDebugGroup();
    }
#endif

    glActiveTexture(restoreActiveTexture);

    if (doRestoreDrawFramebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                          restoreDrawFramebuffer);
    }

    if (_glComplete) {
        glSignalSemaphoreEXT(_glComplete->_glSemaphore, 0, nullptr,
            2, glTexs, glLayouts);

        // Manually submit after to wait on GL
        VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkSubmitInfo submitInfoAfter = {};
        submitInfoAfter.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfoAfter.pWaitSemaphores = &_glComplete->_vkSemaphore;
        submitInfoAfter.waitSemaphoreCount = 1;
        submitInfoAfter.pWaitDstStageMask = &waitMask;

        HGIVULKAN_VERIFY_VK_RESULT(
            vkQueueSubmit(commandQueue->GetVulkanGraphicsQueue(),
                1, &submitInfoAfter, VK_NULL_HANDLE));
    }

    {
        const GLenum error = glGetError();
        TF_VERIFY(error == GL_NO_ERROR, "OpenGL error: 0x%04x", error);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
