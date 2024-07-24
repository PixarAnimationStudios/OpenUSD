//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/computeCmdsDesc.h"
#include "pxr/imaging/hgiVulkan/blitCmds.h"
#include "pxr/imaging/hgiVulkan/commandBuffer.h"
#include "pxr/imaging/hgiVulkan/commandQueue.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/graphicsCmds.h"
#include "pxr/imaging/hgiVulkan/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/shaderCompiler.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hio/image.h"

#include "pxr/base/tf/errorMark.h"
#include <iostream>
#include <string>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static const int _imgSize = 512;
static const HgiFormat _imgFormat = HgiFormatUNorm8Vec4;
static const HioFormat _imgHioFormat = HioFormatUNorm8Vec4;

static bool
TestVulkanInstance(HgiVulkan& hgiVulkan)
{
    HgiVulkanInstance* instance = hgiVulkan.GetVulkanInstance();
    if (!instance) {
        return false;
    }

    // Make sure debug fn ptr were found
    if (!instance->vkDebugMessenger || 
        !instance->vkCreateDebugUtilsMessengerEXT ||
        !instance->vkDestroyDebugUtilsMessengerEXT)
    {
        TF_CODING_ERROR("Instance function ptrs failed");
        return false;
    }

    // Make sure vulkan instance could be created
    if (!instance->GetVulkanInstance()) {
        TF_CODING_ERROR("vkInstance failed");
        return false;
    }
    return true;
}

static bool
TestVulkanDevice(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    // Make sure fn ptr were found
    if (!device->vkCreateRenderPass2KHR ||
        !device->vkCmdBeginDebugUtilsLabelEXT ||
        !device->vkCmdEndDebugUtilsLabelEXT ||
        !device->vkCmdInsertDebugUtilsLabelEXT ||
        !device->vkSetDebugUtilsObjectNameEXT) {
        TF_CODING_ERROR("Device function ptrs failed");
        return false;
    }

    // Make sure vulkan device could be created
    if (!device->GetVulkanDevice() || !device->GetVulkanMemoryAllocator()) {
        TF_CODING_ERROR("vkDevice failed");
        return false;
    }

    return true;
}

static bool
TestVulkanShaderCompiler(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    // Test push constants, scalar buffer layout, storage buffer, sampler arrays
    static const char* frag = 
        "#version 450 \n"
        "#extension GL_EXT_nonuniform_qualifier : require \n"
        "#extension GL_EXT_scalar_block_layout : require \n"
        "#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require \n"
        ""
        "layout(push_constant) uniform PushConstantBuffer { \n"
        "    layout(offset = 0) int textureIndex; \n"
        "} pushConstants; \n"
        ""
        "layout (scalar, set=0, binding=0) buffer StorageBuffer { \n"
        "    vec3 value[]; \n"
        "} storageBuffer; \n"
        ""
        "layout(set=0, binding=1) uniform sampler2DArray samplers2D[]; \n"
        ""
        "layout(location = 0) in vec2 texcoordIn; \n"
        "layout(location = 0) out vec4 outputColor; \n"
        ""
        "layout(early_fragment_tests) in; \n"
        ""
        "void main() { \n"
        "    int idx = pushConstants.textureIndex;\n"
        "    outputColor = texture( \n"
        "        samplers2D[nonuniformEXT(idx)], vec3(texcoordIn, 0)); \n"
        "    outputColor.a = storageBuffer.value[0].x;"
        "} \n";

    std::vector<unsigned int> spirv;
    std::string errors;
    bool result = HgiVulkanCompileGLSL(
        "TestFrag",
        &frag, 1, 
        HgiShaderStageFragment, 
        &spirv, 
        &errors);

    if (!result || !errors.empty() || spirv.empty()) {
        TF_CODING_ERROR("Vulkan shader compiler error(s):\n%s", errors.c_str());
        return false;
    }

    return true;
}

