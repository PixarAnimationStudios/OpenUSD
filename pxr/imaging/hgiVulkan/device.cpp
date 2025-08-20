//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/pipelineCache.h"
#include "pxr/imaging/hgiVulkan/vk_mem_alloc.h"

#include "pxr/base/tf/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIVULKAN_PREFERRED_DEVICE_TYPE,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    "Preferred device type. Use VkPhysicalDeviceType enum values.");

// VMA links this to the interop pool
static VkExportMemoryAllocateInfoKHR _exportInfo =
{
    VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
    nullptr,
    VK_EXTERNAL_MEMORY_HANDLE_AUTO
};

static uint32_t
_GetGraphicsQueueFamilyIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount,
        nullptr);

    std::vector<VkQueueFamilyProperties> queues(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueCount,
        queues.data());

    for (uint32_t i = 0; i < queueCount; i++) {
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return i;
        }
    }

    return VK_QUEUE_FAMILY_IGNORED;
}

static bool
_SupportsPresentation(
    VkPhysicalDevice physicalDevice,
    uint32_t familyIndex)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        return vkGetPhysicalDeviceWin32PresentationSupportKHR(
                    physicalDevice, familyIndex);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        Display* dsp = XOpenDisplay(nullptr);
        VisualID visualID = XVisualIDFromVisual(
            DefaultVisual(dsp, DefaultScreen(dsp)));
        return vkGetPhysicalDeviceXlibPresentationSupportKHR(
                    physicalDevice, familyIndex, dsp, visualID);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        // Presentation currently always supported on Metal / MoltenVk
        return true;
#else
        #error Unsupported Platform
        return true;
#endif
}

HgiVulkanDevice::HgiVulkanDevice(HgiVulkanInstance* instance)
    : _vkPhysicalDevice(nullptr)
    , _vkDevice(nullptr)
    , _vmaAllocator(nullptr)
    , _commandQueue(nullptr)
    , _capabilities(nullptr)
    , _pipelineCache(nullptr)
{
    //
    // Determine physical device
    //

    const uint32_t maxDevices = 64;
    VkPhysicalDevice physicalDevices[maxDevices];
    uint32_t physicalDeviceCount = maxDevices;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkEnumeratePhysicalDevices(
            instance->GetVulkanInstance(),
            &physicalDeviceCount,
            physicalDevices)
    );

    const auto preferredDeviceType = static_cast<VkPhysicalDeviceType>(
        TfGetEnvSetting(HGIVULKAN_PREFERRED_DEVICE_TYPE));
    for (uint32_t i = 0; i < physicalDeviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

        uint32_t familyIndex =
            _GetGraphicsQueueFamilyIndex(physicalDevices[i]);

        if (familyIndex == VK_QUEUE_FAMILY_IGNORED) continue;

        // Assume we always want a presentation capable device for now.
        if (instance->HasPresentation() &&
            !_SupportsPresentation(physicalDevices[i], familyIndex)) {
            continue;
        }

        if (props.apiVersion < VK_API_VERSION_1_0) continue;

        // Try to find a preferred device type. Until we find one, store the
        // first non-preferred device as fallback in case we never find a
        // preferred device at all.
        if (props.deviceType == preferredDeviceType) {
            _vkPhysicalDevice = physicalDevices[i];
            _vkGfxsQueueFamilyIndex = familyIndex;
            break;
        }
        if (!_vkPhysicalDevice) {
            _vkPhysicalDevice = physicalDevices[i];
            _vkGfxsQueueFamilyIndex = familyIndex;
        }
    }

    if (!_vkPhysicalDevice) {
        TF_CODING_ERROR("VULKAN_ERROR: Unable to determine physical device");
        return;
    }

    //
    // Query supported extensions for device
    //

    uint32_t extensionCount = 0;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkEnumerateDeviceExtensionProperties(
            _vkPhysicalDevice,
            nullptr,
            &extensionCount,
            nullptr)
    );

    _vkExtensions.resize(extensionCount);

    HGIVULKAN_VERIFY_VK_RESULT(
        vkEnumerateDeviceExtensionProperties(
            _vkPhysicalDevice,
            nullptr,
            &extensionCount,
            _vkExtensions.data())
    );

    //
    // Create Device
    //
    _capabilities = new HgiVulkanCapabilities(this);

    VkDeviceQueueCreateInfo queueInfo =
        {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    float queuePriorities[] = {1.0f};
    queueInfo.queueFamilyIndex = _vkGfxsQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;

    std::vector<const char*> extensions;

    // Not available if we're surfaceless (minimal Lavapipe build for example).
    if (IsSupportedExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    // Allow certain buffers/images to have dedicated memory allocations to
    // improve performance on some GPUs.
    bool dedicatedAllocations = false;
    if (IsSupportedExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME)
        && IsSupportedExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
    {
        dedicatedAllocations = true;
        extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    }

    // Allow OpenGL interop
    // Note requires four extensions in HgiVulkanInstance.
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (IsSupportedExtension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) &&
        IsSupportedExtension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME) &&
        IsSupportedExtension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) &&
        IsSupportedExtension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
        extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    if (IsSupportedExtension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) &&
        IsSupportedExtension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME) &&
        IsSupportedExtension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) &&
        IsSupportedExtension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
        extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    }
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    // To be added, either through MoltenVK adding GL interop,
    // or a later change if necessary
