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

#include "pxr/base/tf/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE


static uint32_t
_GetGraphicsQueueFamilyIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, 0);

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
    HgiVulkanInstance* instance,
    VkPhysicalDevice physicalDevice,
    uint32_t familyIndex)
{
    // XXX With volk, these functions weren't loaded correctly for me (would 
    // crash when called). Loading them like this seems to work. Unsure why.
    #if defined(VK_USE_PLATFORM_WIN32_KHR)
        VkInstance vkInstance = instance->GetVulkanInstance();
        PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR
            vkGetPhysicalDeviceWin32PresentationSupportKHR = 
                (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)
                vkGetInstanceProcAddr(
                    vkInstance,
                    "vkGetPhysicalDeviceWin32PresentationSupportKHR");
        return vkGetPhysicalDeviceWin32PresentationSupportKHR(
                    physicalDevice, familyIndex);
    #elif defined(VK_USE_PLATFORM_XLIB_KHR)
        Display* dsp = XOpenDisplay(nullptr);
        VisualID visualID = XVisualIDFromVisual(
            DefaultVisual(dsp, DefaultScreen(dsp)));
        
        VkInstance vkInstance = instance->GetVulkanInstance();
        PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR
            vkGetPhysicalDeviceXlibPresentationSupportKHR = 
                (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)
                vkGetInstanceProcAddr(
                    vkInstance,
                    "vkGetPhysicalDeviceXlibPresentationSupportKHR");
        return vkGetPhysicalDeviceXlibPresentationSupportKHR &&
                vkGetPhysicalDeviceXlibPresentationSupportKHR(
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
{
    //
    // Determine physical device
    //

    const uint32_t maxDevices = 64;
    VkPhysicalDevice physicalDevices[maxDevices];
    uint32_t physicalDeviceCount = maxDevices;
    TF_VERIFY(
        vkEnumeratePhysicalDevices(
            instance->GetVulkanInstance(),
            &physicalDeviceCount,
            physicalDevices) == VK_SUCCESS
    );

    for (uint32_t i = 0; i < physicalDeviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

        uint32_t familyIndex =
            _GetGraphicsQueueFamilyIndex(physicalDevices[i]);

        if (familyIndex == VK_QUEUE_FAMILY_IGNORED) continue;

        // Assume we always want a presentation capable device for now.
        if (!_SupportsPresentation(instance, physicalDevices[i], familyIndex)) {
            continue;
        }

        if (props.apiVersion < VK_API_VERSION_1_0) continue;

        // Try to find a discrete device. Until we find a discrete device,
        // store the first non-discrete device as fallback in case we never
        // find a discrete device at all.
        if (props.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            _vkPhysicalDevice = physicalDevices[i];
            _vkGfxsQueueFamilyIndex = familyIndex;
            break;
        } else if (!_vkPhysicalDevice) {
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
    TF_VERIFY(
        vkEnumerateDeviceExtensionProperties(
            _vkPhysicalDevice,
            nullptr,
            &extensionCount,
            nullptr) == VK_SUCCESS
    );

    _vkExtensions.resize(extensionCount);

    TF_VERIFY(
        vkEnumerateDeviceExtensionProperties(
            _vkPhysicalDevice,
            nullptr,
            &extensionCount,
            _vkExtensions.data()) == VK_SUCCESS
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

    // Allow OpenGL interop - Note requires two extensions in HgiVulkanInstance.
    if (IsSupportedExtension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) &&
        IsSupportedExtension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME))
    {
        extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    }

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

    // This extension is needed to allow the viewport to be flipped in Y so that
    // shaders and vertex data can remain the same between opengl and vulkan.
    extensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

    // Enabling certain features may incure a performance hit
    // (e.g. robustBufferAccess), so only enable the features we will use.
    VkPhysicalDeviceVulkan11Features vulkan11Features =
        {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    vulkan11Features.pNext = _capabilities->vkVulkan11Features.pNext;
    vulkan11Features.shaderDrawParameters =
        _capabilities->vkVulkan11Features.shaderDrawParameters;

    VkPhysicalDeviceFeatures2 features =
        {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features.pNext = &vulkan11Features;

    features.features.multiDrawIndirect =
        _capabilities->vkDeviceFeatures.multiDrawIndirect;
    features.features.samplerAnisotropy =
        _capabilities->vkDeviceFeatures.samplerAnisotropy;
    features.features.shaderSampledImageArrayDynamicIndexing =
        _capabilities->vkDeviceFeatures.shaderSampledImageArrayDynamicIndexing;
    features.features.shaderStorageImageArrayDynamicIndexing =
        _capabilities->vkDeviceFeatures.shaderStorageImageArrayDynamicIndexing;
    features.features.sampleRateShading =
        _capabilities->vkDeviceFeatures.sampleRateShading;
    features.features.shaderClipDistance =
        _capabilities->vkDeviceFeatures.shaderClipDistance;
    features.features.tessellationShader =
        _capabilities->vkDeviceFeatures.tessellationShader;
    features.features.depthClamp =
        _capabilities->vkDeviceFeatures.depthClamp;
    features.features.shaderFloat64 =
        _capabilities->vkDeviceFeatures.shaderFloat64;
    features.features.fillModeNonSolid =
        _capabilities->vkDeviceFeatures.fillModeNonSolid;
    features.features.alphaToOne =
        _capabilities->vkDeviceFeatures.alphaToOne;

    // Needed to write to storage buffers from vertex shader (eg. GPU culling).
    features.features.vertexPipelineStoresAndAtomics =
        _capabilities->vkDeviceFeatures.vertexPipelineStoresAndAtomics;
    // Needed to write to storage buffers from fragment shader (eg. OIT).
    features.features.fragmentStoresAndAtomics =
        _capabilities->vkDeviceFeatures.fragmentStoresAndAtomics;
    // Needed for buffer address feature
    features.features.shaderInt64 =
        _capabilities->vkDeviceFeatures.shaderInt64;
    // Needed for gl_primtiveID
    features.features.geometryShader =
        _capabilities->vkDeviceFeatures.geometryShader;

    VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = (uint32_t) extensions.size();
    createInfo.pNext = &features;

    TF_VERIFY(
        vkCreateDevice(
            _vkPhysicalDevice,
            &createInfo,
            HgiVulkanAllocator(),
            &_vkDevice) == VK_SUCCESS
    );

    volkLoadDevice(_vkDevice);

    HgiVulkanSetupDeviceDebug(instance, this);

    //
    // Extension function pointers
    //

    vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)
    vkGetDeviceProcAddr(_vkDevice, "vkCreateRenderPass2KHR");

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

    // If we ever wish to use functions from Vulkan 1.1+ (i.e. use a later 
    // version of VMA), we'll need to add those functions here.
    VmaVulkanFunctions vmaVulkanFunctions = {};
    vmaVulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaVulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vmaVulkanFunctions.vkAllocateMemory = vkAllocateMemory;
    vmaVulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
    vmaVulkanFunctions.vkBindImageMemory = vkBindImageMemory;
    vmaVulkanFunctions.vkCreateBuffer = vkCreateBuffer;
    vmaVulkanFunctions.vkCreateImage = vkCreateImage;
    vmaVulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
    vmaVulkanFunctions.vkDestroyImage = vkDestroyImage;
    vmaVulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vmaVulkanFunctions.vkFreeMemory = vkFreeMemory;
    vmaVulkanFunctions.vkGetBufferMemoryRequirements =
        vkGetBufferMemoryRequirements;
    vmaVulkanFunctions.vkGetImageMemoryRequirements =
        vkGetImageMemoryRequirements;
    vmaVulkanFunctions.vkGetPhysicalDeviceMemoryProperties =
        vkGetPhysicalDeviceMemoryProperties;
    vmaVulkanFunctions.vkGetPhysicalDeviceProperties =
        vkGetPhysicalDeviceProperties;
    vmaVulkanFunctions.vkInvalidateMappedMemoryRanges =
        vkInvalidateMappedMemoryRanges;
    vmaVulkanFunctions.vkMapMemory = vkMapMemory;
    vmaVulkanFunctions.vkUnmapMemory = vkUnmapMemory;
    vmaVulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
    allocatorInfo.pVulkanFunctions = &vmaVulkanFunctions;

    TF_VERIFY(
        vmaCreateAllocator(&allocatorInfo, &_vmaAllocator) == VK_SUCCESS
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
    // Make sure device is idle before destroying objects.
    TF_VERIFY(vkDeviceWaitIdle(_vkDevice) == VK_SUCCESS);

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
    TF_VERIFY(
        vkDeviceWaitIdle(_vkDevice) == VK_SUCCESS
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