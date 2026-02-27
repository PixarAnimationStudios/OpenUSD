//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"

#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>

#include <cstring>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HGIVULKAN_DEBUG, 0, "Enable debugging for HgiVulkan. "
    "1: Enable Validation Layer. "
    ">1: Only enable debug logging/tagging (for other tools)");
TF_DEFINE_ENV_SETTING(HGIVULKAN_DEBUG_VERBOSE, 0,
    "Enable verbose debugging for HgiVulkan");

bool
HgiVulkanIsDebugEnabled()
{
    static bool _v = TfGetEnvSetting(HGIVULKAN_DEBUG) >= 1;
    return _v;
}

bool
HgiVulkanIsValidationEnabled()
{
    static bool _v = TfGetEnvSetting(HGIVULKAN_DEBUG) == 1;
    return _v;
}

bool
HgiVulkanIsVerboseDebugEnabled()
{
    static bool _v = TfGetEnvSetting(HGIVULKAN_DEBUG_VERBOSE) == 1;
    return _v;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
_VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT msgType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    using namespace std::literals::string_view_literals;
    static constexpr auto ignoredMessages = {
        // This warning happens because render passes such as OIT and volume do 
        // not write to the attachments (i.e. color AOV) specified in the 
        // graphics pipeline.
        "Validation Warning: [ Undefined-Value-ShaderInputNotProduced ]"sv,
        // This warning happens during render passes, such as the shadow pass, 
        // in which we write to a shader output (e.g. color) with no 
        // corresponding attachment.
        "Validation Warning: [ Undefined-Value-ShaderOutputNotConsumed ]"sv,
    };

    // If verbose debug is not enabled, we ignore messages included in 
    // ignoredMessages.
    const std::string_view message(callbackData->pMessage);
    if (!HgiVulkanIsVerboseDebugEnabled() && std::any_of(
        ignoredMessages.begin(), ignoredMessages.end(),
        [message](const std::string_view ignoredMessage) {
            return message.rfind(ignoredMessage, 0) == 0; })) {
        return VK_FALSE;
    }

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        TF_CODING_ERROR("VULKAN_ERROR: %s\n", callbackData->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        TF_WARN("VULKAN_WARNING: %s\n", callbackData->pMessage);
    } else {
        TF_STATUS("VULKAN_MESSAGE: %s\n", callbackData->pMessage);
    }

    return VK_FALSE;
}

void
HgiVulkanCreateDebug(HgiVulkanInstance* instance)
{
    if (!HgiVulkanIsDebugEnabled()) {
        return;
    }

    VkInstance vkInstance = instance->GetVulkanInstance();

    instance->vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            vkInstance,
            "vkCreateDebugUtilsMessengerEXT");

    instance->vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            vkInstance,
            "vkDestroyDebugUtilsMessengerEXT");

    if (!TF_VERIFY(instance->vkCreateDebugUtilsMessengerEXT)) {
        return;
    }
    if (!TF_VERIFY(instance->vkDestroyDebugUtilsMessengerEXT)) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT dbgMsgCreateInfo =
        {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    dbgMsgCreateInfo.pNext = nullptr;
    dbgMsgCreateInfo.flags = 0;
    dbgMsgCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    // Verbose debugging will cause many bits of information to be printed by
    // the vulkan validation layers. It is only useful for debugging.
    if (HgiVulkanIsVerboseDebugEnabled()) {
        dbgMsgCreateInfo.messageSeverity |=
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    }

    dbgMsgCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    dbgMsgCreateInfo.pfnUserCallback = _VulkanDebugCallback;
    dbgMsgCreateInfo.pUserData = nullptr;

    HGIVULKAN_VERIFY_VK_RESULT(
        instance->vkCreateDebugUtilsMessengerEXT(
            vkInstance,
            &dbgMsgCreateInfo,
            HgiVulkanAllocator(),
            &instance->vkDebugMessenger)
    );
}

void
HgiVulkanDestroyDebug(HgiVulkanInstance* instance)
{
    if (!HgiVulkanIsDebugEnabled()) {
        return;
    }

    VkInstance vkInstance = instance->GetVulkanInstance();

    if (!TF_VERIFY(instance->vkDestroyDebugUtilsMessengerEXT)) {
        return;
    }

    instance->vkDestroyDebugUtilsMessengerEXT(
        vkInstance, instance->vkDebugMessenger, HgiVulkanAllocator());
}