#endif

    // Memory budget query extension
    bool supportsMemExtension = false;
    if (IsSupportedExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
        supportsMemExtension = true;
        extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    }

    // Resolve depth during render pass resolve extension
    if (IsSupportedExtension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
        extensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
        extensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
    }

    // Allows the same layout in structs between c++ and glsl (share structs).
    // This means instead of 'std430' you can now use 'scalar'.
    if (IsSupportedExtension(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME)) {
        extensions.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
    } else {
        TF_WARN("Unsupported VK_EXT_scalar_block_layout."
                "Update gfx driver?");
    }

    // Allow conservative rasterization.
    if (IsSupportedExtension(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME)) {
        extensions.push_back(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
    }

    // Allow use of built-in shader barycentrics.
    if (IsSupportedExtension(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
    }

    // Allow use of shader draw parameters.
    if (IsSupportedExtension(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
    }

    // Allow use of vertex attribute divisors.
    if (IsSupportedExtension(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME)) {
        extensions.push_back(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME);
    }

    // Allow use of line rasterization ext
    if (IsSupportedExtension(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME);
    }

    // This extension is needed to allow the viewport to be flipped in Y so that
    // shaders and vertex data can remain the same between opengl and vulkan.
    extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

#ifdef VK_USE_PLATFORM_METAL_EXT
    if (IsSupportedExtension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    }
#endif

    // Enabling certain features may incure a performance hit
    // (e.g. robustBufferAccess), so only enable the features we will use.

    VkPhysicalDeviceFeatures2 features2 =
        {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    features2.features.multiDrawIndirect =
        _capabilities->vkDeviceFeatures2.features.multiDrawIndirect;
    features2.features.samplerAnisotropy =
        _capabilities->vkDeviceFeatures2.features.samplerAnisotropy;
    features2.features.shaderSampledImageArrayDynamicIndexing =
        _capabilities->vkDeviceFeatures2.features.shaderSampledImageArrayDynamicIndexing;
    features2.features.shaderStorageImageArrayDynamicIndexing =
        _capabilities->vkDeviceFeatures2.features.shaderStorageImageArrayDynamicIndexing;
    features2.features.sampleRateShading =
        _capabilities->vkDeviceFeatures2.features.sampleRateShading;
    features2.features.shaderClipDistance =
        _capabilities->vkDeviceFeatures2.features.shaderClipDistance;
    features2.features.tessellationShader =
        _capabilities->vkDeviceFeatures2.features.tessellationShader;
    features2.features.depthClamp =
        _capabilities->vkDeviceFeatures2.features.depthClamp;
    features2.features.shaderFloat64 =
        _capabilities->vkDeviceFeatures2.features.shaderFloat64;
    features2.features.fillModeNonSolid =
        _capabilities->vkDeviceFeatures2.features.fillModeNonSolid;
    features2.features.alphaToOne =
        _capabilities->vkDeviceFeatures2.features.alphaToOne;
    // Needed to write to storage buffers from vertex shader (eg. GPU culling).
    features2.features.vertexPipelineStoresAndAtomics =
        _capabilities->vkDeviceFeatures2.features.vertexPipelineStoresAndAtomics;
    // Needed to write to storage buffers from fragment shader (eg. OIT).
    features2.features.fragmentStoresAndAtomics =
        _capabilities->vkDeviceFeatures2.features.fragmentStoresAndAtomics;
    // Needed for buffer address feature
    features2.features.shaderInt64 =
        _capabilities->vkDeviceFeatures2.features.shaderInt64;
    // Needed for gl_primtiveID
    features2.features.geometryShader =
        _capabilities->vkDeviceFeatures2.features.geometryShader;

    VkPhysicalDeviceVulkan11Features vulkan11Features =
        {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    vulkan11Features.shaderDrawParameters =
        _capabilities->vkVulkan11Features.shaderDrawParameters;
    vulkan11Features.pNext = features2.pNext;
    features2.pNext = &vulkan11Features;

    // Vertex attribute divisor features ext
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT vertexAttributeDivisorFeatures
    { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT };
    vertexAttributeDivisorFeatures.vertexAttributeInstanceRateDivisor =
        _capabilities->vkVertexAttributeDivisorFeatures.vertexAttributeInstanceRateDivisor;
    vertexAttributeDivisorFeatures.pNext = features2.pNext;
    features2.pNext = &vertexAttributeDivisorFeatures;

    // Barycentric features
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR barycentricFeatures {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR
    };
    if (IsSupportedExtension(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME)) {
        barycentricFeatures.fragmentShaderBarycentric =
            _capabilities->vkBarycentricFeatures.fragmentShaderBarycentric;
        barycentricFeatures.pNext = features2.pNext;
        features2.pNext = &barycentricFeatures;
    }

    // Line rasterization features needed for Bresenham line rasterization
    VkPhysicalDeviceLineRasterizationFeaturesKHR lineRasterFeatures {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_KHR };
    if (IsSupportedExtension(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME)) {
        lineRasterFeatures.bresenhamLines =
            _capabilities->vkLineRasterizationFeatures.bresenhamLines;
        lineRasterFeatures.pNext = features2.pNext;
        features2.pNext = &lineRasterFeatures;
    }

    VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = (uint32_t) extensions.size();
    createInfo.pNext = &features2;

    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateDevice(
            _vkPhysicalDevice,
            &createInfo,
            HgiVulkanAllocator(),
            &_vkDevice)
    );

    HgiVulkanSetupDeviceDebug(instance, this);

    //
    // Extension function pointers
    //

    vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)
    vkGetDeviceProcAddr(_vkDevice, "vkCreateRenderPass2KHR");

    if (_capabilities->supportsNativeInterop) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)
        vkGetDeviceProcAddr(_vkDevice, "vkGetMemoryWin32HandleKHR");

        vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)
        vkGetDeviceProcAddr(_vkDevice, "vkGetSemaphoreWin32HandleKHR");
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)
        vkGetDeviceProcAddr(_vkDevice, "vkGetMemoryFdKHR");

        vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)
        vkGetDeviceProcAddr(_vkDevice, "vkGetSemaphoreFdKHR");
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#endif
    }

    //
    // Memory allocator
    //

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = instance->GetVulkanInstance();
    allocatorInfo.physicalDevice = _vkPhysicalDevice;
    allocatorInfo.device = _vkDevice;
    if (dedicatedAllocations) {
        allocatorInfo.flags |=VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    }

    if (supportsMemExtension) {
        allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    }

    HGIVULKAN_VERIFY_VK_RESULT(
        vmaCreateAllocator(&allocatorInfo, &_vmaAllocator)
    );

    //
    // Command Queue
    //

    _commandQueue = new HgiVulkanCommandQueue(this);

    //
    // Pipeline cache
    //

    _pipelineCache = new HgiVulkanPipelineCache(this);
}

