//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/sampler.h"
#include "pxr/imaging/hgiVulkan/texture.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    static const uint8_t _descriptorSetCnt = 1;
}

static VkDescriptorSetLayout
_CreateDescriptorSetLayout(
    HgiVulkanDevice* device,
    std::vector<VkDescriptorSetLayoutBinding> const& bindings,
    std::string const& debugName)
{
    // Create descriptor
    VkDescriptorSetLayoutCreateInfo setCreateInfo =
        {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setCreateInfo.bindingCount = (uint32_t) bindings.size();
    setCreateInfo.pBindings = bindings.data();
    setCreateInfo.pNext = nullptr;

    VkDescriptorSetLayout layout = nullptr;
    TF_VERIFY(
        vkCreateDescriptorSetLayout(
            device->GetVulkanDevice(),
            &setCreateInfo,
            HgiVulkanAllocator(),
            &layout) == VK_SUCCESS
    );

    // Debug label
    if (!debugName.empty()) {
        std::string debugLabel = "DescriptorSetLayout " + debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)layout,
            VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
            debugLabel.c_str());
    }

    return layout;
}

HgiVulkanResourceBindings::HgiVulkanResourceBindings(
    HgiVulkanDevice* device,
    HgiResourceBindingsDesc const& desc)
    : HgiResourceBindings(desc)
    , _device(device)
    , _inflightBits(0)
    , _vkDescriptorPool(nullptr)
    , _vkDescriptorSetLayout(nullptr)
    , _vkDescriptorSet(nullptr)
{
    // Initialize the pool sizes for each descriptor type we support
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.resize(HgiBindResourceTypeCount);

    for (size_t i=0; i<HgiBindResourceTypeCount; i++) {
        HgiBindResourceType bt = HgiBindResourceType(i);
        VkDescriptorPoolSize p;
        p.descriptorCount = 0;
        p.type = HgiVulkanConversions::GetDescriptorType(bt);
        poolSizes[i] = p;
    }

    // OpenGL (and Metal) have separate bindings for each buffer and image type.
    // Ubo, ssbo, sampler2D, image all start at bindingIndex 0. So we expect
    // that Hgi clients may specify OpenGL style bindingIndex for each.
    // In Vulkan, bindingIndices are shared (incremented) across all resources.
    // We could split all four into a separate descriptorSet and set the
    // slot=XX in the shader. Instead we keep all resources in one
    // descriptor set and increment all Hgi binding indices here.
    // This assumes that Hgi codeGen does the same for vulkan glsl.
    bool reorder = false;
    std::unordered_set<uint32_t> bindingsVisited;

    for (HgiBufferBindDesc const& b : desc.buffers) {
        if (reorder) break;
        reorder = bindingsVisited.find(b.bindingIndex) != bindingsVisited.end();
        bindingsVisited.insert(b.bindingIndex);
    }
    for (HgiTextureBindDesc const& b : desc.textures) {
        if (reorder) break;
        reorder = bindingsVisited.find(b.bindingIndex) != bindingsVisited.end();
        bindingsVisited.insert(b.bindingIndex);
    }

    //
    // Create DescriptorSetLayout to describe resource bindings.
    //
    // Buffers
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (HgiBufferBindDesc const& b : desc.buffers) {
        VkDescriptorSetLayoutBinding d = {};
        uint32_t bi = reorder ? (uint32_t) bindings.size() : b.bindingIndex;
        d.binding = bi; // binding number in shader stage
        d.descriptorType =
            HgiVulkanConversions::GetDescriptorType(b.resourceType);
        poolSizes[b.resourceType].descriptorCount++;
        d.descriptorCount = (uint32_t) b.buffers.size();
        d.stageFlags = HgiVulkanConversions::GetShaderStages(b.stageUsage);
        d.pImmutableSamplers = nullptr;
        bindings.push_back(std::move(d));
    }

    // Textures

    for (HgiTextureBindDesc const& t : desc.textures) {
        VkDescriptorSetLayoutBinding d = {};
        uint32_t bi = reorder ? (uint32_t) bindings.size() : t.bindingIndex;
        d.binding = bi; // binding number in shader stage
        d.descriptorType =
            HgiVulkanConversions::GetDescriptorType(t.resourceType);
        poolSizes[t.resourceType].descriptorCount++;
        d.descriptorCount = (uint32_t) t.textures.size();
        d.stageFlags = HgiVulkanConversions::GetShaderStages(t.stageUsage);
        d.pImmutableSamplers = nullptr;
        bindings.push_back(std::move(d));
    }

    // Create descriptor set layout
    _vkDescriptorSetLayout =
        _CreateDescriptorSetLayout(_device, bindings, _descriptor.debugName);

    //
    // Create the descriptor pool.
    //
    // XXX For now each resource bindings gets its own pool to allocate its
    // descriptor sets from to simplify multi-threading support.
    for (size_t i=poolSizes.size(); i-- > 0;) {
        // Vulkan validation will complain if any descriptorCount is 0.
        // Instead of removing them we set a minimum of 1. An empty poolSize
        // will not let us create the pool, which prevents us from creating
        // the descriptorSets.
        poolSizes[i].descriptorCount=std::max(poolSizes[i].descriptorCount, 1u);
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = _descriptorSetCnt;
    pool_info.poolSizeCount = (uint32_t) poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    TF_VERIFY(
        vkCreateDescriptorPool(
            _device->GetVulkanDevice(),
            &pool_info,
            HgiVulkanAllocator(),
            &_vkDescriptorPool) == VK_SUCCESS
    );

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string debugLabel = "Descriptor Pool " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkDescriptorPool,
            VK_OBJECT_TYPE_DESCRIPTOR_POOL,
            debugLabel.c_str());
    }

    //
    // Create Descriptor Set
    //
    VkDescriptorSetAllocateInfo allocateInfo =
        {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};

    allocateInfo.descriptorPool = _vkDescriptorPool;
    allocateInfo.descriptorSetCount = _descriptorSetCnt;
    allocateInfo.pSetLayouts = &_vkDescriptorSetLayout;

    TF_VERIFY(
        vkAllocateDescriptorSets(
            _device->GetVulkanDevice(),
            &allocateInfo,
            &_vkDescriptorSet) == VK_SUCCESS
    );

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string dbgLbl = "Descriptor Set Buffers " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            _device,
            (uint64_t)_vkDescriptorSet,
            VK_OBJECT_TYPE_DESCRIPTOR_SET,
            dbgLbl.c_str());
    }

    //
    // Setup limits for each resource type
    //
    VkPhysicalDeviceProperties const& devProps =
        _device->GetDeviceCapabilities().vkDeviceProperties;
    VkPhysicalDeviceLimits const& limits = devProps.limits;

    uint32_t bindLimits[HgiBindResourceTypeCount][2] = {
        {HgiBindResourceTypeSampler,
            limits.maxPerStageDescriptorSamplers},
        {HgiBindResourceTypeSampledImage,
            limits.maxPerStageDescriptorSampledImages},
        {HgiBindResourceTypeCombinedSamplerImage,
            limits.maxPerStageDescriptorSampledImages},
        {HgiBindResourceTypeStorageImage,
            limits.maxPerStageDescriptorStorageImages},
        {HgiBindResourceTypeUniformBuffer,
            limits.maxPerStageDescriptorUniformBuffers},
        {HgiBindResourceTypeStorageBuffer,
            limits.maxPerStageDescriptorStorageBuffers}
    };
    static_assert(HgiBindResourceTypeCount==6, "");

    //
    // Buffers
    //

    std::vector<VkWriteDescriptorSet> writeSets;

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(desc.buffers.size());

    for (HgiBufferBindDesc const& bufDesc : desc.buffers) {
        uint32_t & limit = bindLimits[bufDesc.resourceType][1];
        if (!TF_VERIFY(limit>0, "Maximum size array-of-buffers exceeded")) {
            break;
        }
        limit -= 1;

        TF_VERIFY(bufDesc.buffers.size() == bufDesc.offsets.size());

        // Each buffer can be an array of buffers (usually one)
        for (size_t i=0; i<bufDesc.buffers.size(); i++) {
            HgiBufferHandle const& bufHandle = bufDesc.buffers[i];
            HgiVulkanBuffer* buf =
                static_cast<HgiVulkanBuffer*>(bufHandle.Get());
            if (!TF_VERIFY(buf)) continue;
            VkDescriptorBufferInfo bufferInfo;
            bufferInfo.buffer = buf->GetVulkanBuffer();
            bufferInfo.offset = bufDesc.offsets[i];
            bufferInfo.range = VK_WHOLE_SIZE;
            bufferInfos.push_back(std::move(bufferInfo));
        }
    }

    size_t bufInfoOffset = 0;
    for (HgiBufferBindDesc const& bufDesc : desc.buffers) {
        VkWriteDescriptorSet writeSet= {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeSet.dstBinding = reorder ? // index in descriptor set
            (uint32_t) writeSets.size() :
            bufDesc.bindingIndex;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = (uint32_t) bufDesc.buffers.size(); // 0 ok
        writeSet.dstSet = _vkDescriptorSet;
        writeSet.pBufferInfo = bufferInfos.data() + bufInfoOffset;
        writeSet.pImageInfo = nullptr;
        writeSet.pTexelBufferView = nullptr;
        writeSet.descriptorType =
            HgiVulkanConversions::GetDescriptorType(bufDesc.resourceType);
        writeSets.push_back(std::move(writeSet));
        bufInfoOffset += bufDesc.buffers.size();
    }

    //
    // Textures
    //

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(desc.textures.size());

    for (HgiTextureBindDesc const& texDesc : desc.textures) {

        uint32_t & limit = bindLimits[texDesc.resourceType][1];
        if (!TF_VERIFY(limit>0, "Maximum array-of-texture/samplers exceeded")) {
            break;
        }
        limit -= 1;

        // Each texture can be an array of textures
        for (size_t i=0; i< texDesc.textures.size(); i++) {
            HgiTextureHandle const& texHandle = texDesc.textures[i];
            HgiVulkanTexture* tex =
                static_cast<HgiVulkanTexture*>(texHandle.Get());
            if (!TF_VERIFY(tex)) continue;

            // Not having a sampler is ok only for StorageImage.
            HgiVulkanSampler* smp = nullptr;
            if (i < texDesc.samplers.size()) {
                HgiSamplerHandle const& smpHandle = texDesc.samplers[i];
                smp = static_cast<HgiVulkanSampler*>(smpHandle.Get());
            }

            VkDescriptorImageInfo imageInfo;
            imageInfo.sampler = smp ? smp->GetVulkanSampler() : nullptr;
            imageInfo.imageLayout = tex->GetImageLayout();
            imageInfo.imageView = tex->GetImageView();
            imageInfos.push_back(std::move(imageInfo));
        }
    }

    size_t texInfoOffset = 0;
    for (HgiTextureBindDesc const& texDesc : desc.textures) {
        // For dstBinding we must provided an index in descriptor set.
        // Must be one of the bindings specified in VkDescriptorSetLayoutBinding
        VkWriteDescriptorSet writeSet= {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeSet.dstBinding = reorder ? // index in descriptor set
            (uint32_t) writeSets.size() :
            texDesc.bindingIndex;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = (uint32_t) texDesc.textures.size(); // 0 ok
        writeSet.dstSet = _vkDescriptorSet;
        writeSet.pBufferInfo = nullptr;
        writeSet.pImageInfo = imageInfos.data() + texInfoOffset;
        writeSet.pTexelBufferView = nullptr;
        writeSet.descriptorType =
            HgiVulkanConversions::GetDescriptorType(texDesc.resourceType);
        writeSets.push_back(std::move(writeSet));
        texInfoOffset += texDesc.textures.size();
    }

    // Note: this update is immediate. It is not recorded via a command.
    // This means we should only do this if the descriptorSet is not currently
    // in use on GPU. With 'descriptor indexing' extension this has relaxed a
    // little and we are allowed to use vkUpdateDescriptorSets before
    // vkBeginCommandBuffer and after vkEndCommandBuffer, just not during the
    // command buffer recording.
    vkUpdateDescriptorSets(
        _device->GetVulkanDevice(),
        (uint32_t) writeSets.size(),
        writeSets.data(),
        0,        // copy count
        nullptr); // copy_desc

}

HgiVulkanResourceBindings::~HgiVulkanResourceBindings()
{
    vkDestroyDescriptorSetLayout(
        _device->GetVulkanDevice(),
        _vkDescriptorSetLayout,
        HgiVulkanAllocator());

    // Since we have one pool for this resourceBindings we can reset the pool
    // instead of freeing the descriptorSets (vkFreeDescriptorSets).
    vkDestroyDescriptorPool(
        _device->GetVulkanDevice(),
        _vkDescriptorPool,
        HgiVulkanAllocator());
}

void
HgiVulkanResourceBindings::BindResources(
    VkCommandBuffer cb,
    VkPipelineBindPoint bindPoint,
    VkPipelineLayout layout)
{
    // When binding new resources for the currently bound pipeline it may
    // 'disturb' previously bound resources (for a previous pipeline) that
    // are no longer compatible with the layout for the new pipeline.
    // This essentially unbinds the old resources.

    vkCmdBindDescriptorSets(
        cb,
        bindPoint,
        layout,
        0, // firstSet/slot - Hgi does not provide slot index, assume 0.
        _descriptorSetCnt,
        &_vkDescriptorSet,
        0, // dynamicOffset
        nullptr);
}

HgiVulkanDevice*
HgiVulkanResourceBindings::GetDevice() const
{
    return _device;
}

uint64_t &
HgiVulkanResourceBindings::GetInflightBits()
{
    return _inflightBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