static bool
TestVulkanCommandQueue(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    // Make sure we have a valid graphics device queue
    HgiVulkanCommandQueue* queue = device->GetCommandQueue();
    if (!queue || !queue->GetVulkanGraphicsQueue()) {
        TF_CODING_ERROR("vkQueue failed");
        return false;
    }

    bool result = true;
    hgiVulkan.StartFrame();

    // The goal of the below test is validate that when Cmds objects are created
    // on the main thread, they can be safely used in a secondary thread.
    // We expect that the command buffer that the Cmds object acquires will not
    // be acquired until the first command is recorded so we can be sure the
    // command buffer is grabbed from the secondary threads command pool.
    // Hgi states that each backend must support recording commands on threads.
    // We also test various internal state of command queue and its ability
    // to reuse command buffers when they have been consumed by the gpu.

    HgiBlitCmdsUniquePtr blitCmdsA = hgiVulkan.CreateBlitCmds();
    HgiBlitCmdsUniquePtr blitCmdsB = hgiVulkan.CreateBlitCmds();
    HgiBlitCmdsUniquePtr blitCmdsY = hgiVulkan.CreateBlitCmds();
    HgiBlitCmdsUniquePtr blitCmdsZ = hgiVulkan.CreateBlitCmds();

    // We perform two jobs on each thread. The atomic keeps track of progress
    enum class Job {FirstJob, SecondJob, Wait, Quit};
    std::atomic<Job> status0{Job::Wait};
    std::atomic<Job> status1{Job::Wait};

    auto work = [](
        HgiBlitCmds* cmds0,
        HgiBlitCmds* cmds1, 
        std::atomic<Job>* status) 
    {
        Job job = Job::Wait;
        while(job != Job::Quit) {
            job = status->load();
            // Perform first job on thread and wait (spin) until next job
            if (job==Job::FirstJob) {
                cmds0->PushDebugGroup("First Job");
                cmds0->PopDebugGroup();
                status->store(Job::Wait);
            }
            // Perform second job and quit thread.
            if (job==Job::SecondJob) {
                cmds1->PushDebugGroup("Second Job");
                cmds1->PopDebugGroup();
                status->store(Job::Quit);
            }
        }
    };

    // Start job threads
    std::thread thread0(work, blitCmdsA.get(), blitCmdsB.get(), &status0);
    std::thread thread1(work, blitCmdsY.get(), blitCmdsZ.get(), &status1);

    // Begin first job on both threads
    status0.store(Job::FirstJob);
    status1.store(Job::FirstJob);

    // Wait for first job to finish recording.
    while(status0.load() != Job::Wait || status1.load() != Job::Wait);

    // After recoding we expect the cmd buffers to be in 'IsRecording' mode.
    // It will remain in this mode until we have used SubmitCmds and the
    // command buffer has been consumed by the GPU.
    HgiVulkanBlitCmds* vkBlitCmdsA = 
        static_cast<HgiVulkanBlitCmds*>(blitCmdsA.get());
    HgiVulkanCommandBuffer* cbA = vkBlitCmdsA->GetCommandBuffer();

    HgiVulkanBlitCmds* vkBlitCmdsY = 
        static_cast<HgiVulkanBlitCmds*>(blitCmdsY.get());
    HgiVulkanCommandBuffer* cbY = vkBlitCmdsY->GetCommandBuffer();

    if (!cbA->IsInFlight() || !cbY->IsInFlight()) {
        TF_CODING_ERROR("Command buffer is not in-flight");
        result = false;
    }

    // After having finished the first job on two threads we expect that
    // two commands buffers have been used. We expect the first in-flight bits
    // to have been switched on, indidcating that two commands buffers are
    // in-flight. This is important for garbage collection.
    uint64_t inflightBits1 = queue->GetInflightCommandBuffersBits();
    uint64_t expectedBits1 = (1 << 0) | (1 << 1);
    if (inflightBits1 != expectedBits1) {
        TF_CODING_ERROR("Inflight bits invalid (1)");
        result = false;
    }

    // Similarly we expect the command buffers 'inflightId` to be set.
    // We can't be sure if the command buffer A received the first inflight id
    // or the second. It depends on what job thread got to the atomic first.
    // One of them will be id 0 and one will be id 1.
    if (cbA->GetInflightId() == cbY->GetInflightId()) {
        TF_CODING_ERROR("Inflight id invalid (1)");
        result = false;
    }

    // Submit the first job commands of both threads.
    // After submitting, we cannot reuse those Hgi***Cmds.
    hgiVulkan.SubmitCmds(blitCmdsA.get());
    hgiVulkan.SubmitCmds(blitCmdsY.get());

    // Wait for all command buffers to have been consumed before starting
    // the second job. Normally we would not do this as it stalls the CPU and
    // starves the GPU. But we do it here to test if the command buffers of
    // the first job get reused for the second. They should be because by
    // waiting for the device to finish, they should be available again.
    device->WaitForIdle();

    // EndFrame resets consumed command buffer so they can be reused.
    hgiVulkan.EndFrame();

    // Begin second job on both threads.
    status0.store(Job::SecondJob);
    status1.store(Job::SecondJob);

    // Wait for second job to finish.
    while(status0.load() != Job::Quit || status1.load() != Job::Quit);

    // Job threads are done.
    thread0.join();
    thread1.join();

    // We submit the second job's commands buffer directly to the queue.
    // Normally they are submitted via Hgi::SubmitCmds, but here we want to
    // test the 'WAIT' feature to block until all command buffers have been
    // consumed.
    HgiVulkanBlitCmds* vkBlitCmdsB = 
        static_cast<HgiVulkanBlitCmds*>(blitCmdsB.get());
    HgiVulkanBlitCmds* vkBlitCmdsZ = 
        static_cast<HgiVulkanBlitCmds*>(blitCmdsZ.get());

    queue->SubmitToQueue(vkBlitCmdsB->GetCommandBuffer(), 
        HgiSubmitWaitTypeWaitUntilCompleted);
    queue->SubmitToQueue(vkBlitCmdsZ->GetCommandBuffer(), 
        HgiSubmitWaitTypeWaitUntilCompleted);

    // Since we used WaitForIdle after the first job, we expect the vulkan
    // command buffer of the first and second jobs to be the same.
    // However, we don't know for sure which thread started its second job
    // first, so we account for that.
    VkCommandBuffer a=vkBlitCmdsA->GetCommandBuffer()->GetVulkanCommandBuffer();
    VkCommandBuffer y=vkBlitCmdsY->GetCommandBuffer()->GetVulkanCommandBuffer();
    VkCommandBuffer b=vkBlitCmdsB->GetCommandBuffer()->GetVulkanCommandBuffer();
    VkCommandBuffer z=vkBlitCmdsZ->GetCommandBuffer()->GetVulkanCommandBuffer();

    bool reused = (a==b || a==z) && (y==b || y==z);
    if (!reused) {
        TF_CODING_ERROR("Command buffers were not reused");
        result = false;
    }

    // Since we used WaitForIdle after the first job, we expect that the
    // in-flight bits of the first job have been reset. We also expect that
    // two new bits have been flagged as in-flight.
    // So every time a command buffer is reused for recording, it will receive
    // a new inflight id. This makes sense for garbage collection, because we
    // only want to delay object destruction for the command buffers that were
    // in-flight at the time of the DestroyObject request. If the command buffer
    // is later reused, it will not have the same objects in-use and so a new
    // bit is issued for the new round of recording.
    uint64_t inflightBits2 = queue->GetInflightCommandBuffersBits();
    uint64_t expectedBits2 = (1 << 2) | (1 << 3);
    if (inflightBits2 != expectedBits2) {
        TF_CODING_ERROR("Inflight bits invalid (2)");
        result = false;
    }

    HgiVulkanCommandBuffer* cbB = vkBlitCmdsB->GetCommandBuffer();
    HgiVulkanCommandBuffer* cbZ = vkBlitCmdsZ->GetCommandBuffer();
    if (cbB->GetInflightId() == cbZ->GetInflightId() && 
        cbB->GetInflightId() > 1 && cbZ->GetInflightId() > 1) {
        TF_CODING_ERROR("Inflight id invalid (2)");
        result = false;
    }

    // The commands buffers of all jobs should be consumed by now and we expect
    // the gpu device to be idle. But for good measure, make absolutely sure.
    device->WaitForIdle();
    return result;
}