HgiVulkanDevice::~HgiVulkanDevice()
{
    if (_vkDevice) {
        // Make sure device is idle before destroying objects.
        HGIVULKAN_VERIFY_VK_RESULT(
            vkDeviceWaitIdle(_vkDevice)
        );
    }

    std::lock_guard<std::mutex> lock(_vmaInteropPoolsLock);
    for (auto& entry : _vmaInteropPoolsForMemoryType)
    {
        vmaDestroyPool(_vmaAllocator, entry.second);
    }

    delete _pipelineCache;
    delete _commandQueue;
    delete _capabilities;
    vmaDestroyAllocator(_vmaAllocator);
    vkDestroyDevice(_vkDevice, HgiVulkanAllocator());
}

VkDevice
HgiVulkanDevice::GetVulkanDevice() const
{
    return _vkDevice;
}

VmaAllocator
HgiVulkanDevice::GetVulkanMemoryAllocator() const
{
    return _vmaAllocator;
}

VmaPool
HgiVulkanDevice::GetVMAPoolForInterop(VkImageCreateInfo imageInfo)
{
    TF_VERIFY(_capabilities->supportsNativeInterop,
        "Device doesn't support native interop!");
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    uint32_t memoryTypeIndex;
    HGIVULKAN_VERIFY_VK_RESULT(
        vmaFindMemoryTypeIndexForImageInfo(
        GetVulkanMemoryAllocator(),
        &imageInfo,
        &allocInfo,
        &memoryTypeIndex));

    std::lock_guard<std::mutex> lock(_vmaInteropPoolsLock);
    auto iter = _vmaInteropPoolsForMemoryType.find(memoryTypeIndex);
    if (iter == _vmaInteropPoolsForMemoryType.end()) {
        VmaPoolCreateInfo poolInfo = {};
        poolInfo.pMemoryAllocateNext = &_exportInfo;
        poolInfo.memoryTypeIndex = memoryTypeIndex;

        VmaPool pool;
        HGIVULKAN_VERIFY_VK_RESULT(
            vmaCreatePool(
                _vmaAllocator,
                &poolInfo, 
                &pool));
        iter = _vmaInteropPoolsForMemoryType.insert({ memoryTypeIndex, pool }).first;
    }
    return iter->second;
}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
HANDLE
HgiVulkanDevice::GetWin32HandleForMemory(VkDeviceMemory memory)
{
    // A temporary workaround for vkGetMemoryWin32HandleKHR being invalid to
    // call on the same VkDeviceMemory and handleType twice. To be replaced
    // with direct VMA call on SDK update (see header for details)
    std::lock_guard<std::mutex> lock(_vmaInteropWin32HandleLock);
    auto iter = _vmaInteropWin32HandleForMemory.find(memory);
    if (iter == _vmaInteropWin32HandleForMemory.end()) {
        VkMemoryGetWin32HandleInfoKHR getInfo { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
        getInfo.memory = memory;
        getInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;

        HANDLE handle;
        HGIVULKAN_VERIFY_VK_RESULT(
            vkGetMemoryWin32HandleKHR(
                GetVulkanDevice(),
                &getInfo,
                &handle));
        iter = _vmaInteropWin32HandleForMemory.insert({ memory, handle }).first;
    }
    HANDLE curProc = GetCurrentProcess();
    HANDLE duplicateHandle;
    if (!DuplicateHandle(curProc, iter->second, curProc,
        &duplicateHandle, 0, false, DUPLICATE_SAME_ACCESS)) {
        TF_CODING_ERROR("Couldn't duplicate Windows Handle!");
    }
    return duplicateHandle;
}
#endif

HgiVulkanCommandQueue*
HgiVulkanDevice::GetCommandQueue() const
{
    return _commandQueue;
}

HgiVulkanCapabilities const&
HgiVulkanDevice::GetDeviceCapabilities() const
{
    return *_capabilities;
}

uint32_t
HgiVulkanDevice::GetGfxQueueFamilyIndex() const
{
    return _vkGfxsQueueFamilyIndex;
}

VkPhysicalDevice
HgiVulkanDevice::GetVulkanPhysicalDevice() const
{
    return _vkPhysicalDevice;
}

HgiVulkanPipelineCache*
HgiVulkanDevice::GetPipelineCache() const
{
    return _pipelineCache;
}

void
HgiVulkanDevice::WaitForIdle()
{
    HGIVULKAN_VERIFY_VK_RESULT(
        vkDeviceWaitIdle(_vkDevice)
    );
}

bool
HgiVulkanDevice::IsSupportedExtension(const char* extensionName) const
{
    for (VkExtensionProperties const& ext : _vkExtensions) {
        if (!strcmp(extensionName, ext.extensionName)) {
            return true;
        }
    }

    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