void
HgiVulkanSetupDeviceDebug(
    HgiVulkanInstance* instance,
    HgiVulkanDevice* device)
{
    VkInstance vkInstance = instance->GetVulkanInstance();
    device->vkCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
        vkInstance,
        "vkCmdBeginDebugUtilsLabelEXT");

    device->vkCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
        vkInstance,
        "vkCmdEndDebugUtilsLabelEXT");

    device->vkCmdInsertDebugUtilsLabelEXT =
        (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(
        vkInstance,
        "vkCmdInsertDebugUtilsLabelEXT");

    device->vkSetDebugUtilsObjectNameEXT =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(
        vkInstance,
        "vkSetDebugUtilsObjectNameEXT");

    device->vkQueueBeginDebugUtilsLabelEXT =
        (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(
        vkInstance,
        "vkQueueBeginDebugUtilsLabelEXT");

    device->vkQueueEndDebugUtilsLabelEXT =
        (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(
        vkInstance,
        "vkQueueEndDebugUtilsLabelEXT");
}

void
HgiVulkanSetDebugName(
    HgiVulkanDevice* device,
    uint64_t vulkanObject,
    VkObjectType objectType,
    const char* name)
{
    if (!HgiVulkanIsDebugEnabled() || !name) {
        return;
    }

    if (!TF_VERIFY(device && device->vkSetDebugUtilsObjectNameEXT)) {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT debugInfo =
        {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    debugInfo.objectHandle = vulkanObject;
    debugInfo.objectType = objectType;
    debugInfo.pObjectName = name;
    device->vkSetDebugUtilsObjectNameEXT(device->GetVulkanDevice(), &debugInfo);
}


void
HgiVulkanBeginLabel(
    HgiVulkanDevice* device,
    HgiVulkanCommandBuffer* cb,
    const char* label,
    const GfVec4f& color)
{
    if (!HgiVulkanIsDebugEnabled() || !label) {
        return;
    }

    if (!TF_VERIFY(device && device->vkCmdBeginDebugUtilsLabelEXT)) {
        return;
    }

    VkCommandBuffer vkCmbuf = cb->GetVulkanCommandBuffer();
    VkDebugUtilsLabelEXT labelInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    labelInfo.pLabelName = label;
    labelInfo.color[0] = color[0];
    labelInfo.color[1] = color[1];
    labelInfo.color[2] = color[2];
    labelInfo.color[3] = color[3];
    device->vkCmdBeginDebugUtilsLabelEXT(vkCmbuf, &labelInfo);
}

void
HgiVulkanEndLabel(
    HgiVulkanDevice* device,
    HgiVulkanCommandBuffer* cb)
{
    if (!HgiVulkanIsDebugEnabled()) {
        return;
    }

    if (!TF_VERIFY(device && device->vkCmdEndDebugUtilsLabelEXT)) {
        return;
    }

    VkCommandBuffer vkCmbuf = cb->GetVulkanCommandBuffer();
    device->vkCmdEndDebugUtilsLabelEXT(vkCmbuf);
}

void
HgiVulkanInsertDebugMarker(
    HgiVulkanDevice* device,
    HgiVulkanCommandBuffer* cb,
    const char* label,
    const GfVec4f& color)
{
    if (!HgiVulkanIsDebugEnabled() || !label) {
        return;
    }

    if (!TF_VERIFY(device && device->vkCmdInsertDebugUtilsLabelEXT)) {
        return;
    }

    VkCommandBuffer vkCmbuf = cb->GetVulkanCommandBuffer();
    VkDebugUtilsLabelEXT labelInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    labelInfo.pLabelName = label;
    labelInfo.color[0] = color[0];
    labelInfo.color[1] = color[1];
    labelInfo.color[2] = color[2];
    labelInfo.color[3] = color[3];
    device->vkCmdInsertDebugUtilsLabelEXT(vkCmbuf, &labelInfo);
}

void
HgiVulkanBeginQueueLabel(
    HgiVulkanDevice* device,
    const char* label)
{
    if (!HgiVulkanIsDebugEnabled() || !label) {
        return;
    }

    if (!TF_VERIFY(device && device->vkQueueBeginDebugUtilsLabelEXT)) {
        return;
    }

    VkQueue gfxQueue = device->GetCommandQueue()->GetVulkanGraphicsQueue();
    VkDebugUtilsLabelEXT labelInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    labelInfo.pLabelName = label;
    device->vkQueueBeginDebugUtilsLabelEXT(gfxQueue, &labelInfo);
}

void
HgiVulkanEndQueueLabel(HgiVulkanDevice* device)
{
    if (!HgiVulkanIsDebugEnabled()) {
        return;
    }

    if (!TF_VERIFY(device && device->vkQueueEndDebugUtilsLabelEXT)) {
        return;
    }

    VkQueue gfxQueue = device->GetCommandQueue()->GetVulkanGraphicsQueue();
    device->vkQueueEndDebugUtilsLabelEXT(gfxQueue);
}

const char*
HgiVulkanResultString(VkResult result)
{
    return string_VkResult(result);
}

PXR_NAMESPACE_CLOSE_SCOPE