static bool
TestVulkanGarbageCollection(HgiVulkan& hgiVulkan)
{
    // The goal of the below test is to verify that garbage collection works
    // correctly. Destruction of objects is delayed until the in-flight command
    // buffers have been consumed by the GPU.

    // Create a second Hgi to test that the thread_local setup in the
    // garbage collector works correctly when multiple Hgi are in play.
    // The garbage collector uses statics / thread_local storage.
    HgiVulkan hgiVulkan2;

    // Create two shaders for us to delete via garbage collection
    HgiShaderFunctionDesc desc;
    desc.shaderStage = HgiShaderStageCompute;
    desc.shaderCode = 
        "void main() { \n"
        "   bool empty = true; \n"
        "} \n";

    desc.debugName = "Shader0";
    HgiShaderFunctionHandle shader0 = hgiVulkan2.CreateShaderFunction(desc);
    if (!shader0 || !shader0->IsValid()) {
        std::string const& error = shader0->GetCompileErrors();
        TF_CODING_ERROR("TestVulkanGarbageCollection failed %s", error.c_str());
        return false;
    }

    desc.debugName = "Shader1";
    HgiShaderFunctionHandle shader1 = hgiVulkan2.CreateShaderFunction(desc);
    if (!shader1 || !shader1->IsValid()) {
        std::string const& error = shader1->GetCompileErrors();
        TF_CODING_ERROR("TestVulkanGarbageCollection failed %s", error.c_str());
        return false;
    }

    // Store the vulkan handle of shader1 for later comparison
    HgiVulkanShaderFunction* vkShader1 = 
        static_cast<HgiVulkanShaderFunction*>(shader1.Get());
    VkShaderModule shader1VkModule = vkShader1->GetShaderModule();

    // Create two BlitCmds.
    HgiBlitCmdsUniquePtr blitCmds0 = hgiVulkan2.CreateBlitCmds();
    HgiBlitCmdsUniquePtr blitCmds1 = hgiVulkan2.CreateBlitCmds();

    // Start recording commands in BlitCmds0.
    // This means the command buffer inside the Cmds is now 'in-flight'.
    blitCmds0->PushDebugGroup("BlitCmds0");
    blitCmds0->PopDebugGroup();

    // Schedule destruction of the first shader.
    // This obj now has a 'dependency' on the BlitCmds0, because it's in-flight.
    // It will only be destroyed once blitCmds0 has been consumed by GPU.
    hgiVulkan2.DestroyShaderFunction(&shader0);

    // Start recording commands in BlitCmds1.
    // This means the command buffer inside the Cmds is now 'in-flight'.
    blitCmds1->PushDebugGroup("BlitCmds1");
    blitCmds1->PopDebugGroup();

    // Schedule destruction of the second shader.
    // This obj now has a 'dependency' on the in-flight BlitCmds0 AND BlitCmds1,
    // because both Cmds are still in-flight (none of have consumed yet).
    hgiVulkan2.DestroyShaderFunction(&shader1);

    // shader0 and shader1 should now we waiting for destruction in collector.
    HgiVulkanGarbageCollector* gc = hgiVulkan2.GetGarbageCollector();
    HgiVulkanShaderFunctionVector* shaderGarbage0 = gc->GetShaderFunctionList();
    if (shaderGarbage0->size() != 2) {
        TF_CODING_ERROR("We expected two objects in garbage collector");
        return false;
    }

    // Submit BlitCmds0 to queue for GPU consumption and wait for completion.
    hgiVulkan2.SubmitCmds(blitCmds0.get());
    HgiVulkanDevice* device = hgiVulkan2.GetPrimaryDevice();
    device->WaitForIdle();

    // If there are no calls to Hgi::StartFrame then the garbage collector runs
    // after SubmitCmds. So we submit another BlitCmds to trigger the garbage
    // collector to run. Which should then cause shader0 to be destroyed since
    // it was waiting on BlitCmds0 to no longer be  in-flight.
    HgiBlitCmdsUniquePtr blitCmdsX = hgiVulkan2.CreateBlitCmds();
    blitCmdsX->PushDebugGroup("BlitCmdsX");
    blitCmdsX->PopDebugGroup();
    hgiVulkan2.SubmitCmds(blitCmdsX.get());

    // We now expect the garbage collector to have run and shader0 to be have
    // been destroyed. Shader1 should still be in the garbage collector since
    // BlitCmds1 has not been consumed yet.
    HgiVulkanShaderFunctionVector* shaderGarbage1 = gc->GetShaderFunctionList();
    if (shaderGarbage1->size() != 1) {
        TF_CODING_ERROR("We expected one object in garbage collector");
        return false;
    }

    if (shader1VkModule != shaderGarbage1->front()->GetShaderModule()) {
        TF_CODING_ERROR("We expected shader1 in garbage collector");
        return false;
    }

    // Create and destroy another object but in the original Hgi this time.
    desc.debugName = "ShaderOriginalHgi";
    HgiShaderFunctionHandle shaderOrg = hgiVulkan.CreateShaderFunction(desc);
    if (!shaderOrg || !shaderOrg->IsValid()) {
        std::string const& error = shaderOrg->GetCompileErrors();
        TF_CODING_ERROR("TestVulkanGarbageCollection failed %s", error.c_str());
        return false;
    }
    hgiVulkan.DestroyShaderFunction(&shaderOrg);

    // We expected two objects in the garbage collector (one for each Hgi)
    HgiVulkanShaderFunctionVector* shaderGcOrg = gc->GetShaderFunctionList();
    if (shaderGcOrg->size() != 2) {
        TF_CODING_ERROR("We expected two objects in garbage collector");
        return false;
    }

    // Submit BlitCmds1 to queue for GPU consumption and wait for completion.
    hgiVulkan2.SubmitCmds(blitCmds1.get());
    device->WaitForIdle();

    // All command buffers are expected to have been consumed since we
    // WaitForIdle above. Resetting them should allow the garbage collector
    // below to destroy all remaining objects.
    device->GetCommandQueue()->ResetConsumedCommandBuffers();
    if (device->GetCommandQueue()->GetInflightCommandBuffersBits() != 0) {
        TF_CODING_ERROR("Not all command buffers were reset");
        return false;
    }

    // Call EndFrame to trigger garbage collection (alternate to SubmitCmds).
    // This only clears the garbage of the devices of hgiVulkan2.
    hgiVulkan2.StartFrame();
    hgiVulkan2.EndFrame();

    HgiVulkanShaderFunctionVector* shaderGarbageEnd =
        gc->GetShaderFunctionList();
    if (shaderGarbageEnd->size() != 1) {
        TF_CODING_ERROR("We expected the object of the original Hgi to "
                        "remain in the garbage collector.");
        return false;
    }

    // Call EndFrame to trigger garbage collection of the original Hgi.
    // This should now remove the 'shaderOrg' object from garbage collector.
    hgiVulkan.StartFrame();
    hgiVulkan.EndFrame();

    HgiVulkanShaderFunctionVector* shaderGarbageEndOrg =
        gc->GetShaderFunctionList();
    if (!shaderGarbageEndOrg->empty()) {
        TF_CODING_ERROR("We expected the garbage collector to be empty");
        return false;
    }

    return true;
}

static bool
TestVulkanBuffer(HgiVulkan& hgiVulkan)
{
    // The goal of this test is to validate vulkan buffer creation and verify
    // data is uploaded correctly.

    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    hgiVulkan.StartFrame();

    // Create test data and buffer descriptor
    std::vector<uint32_t> blob(16, 123);
    HgiBufferDesc desc;
    desc.debugName = "TestBuffer";
    desc.byteSize = blob.size() * sizeof(blob[0]);
    desc.initialData = blob.data();
    desc.usage = HgiBufferUsageStorage;

    // Create the buffer
    HgiBufferHandle buffer = hgiVulkan.CreateBuffer(desc);
    if (!buffer) {
        TF_CODING_ERROR("Invalid buffer");
        return false;
    }

    if (buffer->GetByteSizeOfResource() != desc.byteSize) {
        TF_CODING_ERROR("Incorrect GetByteSizeOfResource");
        return false;
    }

    // Buffer data is uploaded via 'staging buffers'.
    // A staging buffer then transfers the data to the device-local gpu buffer.
    // This transfer happens via (internal) resource command buffers.
    // The resource command buffers are submitted before Hgi*Cmds are submitted.
    // So we need to submit at least one Hgi*Cmds before the transfer completes.
    // We want a GpuToCpu read-back anyway, so that works out ok.
    std::vector<uint32_t> readbackBlob(blob.size(), 0);

    HgiBufferGpuToCpuOp copyOp;
    copyOp.byteSize = desc.byteSize;
    copyOp.cpuDestinationBuffer = &readbackBlob[0];
    copyOp.destinationByteOffset = 0;
    copyOp.gpuSourceBuffer = buffer;
    copyOp.sourceByteOffset = 0;

    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan.CreateBlitCmds();
    blitCmds->CopyBufferGpuToCpu(copyOp);

    // Submit BlitCmds, this should automatically trigger the submission of
    // the internal resource command buffer(s) that has recorded the transfer
    // of the staging buffer data to the device-local gpu buffer.
    hgiVulkan.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    if (readbackBlob != blob) {
        TF_CODING_ERROR("Read back of initialData failed");
        return false;
    }

    // Write new data into CPU staging area
    std::vector<uint32_t> stagingBlob(blob.size(), 456);
    void* cpuAddress = buffer->GetCPUStagingAddress();
    memcpy(cpuAddress, stagingBlob.data(), desc.byteSize);

    // Schedule copy from staging area to GPU device-local buffer.
    HgiBufferCpuToGpuOp transferOp;
    transferOp.byteSize = desc.byteSize;
    transferOp.cpuSourceBuffer = cpuAddress;
    transferOp.sourceByteOffset = 0;
    transferOp.destinationByteOffset = 0;
    transferOp.gpuDestinationBuffer = buffer;

    HgiBlitCmdsUniquePtr blitCmds2 = hgiVulkan.CreateBlitCmds();
    blitCmds2->CopyBufferCpuToGpu(transferOp);
    hgiVulkan.SubmitCmds(blitCmds2.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    std::vector<uint32_t> transferBackBlob(blob.size(), 0);

    // Read back the transfer to confirm it worked
    HgiBufferGpuToCpuOp copyOp2;
    copyOp2.byteSize = desc.byteSize;
    copyOp2.cpuDestinationBuffer = &transferBackBlob[0];
    copyOp2.destinationByteOffset = 0;
    copyOp2.gpuSourceBuffer = buffer;
    copyOp2.sourceByteOffset = 0;

    HgiBlitCmdsUniquePtr blitCmds3 = hgiVulkan.CreateBlitCmds();
    blitCmds3->CopyBufferGpuToCpu(copyOp2);
    hgiVulkan.SubmitCmds(blitCmds3.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    if (transferBackBlob != stagingBlob) {
        TF_CODING_ERROR("Transfer readback failed");
        return false;
    }

    // Put buffer in garbage collector
    device->WaitForIdle();
    hgiVulkan.DestroyBuffer(&buffer);

    // End frame garbage collection of buffer.
    // This should cleanup the buffer itself AND any internal staging buffers.
    hgiVulkan.EndFrame();

    return true;
}

static bool
TestVulkanTexture(HgiVulkan& hgiVulkan)
{
    // The goal of this test is to validate vulkan texture creation and verify
    // data is uploaded correctly.

    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    hgiVulkan.StartFrame();

    // Create the texture
    HgiTextureDesc desc;
    desc.debugName = "Debug Texture";
    desc.dimensions = GfVec3i(32, 32, 1);
    desc.format = HgiFormatFloat32Vec4;
    desc.type = HgiTextureType2D;
    desc.usage = HgiTextureUsageBitsColorTarget | HgiTextureUsageBitsShaderRead;

    size_t numTexels = 
        desc.dimensions[0] * desc.dimensions[1] * desc.dimensions[2];
    desc.pixelsByteSize = HgiGetDataSizeOfFormat(desc.format) * numTexels;

    std::vector<float> pixels(numTexels, 0.123f);
    desc.initialData = pixels.data();

    HgiTextureHandle texture = hgiVulkan.CreateTexture(desc);
    if (!texture) {
        TF_CODING_ERROR("Invalid texture");
        return false;
    }

    // Create a second texture VIEW of the first texture's data
    HgiTextureViewDesc viewDesc;
    viewDesc.debugName = "Debug TextureView";
    viewDesc.format = desc.format;
    viewDesc.sourceTexture = texture;

    HgiTextureViewHandle textureView = hgiVulkan.CreateTextureView(viewDesc);
    if (!textureView) {
        TF_CODING_ERROR("Invalid texture view");
        return false;
    }

    // Read back the initial pixels by using the TextureView
    std::vector<float> readBack(numTexels, 0);
    HgiTextureGpuToCpuOp readBackOp;
    readBackOp.cpuDestinationBuffer = &readBack[0];
    readBackOp.destinationBufferByteSize= readBack.size() * sizeof(readBack[0]);
    readBackOp.destinationByteOffset = 0;
    readBackOp.gpuSourceTexture = textureView->GetViewTexture();
    readBackOp.mipLevel = 0;
    readBackOp.sourceTexelOffset = GfVec3i(0);

    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan.CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(readBackOp);
    hgiVulkan.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    if (pixels != readBack) {
        TF_CODING_ERROR("initialData readback failed");
        return false;
    }

    // Upload some new pixels to the texture using the TextureView followed
    // by reading back the results.
    std::vector<float> upload(numTexels, 0.456f);
    HgiTextureCpuToGpuOp uploadOp;
    uploadOp.bufferByteSize = upload.size() * sizeof(upload[0]);
    uploadOp.cpuSourceBuffer = upload.data();
    uploadOp.destinationTexelOffset = GfVec3i(0);
    uploadOp.gpuDestinationTexture = textureView->GetViewTexture();
    uploadOp.mipLevel = 0;

    HgiBlitCmdsUniquePtr blitCmds2 = hgiVulkan.CreateBlitCmds();
    blitCmds2->CopyTextureCpuToGpu(uploadOp);

    HgiTextureGpuToCpuOp readBackOp2;
    readBackOp2.cpuDestinationBuffer = &readBack[0];
    readBackOp2.destinationBufferByteSize=readBack.size() * sizeof(readBack[0]);
    readBackOp2.destinationByteOffset = 0;
    readBackOp2.gpuSourceTexture = textureView->GetViewTexture();
    readBackOp2.mipLevel = 0;
    readBackOp2.sourceTexelOffset = GfVec3i(0);

    blitCmds2->CopyTextureGpuToCpu(readBackOp2);

    hgiVulkan.SubmitCmds(blitCmds2.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    if (upload != readBack) {
        TF_CODING_ERROR("upload readback failed");
        return false;
    }

    // Generate mips
    HgiBlitCmdsUniquePtr blitCmds3 = hgiVulkan.CreateBlitCmds();
    blitCmds3->GenerateMipMaps(texture);
    hgiVulkan.SubmitCmds(blitCmds3.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    // Cleanup
    hgiVulkan.DestroyTextureView(&textureView);
    hgiVulkan.DestroyTexture(&texture);

    // End frame garbage collection of texture.
    // This should cleanup the texture itself AND any internal staging buffers.
    hgiVulkan.EndFrame();

    return true;
}

static bool
TestVulkanPipeline(HgiVulkan& hgiVulkan)
{
    hgiVulkan.StartFrame();

    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    // Create a vertex program
    HgiShaderFunctionDesc shaderDesc;
    shaderDesc.shaderStage = HgiShaderStageVertex;
    shaderDesc.shaderCode = 
        "layout(location = 0) in vec3 positionIn; \n"
        ""
        "void main() { \n"
        "    gl_PointSize = 1.0; \n"
        "    gl_Position = vec4(positionIn, 1.0); \n"
        "} \n";

    shaderDesc.debugName = "debugShader";
    HgiShaderFunctionHandle vs = hgiVulkan.CreateShaderFunction(shaderDesc);
    if (!vs || !vs->IsValid()) {
        std::string const& error = vs->GetCompileErrors();
        TF_CODING_ERROR("TestVulkanPipeline failed %s", error.c_str());
        return false;
    }

    HgiShaderProgramDesc prgDesc;
    prgDesc.debugName = "debugProgram";
    prgDesc.shaderFunctions.push_back(vs);
    HgiShaderProgramHandle prg = hgiVulkan.CreateShaderProgram(prgDesc);
    TF_VERIFY(prg->IsValid());

    // Describe VBO of the vertex shader that has a 'positionIn' attribute
    HgiVertexAttributeDesc attrDesc;
    attrDesc.format = HgiFormatFloat32Vec3;
    attrDesc.offset = 0;
    attrDesc.shaderBindLocation = 0;

    HgiVertexBufferDesc vboDesc;
    vboDesc.bindingIndex = 0;
    vboDesc.vertexAttributes.push_back(attrDesc);
    vboDesc.vertexStride = HgiGetDataSizeOfFormat(attrDesc.format);

    // Try creating a pipeline without rasterizer.
    HgiGraphicsPipelineDesc psoDesc;
    psoDesc.debugName = "debugPipeline";
    psoDesc.depthState.depthTestEnabled = false;
    psoDesc.depthState.depthWriteEnabled = false;
    psoDesc.depthState.stencilTestEnabled = false;
    psoDesc.primitiveType = HgiPrimitiveTypePointList;
    psoDesc.rasterizationState.rasterizerEnabled = false;
    psoDesc.shaderConstantsDesc.byteSize = 64;
    psoDesc.shaderConstantsDesc.stageUsage = HgiShaderStageVertex;
    psoDesc.shaderProgram = prg;
    psoDesc.vertexBuffers.push_back(vboDesc);

    HgiGraphicsPipelineHandle pso = hgiVulkan.CreateGraphicsPipeline(psoDesc);
    if (!pso) {
        TF_CODING_ERROR("TestVulkanPipeline pipeline failed");
        return false;
    }

    // Try to bind the pipeline to a graphics cmds
    // No attachments since this is a vertex only shader.
    HgiGraphicsCmdsDesc gfxDesc;
    HgiGraphicsCmdsUniquePtr gfxCmds = hgiVulkan.CreateGraphicsCmds(gfxDesc);
    TF_VERIFY(gfxCmds);
    gfxCmds->PushDebugGroup("TestVulkanPipeline");
    gfxCmds->BindPipeline(pso);
    gfxCmds->PopDebugGroup();

    hgiVulkan.SubmitCmds(gfxCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    hgiVulkan.DestroyGraphicsPipeline(&pso);
    hgiVulkan.DestroyShaderProgram(&prg);
    hgiVulkan.DestroyShaderFunction(&vs);

    hgiVulkan.EndFrame();
    return true;
}

static bool
TestVulkanGraphicsCmds(HgiVulkan& hgiVulkan)
{
    hgiVulkan.StartFrame();

    HgiVulkanGarbageCollector* gc = hgiVulkan.GetGarbageCollector();
    HgiVulkanBufferVector* gcBuffer = gc->GetBufferList();
    if (!gcBuffer->empty()) {
        TF_CODING_ERROR("We expected the garbage collector to be empty");
        return false;
    }

    const uint32_t size = 64;

    HgiTextureHandle textures[2];
    HgiTextureHandle resolves[2];
    HgiAttachmentDesc attachments[2];

    // Describe / create texture attachments
    for (size_t i=0; i<2; i++) {
        HgiTextureDesc texDesc;
        texDesc.dimensions = GfVec3i(size, size, 1);
        texDesc.format = i==0 ? 
            HgiFormatFloat16Vec4 :
            HgiFormatFloat32UInt8;
        texDesc.sampleCount = HgiSampleCount4;
        texDesc.type = HgiTextureType2D;
        texDesc.usage = i==0 ? 
            HgiTextureUsageBitsColorTarget :
            HgiTextureUsageBitsDepthTarget | HgiTextureUsageBitsStencilTarget;
        textures[i] = hgiVulkan.CreateTexture(texDesc);

        attachments[i].usage = texDesc.usage;
        attachments[i].format = texDesc.format;
        attachments[i].loadOp = HgiAttachmentLoadOpClear;
        attachments[i].storeOp = HgiAttachmentStoreOpDontCare;

        HgiTextureDesc resolveDesc;
        resolveDesc.dimensions = texDesc.dimensions;
        resolveDesc.format = texDesc.format;
        resolveDesc.sampleCount = HgiSampleCount1;
        resolveDesc.type = texDesc.type;
        resolveDesc.usage = texDesc.usage;
        resolves[i] = hgiVulkan.CreateTexture(resolveDesc);
    }

    // Create fullscreen triangle buffers
    const std::vector<float> position = {
        -1.0f, -1.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f};

    const std::vector<uint32_t> indices = {0,1,2};

    // Create the index buffer
    HgiBufferDesc indicesDesc;
    indicesDesc.debugName = "Indices Fullscreen";
    indicesDesc.byteSize = indices.size() * sizeof(uint32_t);
    indicesDesc.initialData = indices.data();
    indicesDesc.usage = HgiBufferUsageIndex32;
    HgiBufferHandle ibo = hgiVulkan.CreateBuffer(indicesDesc);

    // Create the position vertex buffer
    HgiBufferDesc verticesDesc;
    verticesDesc.debugName = "Position Fullscreen";
    verticesDesc.byteSize = position.size() * sizeof(float);
    verticesDesc.initialData = position.data();
    verticesDesc.usage = HgiBufferUsageVertex;
    HgiBufferHandle vbo = hgiVulkan.CreateBuffer(verticesDesc);

    // Describe VBO of the vertex shader that has a 'positionIn' attribute
    HgiVertexAttributeDesc attrDesc;
    attrDesc.format = HgiFormatFloat32Vec3;
    attrDesc.offset = 0;
    attrDesc.shaderBindLocation = 0;

    HgiVertexBufferDesc vboDesc;
    vboDesc.bindingIndex = 0;
    vboDesc.vertexAttributes.push_back(attrDesc);
    vboDesc.vertexStride = HgiGetDataSizeOfFormat(attrDesc.format);

    // Create a vertex program
    HgiShaderFunctionDesc vsDesc;
    vsDesc.shaderStage = HgiShaderStageVertex;
    vsDesc.shaderCode = 
        "layout(location = 0) in vec3 positionIn; \n"
        ""
        "void main() { \n"
        "    gl_Position = vec4(positionIn, 1.0); \n"
        "} \n";

    vsDesc.debugName = "debug vs shader";
    HgiShaderFunctionHandle vs = hgiVulkan.CreateShaderFunction(vsDesc);
    if (!vs || !vs->IsValid()) {
        std::string const& error = vs->GetCompileErrors();
        TF_CODING_ERROR("TestVulkanGraphicsCmds failed %s", error.c_str());
        return false;
    }

    // Create a fragment program
    HgiShaderFunctionDesc fsDesc;
    fsDesc.shaderStage = HgiShaderStageFragment;
    fsDesc.shaderCode = 
        "layout(location = 0) out vec4 outputColor; \n"
        ""
        "void main() { \n"
        "    outputColor = vec4(1,0,1,1); \n"
        "} \n";

    fsDesc.debugName = "debug fs shader";
    HgiShaderFunctionHandle fs = hgiVulkan.CreateShaderFunction(fsDesc);
    if (!fs || !fs->IsValid()) {
        std::string const& error = fs->GetCompileErrors();
        TF_CODING_ERROR("TestVulkanGraphicsCmds failed %s", error.c_str());
        return false;
    }

    // shader program
    HgiShaderProgramDesc prgDesc;
    prgDesc.debugName = "debug shader program";
    prgDesc.shaderFunctions.push_back(vs);
    prgDesc.shaderFunctions.push_back(fs);
    HgiShaderProgramHandle prg = hgiVulkan.CreateShaderProgram(prgDesc);
    TF_VERIFY(prg->IsValid());

    // Try creating a pipeline without rasterizer.
    HgiGraphicsPipelineDesc psoDesc;
    psoDesc.debugName = "debugPipeline";
    psoDesc.depthState.depthTestEnabled = false;
    psoDesc.depthState.depthWriteEnabled = false;
    psoDesc.depthState.stencilTestEnabled = false;
    psoDesc.multiSampleState.sampleCount = HgiSampleCount4;
    psoDesc.primitiveType = HgiPrimitiveTypeTriangleList;
    psoDesc.shaderProgram = prg;
    psoDesc.vertexBuffers.push_back(vboDesc);
    psoDesc.colorAttachmentDescs.push_back(attachments[0]);
    psoDesc.depthAttachmentDesc = attachments[1];
    psoDesc.resolveAttachments = true;

    HgiGraphicsPipelineHandle pso = hgiVulkan.CreateGraphicsPipeline(psoDesc);
    if (!pso) {
        TF_CODING_ERROR("TestVulkanGraphicsCmds pipeline failed");
        return false;
    }

    // Render
    HgiGraphicsCmdsDesc gfxDesc;
    gfxDesc.colorAttachmentDescs.push_back(attachments[0]);
    gfxDesc.colorTextures.push_back(textures[0]);
    gfxDesc.colorResolveTextures.push_back(resolves[0]);
    gfxDesc.depthAttachmentDesc = attachments[1];
    gfxDesc.depthTexture = textures[1];
    gfxDesc.depthResolveTexture = resolves[1];

    HgiGraphicsCmdsUniquePtr gfxCmds = hgiVulkan.CreateGraphicsCmds(gfxDesc);
    gfxCmds->PushDebugGroup("TestVulkanGraphicsCmds");
    gfxCmds->BindPipeline(pso);
    gfxCmds->BindVertexBuffers({{vbo, 0, 0}});
    GfVec4i vp = GfVec4i(0, 0, size, size);
    gfxCmds->SetViewport(vp);
    gfxCmds->DrawIndexed(ibo, 3, 0, 0, 1, 0);
    gfxCmds->PopDebugGroup();

    hgiVulkan.SubmitCmds(gfxCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    // Cleanup
    hgiVulkan.DestroyTexture(&textures[0]);
    hgiVulkan.DestroyTexture(&textures[1]);
    hgiVulkan.DestroyTexture(&resolves[0]);
    hgiVulkan.DestroyTexture(&resolves[1]);
    hgiVulkan.DestroyGraphicsPipeline(&pso);
    hgiVulkan.DestroyShaderProgram(&prg);
    hgiVulkan.DestroyShaderFunction(&fs);
    hgiVulkan.DestroyShaderFunction(&vs);
    hgiVulkan.DestroyBuffer(&vbo);
    hgiVulkan.DestroyBuffer(&ibo);

    hgiVulkan.EndFrame();

    HgiVulkanCommandQueue* queue = 
        hgiVulkan.GetPrimaryDevice()->GetCommandQueue();
    uint64_t inflightBits = queue->GetInflightCommandBuffersBits();
    if (inflightBits != 0) {
        TF_CODING_ERROR("Not all command buffers fully consumed");
        return false;
    }

    if (!gcBuffer->empty()) {
        TF_CODING_ERROR("We expected the garbage collector to be empty");
        return false;
    }

    return true;
}

static bool
TestVulkanComputeCmds(HgiVulkan& hgiVulkan)
{
    hgiVulkan.StartFrame();

    // Create a compute program
    HgiShaderFunctionDesc csDesc;
    csDesc.shaderStage = HgiShaderStageCompute;
    csDesc.shaderCode = 
        "#extension GL_EXT_nonuniform_qualifier : require \n"
        "#extension GL_EXT_scalar_block_layout : require \n"
        ""
        "layout(push_constant) uniform PushConstantBuffer { \n"
        "    layout(offset = 0) int index; \n"
        "} pushConstants; \n"
        ""
        "layout (scalar, set=0, binding=0) uniform ParamsIn { \n"
        "    float offset; \n"
        "} paramsIn; \n"
        ""
        "layout (scalar, set=0, binding=1) buffer StorageBufferIn { \n"
        "    vec4 value[]; \n"
        "} storageBufferIn; \n"
        ""
        "layout (scalar, set=0, binding=2) buffer StorageBufferOut { \n"
        "    vec4 value[]; \n"
        "} storageBufferOut; \n"
        ""
        "layout (rgba32f, set=0, binding=3) uniform image2D ImageIn; \n"
        ""
        "void main() { \n"
        "    vec4 v = storageBufferIn.value[pushConstants.index]; \n"
        "    v *= paramsIn.offset; \n"
        "    storageBufferOut.value[pushConstants.index] = v; \n"
        "} \n";

    csDesc.debugName = "debug cs shader";
    HgiShaderFunctionHandle cs = hgiVulkan.CreateShaderFunction(csDesc);
    if (!cs || !cs->IsValid()) {
        std::string const& error = cs->GetCompileErrors();
        TF_CODING_ERROR("TestVulkanComputeCmds failed %s", error.c_str());
        return false;
    }

    HgiShaderProgramDesc prgDesc;
    prgDesc.shaderFunctions.push_back(cs);
    HgiShaderProgramHandle prg = hgiVulkan.CreateShaderProgram(prgDesc);

    // Create the pipeline
    HgiComputePipelineDesc psoDesc;
    psoDesc.shaderConstantsDesc.byteSize = 16;
    psoDesc.shaderProgram = prg;
    HgiComputePipelineHandle pso = hgiVulkan.CreateComputePipeline(psoDesc);

    std::vector<uint8_t> pushConstants(psoDesc.shaderConstantsDesc.byteSize, 0);

    // Create resource buffers
    std::vector<uint8_t> blob(64, 0);

    HgiBufferDesc uboDesc;
    uboDesc.debugName = "Ubo";
    uboDesc.byteSize = 64;
    uboDesc.initialData = blob.data();
    uboDesc.usage = HgiBufferUsageUniform;
    HgiBufferHandle ubo = hgiVulkan.CreateBuffer(uboDesc);

    HgiBufferBindDesc uboBindDesc;
    uboBindDesc.bindingIndex = 0;
    uboBindDesc.buffers.push_back(ubo);
    uboBindDesc.offsets.push_back(0);
    uboBindDesc.resourceType = HgiBindResourceTypeUniformBuffer;
    uboBindDesc.stageUsage = HgiShaderStageCompute;

    HgiBufferDesc ssbo0Desc;
    ssbo0Desc.debugName = "Sbbo 0";
    ssbo0Desc.byteSize = 64;
    ssbo0Desc.initialData = blob.data();
    ssbo0Desc.usage = HgiBufferUsageStorage;
    HgiBufferHandle ssbo0 = hgiVulkan.CreateBuffer(ssbo0Desc);

    HgiBufferBindDesc ssbo0BindDesc;
    ssbo0BindDesc.bindingIndex = 1;
    ssbo0BindDesc.buffers.push_back(ssbo0);
    ssbo0BindDesc.offsets.push_back(0);
    ssbo0BindDesc.resourceType = HgiBindResourceTypeStorageBuffer;
    ssbo0BindDesc.stageUsage = HgiShaderStageCompute;

    HgiBufferDesc ssbo1Desc;
    ssbo1Desc.debugName = "Sbbo 1";
    ssbo1Desc.byteSize = 64;
    ssbo1Desc.initialData = blob.data();
    ssbo1Desc.usage = HgiBufferUsageStorage;
    HgiBufferHandle ssbo1 = hgiVulkan.CreateBuffer(ssbo1Desc);

    HgiBufferBindDesc ssbo1BindDesc;
    ssbo1BindDesc.bindingIndex = 2;
    ssbo1BindDesc.buffers.push_back(ssbo1);
    ssbo1BindDesc.offsets.push_back(0);
    ssbo1BindDesc.resourceType = HgiBindResourceTypeStorageBuffer;
    ssbo1BindDesc.stageUsage = HgiShaderStageCompute;

    // Create resource image
    HgiTextureDesc imageDesc;
    imageDesc.dimensions = GfVec3i(64,64,1);
    imageDesc.format = HgiFormatFloat32Vec4;
    size_t imageBytes = HgiGetDataSize(imageDesc.format, imageDesc.dimensions);
    std::vector<uint8_t> imageBlob(imageBytes, 0);
    imageDesc.initialData = imageBlob.data();
    imageDesc.pixelsByteSize = imageBlob.size();
    imageDesc.usage = 
        HgiTextureUsageBitsShaderRead | 
        HgiTextureUsageBitsShaderWrite;
    HgiTextureHandle image = hgiVulkan.CreateTexture(imageDesc);

    HgiTextureBindDesc imageBindDesc;
    imageBindDesc.bindingIndex = 0;
    imageBindDesc.resourceType = HgiBindResourceTypeStorageImage;
    imageBindDesc.samplers.push_back(HgiSamplerHandle()); // no sampler for img
    imageBindDesc.stageUsage = HgiShaderStageCompute;
    imageBindDesc.textures.push_back(image);

    // Make resource bindings
    HgiResourceBindingsDesc rbDesc;
    rbDesc.buffers.push_back(uboBindDesc);
    rbDesc.buffers.push_back(ssbo0BindDesc);
    rbDesc.buffers.push_back(ssbo1BindDesc);
    rbDesc.textures.push_back(imageBindDesc);

    HgiResourceBindingsHandle resourceBindings =
        hgiVulkan.CreateResourceBindings(rbDesc);

    // Dispatch compute work
    HgiComputeCmdsDesc compDesc;
    HgiComputeCmdsUniquePtr computeCmds = hgiVulkan.CreateComputeCmds(compDesc);
    computeCmds->PushDebugGroup("TestVulkanComputeCmds");
    computeCmds->BindPipeline(pso);
    computeCmds->BindResources(resourceBindings);
    computeCmds->SetConstantValues(
        pso, 0, (uint32_t) pushConstants.size(), pushConstants.data());
    computeCmds->Dispatch(64, 64);
    computeCmds->PopDebugGroup();
    hgiVulkan.SubmitCmds(computeCmds.get(),HgiSubmitWaitTypeWaitUntilCompleted);

    // Cleanup
    hgiVulkan.DestroyResourceBindings(&resourceBindings);
    hgiVulkan.DestroyTexture(&image);
    hgiVulkan.DestroyBuffer(&ubo);
    hgiVulkan.DestroyBuffer(&ssbo0);
    hgiVulkan.DestroyBuffer(&ssbo1);
    hgiVulkan.DestroyComputePipeline(&pso);
    hgiVulkan.DestroyShaderProgram(&prg);
    hgiVulkan.DestroyShaderFunction(&cs);

    hgiVulkan.EndFrame();

    HgiVulkanCommandQueue* queue = 
        hgiVulkan.GetPrimaryDevice()->GetCommandQueue();
    uint64_t inflightBits = queue->GetInflightCommandBuffersBits();
    if (inflightBits != 0) {
        TF_CODING_ERROR("Not all command buffers fully consumed");
        return false;
    }

    return true;
}

static void
_SaveToPNG(
    int width,
    int height,
    const char* pixels,
    const std::string& filePath)
{
    HioImage::StorageSpec storage;
    storage.width = width;
    storage.height = height;
    storage.format = _imgHioFormat;
    storage.flipped = false;
    storage.data = (void*)pixels;

    HioImageSharedPtr image = HioImage::OpenForWriting(filePath);
    TF_VERIFY(image && image->Write(storage));
}

static void
_SaveGpuTextureToFile(
    HgiVulkan& hgiVulkan,
    HgiTextureHandle const& texHandle,
    int width,
    int height,
    HgiFormat format,
    std::string const& filePath)
{
    // Copy the pixels from gpu into a cpu buffer so we can save it to disk.
    const size_t bufferByteSize =
        width * height * HgiGetDataSizeOfFormat(format);
    std::vector<char> buffer(bufferByteSize);

    HgiTextureGpuToCpuOp copyOp;
    copyOp.gpuSourceTexture = texHandle;
    copyOp.sourceTexelOffset = GfVec3i(0);
    copyOp.mipLevel = 0;
    copyOp.cpuDestinationBuffer = buffer.data();
    copyOp.destinationByteOffset = 0;
    copyOp.destinationBufferByteSize = bufferByteSize;

    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan.CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(copyOp);
    hgiVulkan.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    _SaveToPNG(width, height, buffer.data(), filePath);
}

static void
_SaveGpuBufferToFile(
    HgiVulkan& hgiVulkan,
    HgiBufferHandle const& bufHandle,
    int width,
    int height,
    HgiFormat format,
    std::string const& filePath)
{
    // Copy the pixels from gpu into a cpu buffer so we can save it to disk.
    const size_t bufferByteSize =
        width * height * HgiGetDataSizeOfFormat(format);
    std::vector<char> buffer(bufferByteSize);

    HgiBufferGpuToCpuOp copyOp;
    copyOp.gpuSourceBuffer = bufHandle;
    copyOp.sourceByteOffset = 0;
    copyOp.byteSize = bufferByteSize;
    copyOp.cpuDestinationBuffer = buffer.data();
    copyOp.destinationByteOffset = 0;    

    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan.CreateBlitCmds();
    blitCmds->CopyBufferGpuToCpu(copyOp);
    hgiVulkan.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    _SaveToPNG(width, height, buffer.data(), filePath);
}

static HgiTextureHandle
_CreateTexture(
    HgiVulkan& hgiVulkan,
    int width, 
    int height,
    HgiFormat format,
    void *data)
{
    const size_t textureByteSize =
        width * height * HgiGetDataSizeOfFormat(format);

    HgiTextureDesc texDesc;
    texDesc.debugName = "Debug texture";
    texDesc.dimensions = GfVec3i(width, height, 1);
    texDesc.format = format;
    texDesc.initialData = data;
    texDesc.layerCount = 1;
    texDesc.mipLevels = 1;
    texDesc.pixelsByteSize = textureByteSize;
    texDesc.sampleCount = HgiSampleCount1;
    texDesc.usage = HgiTextureUsageBitsShaderRead;

    return hgiVulkan.CreateTexture(texDesc);
}

static HgiBufferHandle
_CreateBuffer(
    HgiVulkan& hgiVulkan,
    size_t byteSize,
    void *data)
{
    HgiBufferDesc bufDesc;
    bufDesc.usage = HgiBufferUsageUniform;
    bufDesc.byteSize = byteSize;
    bufDesc.initialData = data;   

    return hgiVulkan.CreateBuffer(bufDesc);
}

static HgiGraphicsCmdsDesc
_CreateGraphicsCmdsColor0Color1Depth(
    HgiVulkan& hgiVulkan,
    GfVec3i const& size,
    HgiFormat colorFormat)
{
    // Create two color attachments
    HgiTextureDesc texDesc;
    texDesc.dimensions = size;
    texDesc.type = HgiTextureType2D;
    texDesc.format = colorFormat;
    texDesc.sampleCount = HgiSampleCount1;
    texDesc.usage = HgiTextureUsageBitsColorTarget;
    HgiTextureHandle colorTex0 = hgiVulkan.CreateTexture(texDesc);
    HgiTextureHandle colorTex1 = hgiVulkan.CreateTexture(texDesc);

    // Create a depth attachment
    texDesc.usage = HgiTextureUsageBitsDepthTarget;
    texDesc.format = HgiFormatFloat32;
    HgiTextureHandle depthTex = hgiVulkan.CreateTexture(texDesc);

    // Setup color and depth attachments
    HgiAttachmentDesc colorAttachment0;
    colorAttachment0.loadOp = HgiAttachmentLoadOpClear;
    colorAttachment0.storeOp = HgiAttachmentStoreOpStore;
    colorAttachment0.format = colorFormat;
    colorAttachment0.usage = HgiTextureUsageBitsColorTarget;

    HgiAttachmentDesc colorAttachment1;
    colorAttachment1.loadOp = HgiAttachmentLoadOpClear;
    colorAttachment1.storeOp = HgiAttachmentStoreOpStore;
    colorAttachment1.format = colorFormat;
    colorAttachment1.usage = HgiTextureUsageBitsColorTarget;

    HgiAttachmentDesc depthAttachment;
    depthAttachment.format = HgiFormatFloat32;
    depthAttachment.usage = HgiTextureUsageBitsDepthTarget;

    // Configure graphics cmds
    HgiGraphicsCmdsDesc desc;
    desc.colorAttachmentDescs.push_back(colorAttachment0);
    desc.colorAttachmentDescs.push_back(colorAttachment1);
    desc.depthAttachmentDesc = depthAttachment;
    desc.colorTextures.push_back(colorTex0);
    desc.colorTextures.push_back(colorTex1);
    desc.depthTexture = depthTex;

    return desc;
}

static bool
TestGraphicsCmdsClear(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }
    
    const size_t width = _imgSize;
    const size_t height = _imgSize;
    const HgiFormat format = _imgFormat;
    
    // Create a default cmds description and set the clearValue for the
    // first attachment to something other than black.
    // Setting 'loadOp' tp 'Clear' is important for this test since we expect
    // the attachment to be cleared when the graphics cmds is submitted
    HgiGraphicsCmdsDesc desc = _CreateGraphicsCmdsColor0Color1Depth(
        hgiVulkan, GfVec3i(width, height, 1), format);
    desc.colorAttachmentDescs[0].loadOp = HgiAttachmentLoadOpClear;
    desc.colorAttachmentDescs[0].storeOp = HgiAttachmentStoreOpStore;
    desc.colorAttachmentDescs[0].clearValue = GfVec4f(1, 0, 0.5, 1);

    // For Vulkan, we expect attachment0 to be cleared when the cmds is 
    // submitted.
    HgiGraphicsCmdsUniquePtr gfxCmds = hgiVulkan.CreateGraphicsCmds(desc);
    hgiVulkan.SubmitCmds(gfxCmds.get());

    // Save attachment0 to disk
    _SaveGpuTextureToFile(
        hgiVulkan, 
        desc.colorTextures[0], 
        width, 
        height, 
        format, 
        "graphicsCmdsClear.png");

    // Cleanup
    for (HgiTextureHandle& tex : desc.colorTextures) {
        hgiVulkan.DestroyTexture(&tex);
    }
    if (desc.depthTexture) {
        hgiVulkan.DestroyTexture(&desc.depthTexture);
    }

    return true;
}

static bool
TestCreateSrgbaTexture(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    const size_t width = 128;
    const size_t height = 128;
    const HgiFormat format = HgiFormatUNorm8Vec4srgb;

    const size_t dataByteSize = width * height * HgiGetDataSizeOfFormat(format);
    
    // Create the texture
    std::vector<uint8_t> textureData(dataByteSize, 64);
    HgiTextureHandle tex = _CreateTexture(
        hgiVulkan, width, height, format, textureData.data());

    // Write texture to file
    static std::string const filePath = "srgba.png";
    _SaveGpuTextureToFile(hgiVulkan, tex, width, height, format, filePath);

    hgiVulkan.DestroyTexture(&tex);

    return true;
}

static bool
TestHgiGetMipInitialData(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    // Test helper function that is used during mipmap data upload.
    // (This does not actually upload any data)
    const HgiFormat format = _imgFormat;
    const GfVec3i size0(37,53,1);
    const size_t layerCount = 1;

    const size_t texelByteSize = HgiGetDataSizeOfFormat(format);
    const size_t firstMipSize = size0[0] * size0[1] * size0[2] * texelByteSize;

    // We expect the second mip to be 1/4 of the bytesize of the first etc.
    GfVec3i size1 = size0 / 2;
    size1[2] = 1;

    const size_t secondMipSize = size1[0] * size1[1] * size1[2] * texelByteSize;
    
    GfVec3i size2 = size1 / 2;
    size2[2] = 1;

    const size_t thirdMipSize = size2[0] * size2[1] * size2[2] * texelByteSize;

    // Create some fake mipmap data for all three mips
    const size_t totalSize = firstMipSize + secondMipSize + thirdMipSize;

    const std::vector<HgiMipInfo> mipInfos = HgiGetMipInfos(
        format, size0, layerCount, totalSize);

    if (mipInfos.size() != 3) {
        TF_CODING_ERROR("TestHgiGetMipInfos returned wrong number of infos");
        return false;
    }

    // We expect the returned ptr to be at the start of the third mip's data
    // in the 'mipData' vector. And the returned dimensions and bytesize to
    // match the third mip.
    const size_t startOfThirdMip = firstMipSize + secondMipSize;

    if (mipInfos[2].dimensions != size2 ||
        mipInfos[2].byteSizePerLayer != thirdMipSize ||
        mipInfos[2].byteOffset != startOfThirdMip) {
        TF_CODING_ERROR("TestHgiGetMipInitialData incorrect return values");
        return false;
    }

    return true;
}

static bool
TestHgiTextureToBufferCopy(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    const int width = 128;
    const int height = 128;
    const HgiFormat format = HgiFormatUNorm8Vec4srgb;
    
    const size_t dataByteSize = width * height * HgiGetDataSizeOfFormat(format);

    // Create the texture
    std::vector<uint8_t> textureData(dataByteSize, 16);
    HgiTextureHandle tex = _CreateTexture(
        hgiVulkan, width, height, format, textureData.data());

    // Create the buffer
    HgiBufferHandle buf = _CreateBuffer(hgiVulkan, dataByteSize, nullptr);
    
    // Copy texture to buffer
    HgiTextureToBufferOp copyOp;
    copyOp.gpuSourceTexture = tex;
    copyOp.gpuDestinationBuffer = buf;
    copyOp.byteSize = dataByteSize;
    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan.CreateBlitCmds();
    blitCmds->CopyTextureToBuffer(copyOp);
    hgiVulkan.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    static std::string const filePath = "copyTextureToBuffer.png";
    _SaveGpuBufferToFile(hgiVulkan, buf, width, height, format, filePath);
    
    hgiVulkan.DestroyBuffer(&buf);
    hgiVulkan.DestroyTexture(&tex);

    return true;
}

static bool
TestHgiBufferToTextureCopy(HgiVulkan& hgiVulkan)
{
    HgiVulkanDevice* device = hgiVulkan.GetPrimaryDevice();
    if (!device) {
        return false;
    }

    const int width = 128;
    const int height = 128;
    const HgiFormat format = HgiFormatUNorm8Vec4srgb;
    
    const size_t dataByteSize = width * height * HgiGetDataSizeOfFormat(format);
    
    // Create the buffer
    std::vector<uint8_t> bufferData(dataByteSize, 32);
    HgiBufferHandle buf = _CreateBuffer(
        hgiVulkan, dataByteSize, bufferData.data());

    // Create the texture
    HgiTextureHandle tex = _CreateTexture(
        hgiVulkan, width, height, format, nullptr);
    
    // Copy buffer to texture
    HgiBufferToTextureOp copyOp;
    copyOp.gpuSourceBuffer = buf;
    copyOp.gpuDestinationTexture = tex;
    copyOp.byteSize = dataByteSize;
    HgiBlitCmdsUniquePtr blitCmds = hgiVulkan.CreateBlitCmds();
    blitCmds->CopyBufferToTexture(copyOp);
    hgiVulkan.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    static std::string const filePath = "copyBufferToTexture.png";
    _SaveGpuTextureToFile(hgiVulkan, tex, width, height, format, filePath);
    
    hgiVulkan.DestroyTexture(&tex);
    hgiVulkan.DestroyBuffer(&buf);

    return true;
}

static bool
TestHgiVulkan()
{
    // Run tests
    HgiVulkan hgiVulkan;

    bool ret = true;
    std::cout <<  "*** Running test: HgiVulkan" << std::endl << std::endl;

    // Test vulkan instance creation
    ret &= TestVulkanInstance(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanInstance failed");
        return false;
    }

    // Test vulkan device creation
    ret &= TestVulkanDevice(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanDevice failed");
        return false;
    }

    // Test vulkan shader compiler
    ret &= TestVulkanShaderCompiler(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanShaderCompiler failed");
        return false;
    }

    // Test vulkan command queue
    ret &= TestVulkanCommandQueue(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanCommandQueue failed");
        return false;
    }

    // Test vulkan garbage collection
    ret &= TestVulkanGarbageCollection(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanGarbageCollection failed");
        return false;
    }

    // Test vulkan buffer
    ret &= TestVulkanBuffer(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanBuffer failed");
        return false;
    }

    // Test vulkan texture
    ret &= TestVulkanTexture(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanTexture failed");
        return false;
    }

    // Test vulkan pipeline
    ret &= TestVulkanPipeline(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanPipeline failed");
        return false;
    }

    // Test vulkan graphicsCmds
    ret &= TestVulkanGraphicsCmds(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanGraphicsCmds failed");
        return false;
    }

    // Test vulkan computeCmds
    ret &= TestVulkanComputeCmds(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestVulkanComputeCmds failed");
        return false;
    }

    // Test clearing attachment0 in graphics cmds
    ret &= TestGraphicsCmdsClear(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestGraphicsCmdsClear failed");
        return false;
    }

    // Test saving a SRGBA texture
    ret &= TestCreateSrgbaTexture(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestCreateSrgbaTexture failed");
        return false;
    }

    // Test getting texel data for mips
    ret &= TestHgiGetMipInitialData(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestHgiGetMipInitialData failed");
        return false;
    }

    // Test copying a GPU texture to a GPU buffer via HgiTextureToBufferOp
    ret &= TestHgiTextureToBufferCopy(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestHgiTextureToBufferCopy failed");
        return false;
    }

    // Test copying a GPU buffer to a GPU texture via HgiBufferToTextureOp
    ret &= TestHgiBufferToTextureCopy(hgiVulkan);
    if (!ret) {
        TF_CODING_ERROR("TestHgiBufferToTextureCopy failed");
        return false;
    }

    return ret;
}

int
main(int argc, char **argv)
{
    TfErrorMark mark;
    bool passed = TestHgiVulkan();

    if (passed && mark.IsClean()) {
        std::cout << "HgiVulkan: Passed" << std::endl;
        exit(0);
    } else {
        for (auto it = mark.begin(); it != mark.end(); it++) {
            std::cout << it->GetCommentary() << std::endl;
        }
        std::cout << "HgiVulkan: Failed" << std::endl;
        exit(1);
    }
}
