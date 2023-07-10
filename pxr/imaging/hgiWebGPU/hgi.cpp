//
// Copyright 2022 Pixar
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
#include "pxr/base/arch/defines.h"

#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/buffer.h"
#include "pxr/imaging/hgiWebGPU/blitCmds.h"
#include "pxr/imaging/hgiWebGPU/computeCmds.h"
#include "pxr/imaging/hgiWebGPU/computePipeline.h"
#include "pxr/imaging/hgiWebGPU/capabilities.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/graphicsCmds.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/resourceBindings.h"
#include "pxr/imaging/hgiWebGPU/sampler.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/shaderProgram.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include <algorithm>

#if defined EMSCRIPTEN
#include <emscripten/html5_webgpu.h>
#else
#if defined _WIN32 && !defined WIN32_VULKAN
#define DAWN_ENABLE_BACKEND_D3D12
#elif defined(ARCH_OS_DARWIN)
#define DAWN_ENABLE_BACKEND_METAL
#else
#define DAWN_ENABLE_BACKEND_VULKAN
#endif

#include <dawn/dawn_proc.h>
#include <dawn/webgpu_cpp.h>
#include <dawn/native/NullBackend.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<HgiWebGPU, TfType::Bases<Hgi> >();
    t.SetFactory<HgiFactory<HgiWebGPU>>();
}

// GetDevice code based on https://github.com/kainino0x/webgpu-cross-platform-demo/blob/main/main.cpp
#if defined EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>

wgpu::Device GetDevice() {
    WGPUDevice deviceImp = emscripten_webgpu_get_device();
    return wgpu::Device::Acquire(deviceImp);
}
#else
#include <dawn/native/DawnNative.h>

    void PrintDeviceError(WGPUErrorType errorType, const char* message, void*) {
        std::string errorTypeName = "";
        switch (errorType) {
            case WGPUErrorType_Validation:
                errorTypeName = "Validation";
                break;
            case WGPUErrorType_OutOfMemory:
                errorTypeName = "Out of memory";
                break;
            case WGPUErrorType_Unknown:
                errorTypeName = "Unknown";
                break;
            case WGPUErrorType_DeviceLost:
                errorTypeName = "Device lost";
                break;
            default:
                return;
        }
        TF_CODING_ERROR(errorTypeName + " error: " + message);
    }
    static std::unique_ptr<dawn::native::Instance> instance;

// Return backend select priority, smaller number means higher priority.
    int GetBackendPriority(wgpu::BackendType t) {
        switch (t) {
            case wgpu::BackendType::Null:
                return 9999;
            case wgpu::BackendType::D3D12:
            case wgpu::BackendType::Metal:
            case wgpu::BackendType::Vulkan:
                return 0;
            case wgpu::BackendType::WebGPU:
                return 5;
            case wgpu::BackendType::D3D11:
            case wgpu::BackendType::OpenGL:
            case wgpu::BackendType::OpenGLES:
                return 10;
        }
        return 100;
    }

    const char* BackendTypeName(wgpu::BackendType t)
    {
        switch (t) {
            case wgpu::BackendType::Null:
                return "Null";
            case wgpu::BackendType::WebGPU:
                return "WebGPU";
            case wgpu::BackendType::D3D11:
                return "D3D11";
            case wgpu::BackendType::D3D12:
                return "D3D12";
            case wgpu::BackendType::Metal:
                return "Metal";
            case wgpu::BackendType::Vulkan:
                return "Vulkan";
            case wgpu::BackendType::OpenGL:
                return "OpenGL";
            case wgpu::BackendType::OpenGLES:
                return "OpenGL ES";
        }
        return "?";
    }

    const char* AdapterTypeName(wgpu::AdapterType t)
    {
        switch (t) {
            case wgpu::AdapterType::DiscreteGPU:
                return "Discrete GPU";
            case wgpu::AdapterType::IntegratedGPU:
                return "Integrated GPU";
            case wgpu::AdapterType::CPU:
                return "CPU";
            case wgpu::AdapterType::Unknown:
                return "Unknown";
        }
        return "?";
    }

    wgpu::Device GetDevice() {
        instance = std::make_unique<dawn::native::Instance>();
        instance->DiscoverDefaultAdapters();

        auto adapters = instance->GetAdapters();

        // Sort adapters by adapterType,
        std::sort(adapters.begin(), adapters.end(), [](const dawn::native::Adapter& a, const dawn::native::Adapter& b){
            wgpu::AdapterProperties pa, pb;
            a.GetProperties(&pa);
            b.GetProperties(&pb);

            if (pa.adapterType != pb.adapterType) {
                // Put GPU adapter (D3D, Vulkan, Metal) at front and CPU adapter at back.
                return pa.adapterType < pb.adapterType;
            }

            return GetBackendPriority(pa.backendType) < GetBackendPriority(pb.backendType);
        });
        // Simply pick the first adapter in the sorted list.
        dawn::native::Adapter backendAdapter = adapters[0];

        TF_DEBUG(HGIWEBGPU_DEBUG_DEVICE_CREATION).
          Msg("Available adapters sorted by their Adapter type, with GPU adapters listed at front and preferred:\n\n");
        TF_DEBUG(HGIWEBGPU_DEBUG_DEVICE_CREATION).Msg(" [Selected] -> ");
        for (auto&& a : adapters) {
            wgpu::AdapterProperties p;
            a.GetProperties(&p);
            TF_DEBUG(HGIWEBGPU_DEBUG_DEVICE_CREATION).Msg(
                    "* %s (%s)\n"
                    "    deviceID=%u, vendorID=0x%x, BackendType::%s, AdapterType::%s\n",
                    p.name, p.driverDescription, p.deviceID, p.vendorID,
                    BackendTypeName(p.backendType), AdapterTypeName(p.adapterType));
        }
        TF_DEBUG(HGIWEBGPU_DEBUG_DEVICE_CREATION).Msg("\n\n");

        // Toggle for debugging shader
        wgpu::DeviceDescriptor descriptor;
        wgpu::FeatureName requiredFeatures = wgpu::FeatureName::Depth32FloatStencil8;
        descriptor.requiredFeatures = &requiredFeatures;
        descriptor.requiredFeaturesCount = 1;

        WGPUDevice cDevice = backendAdapter.CreateDevice(&descriptor);
        wgpu::Device device = wgpu::Device::Acquire(cDevice);
        DawnProcTable procs = dawn::native::GetProcs();

        dawnProcSetProcs(&procs);
        procs.deviceSetUncapturedErrorCallback(cDevice, PrintDeviceError, nullptr);
        return device;
    }
#endif  // __EMSCRIPTEN__

HgiWebGPU::HgiWebGPU(wgpu::Device device)
: _device(device)
, _currentCmds(nullptr)
, _workToFlush(false)
{
    // get the webgpu device
    _device = GetDevice();

    // get the default command queue
    _commandQueue = _device.GetQueue();

    _capabilities.reset(new HgiWebGPUCapabilities(_device));
}

HgiWebGPU::~HgiWebGPU()
{
}

bool
HgiWebGPU::IsBackendSupported() const
{
    return true;
}

wgpu::Device
HgiWebGPU::GetPrimaryDevice() const
{
    return _device;
}

HgiGraphicsCmdsUniquePtr
HgiWebGPU::CreateGraphicsCmds(
    HgiGraphicsCmdsDesc const& desc)
{
    HgiWebGPUGraphicsCmds* gfxCmds(new HgiWebGPUGraphicsCmds(this, desc));
    return HgiGraphicsCmdsUniquePtr(gfxCmds);
}

HgiComputeCmdsUniquePtr
HgiWebGPU::CreateComputeCmds(HgiComputeCmdsDesc const& desc)
{
    HgiWebGPUComputeCmds* computeCmds = new HgiWebGPUComputeCmds(this, desc);
    if (!_currentCmds) {
        _currentCmds = computeCmds;
    }
    return HgiComputeCmdsUniquePtr(computeCmds);
}

HgiBlitCmdsUniquePtr
HgiWebGPU::CreateBlitCmds()
{
    HgiWebGPUBlitCmds* blitCmds = new HgiWebGPUBlitCmds(this);
    if (!_currentCmds) {
        _currentCmds = blitCmds;
    }
    return HgiBlitCmdsUniquePtr(blitCmds);
}

HgiTextureHandle
HgiWebGPU::CreateTexture(HgiTextureDesc const & desc)
{
    return HgiTextureHandle(new HgiWebGPUTexture(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyTexture(HgiTextureHandle* texHandle)
{
    _TrashObject(texHandle);
}

HgiTextureViewHandle
HgiWebGPU::CreateTextureView(HgiTextureViewDesc const & desc)
{
    if (!desc.sourceTexture) {
        TF_CODING_ERROR("Source texture is null");
    }

    HgiTextureHandle src =
        HgiTextureHandle(new HgiWebGPUTexture(this, desc), GetUniqueId());
    HgiTextureView* view = new HgiTextureView(desc);
    view->SetViewTexture(src);
    return HgiTextureViewHandle(view, GetUniqueId());
}

void
HgiWebGPU::DestroyTextureView(HgiTextureViewHandle* viewHandle)
{
    HgiTextureHandle texHandle = (*viewHandle)->GetViewTexture();
    _TrashObject(&texHandle);
    (*viewHandle)->SetViewTexture(HgiTextureHandle());
    delete viewHandle->Get();
    *viewHandle = HgiTextureViewHandle();
}

HgiSamplerHandle
HgiWebGPU::CreateSampler(HgiSamplerDesc const & desc)
{
    return HgiSamplerHandle(new HgiWebGPUSampler(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroySampler(HgiSamplerHandle* smpHandle)
{
    _TrashObject(smpHandle);
}

HgiBufferHandle
HgiWebGPU::CreateBuffer(HgiBufferDesc const & desc)
{
    return HgiBufferHandle(new HgiWebGPUBuffer(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyBuffer(HgiBufferHandle* bufHandle)
{
    _TrashObject(bufHandle);
}

HgiShaderFunctionHandle
HgiWebGPU::CreateShaderFunction(HgiShaderFunctionDesc const& desc)
{
    return HgiShaderFunctionHandle(
        new HgiWebGPUShaderFunction(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyShaderFunction(HgiShaderFunctionHandle* shaderFunctionHandle)
{
    _TrashObject(shaderFunctionHandle);
}

HgiShaderProgramHandle
HgiWebGPU::CreateShaderProgram(HgiShaderProgramDesc const& desc)
{
    return HgiShaderProgramHandle(
        new HgiWebGPUShaderProgram(desc), GetUniqueId());
}

void
HgiWebGPU::DestroyShaderProgram(HgiShaderProgramHandle* shaderProgramHandle)
{
    _TrashObject(shaderProgramHandle);
}


HgiResourceBindingsHandle
HgiWebGPU::CreateResourceBindings(HgiResourceBindingsDesc const& desc)
{
    return HgiResourceBindingsHandle(
        new HgiWebGPUResourceBindings(desc), GetUniqueId());
}

void
HgiWebGPU::DestroyResourceBindings(HgiResourceBindingsHandle* resHandle)
{
    _TrashObject(resHandle);
}

HgiGraphicsPipelineHandle
HgiWebGPU::CreateGraphicsPipeline(HgiGraphicsPipelineDesc const& desc)
{
    return HgiGraphicsPipelineHandle(
        new HgiWebGPUGraphicsPipeline(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyGraphicsPipeline(HgiGraphicsPipelineHandle* pipeHandle)
{
    _TrashObject(pipeHandle);
}

HgiComputePipelineHandle
HgiWebGPU::CreateComputePipeline(HgiComputePipelineDesc const& desc)
{
    return HgiComputePipelineHandle(
        new HgiWebGPUComputePipeline(this, desc), GetUniqueId());
}

void
HgiWebGPU::DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle)
{
    _TrashObject(pipeHandle);
}

TfToken const&
HgiWebGPU::GetAPIName() const {
    return HgiTokens->WebGPU;
}

HgiWebGPUCapabilities const*
HgiWebGPU::GetCapabilities() const
{
    return _capabilities.get();
}

HgiIndirectCommandEncoder*
HgiWebGPU::GetIndirectCommandEncoder() const
{
    return nullptr;
}

void
HgiWebGPU::StartFrame()
{
    // TODO
}

void
HgiWebGPU::EndFrame()
{
    // TODO
}

wgpu::Queue
HgiWebGPU::GetQueue() const
{
    return _commandQueue;
}

void
HgiWebGPU::EnqueueCommandBuffer(wgpu::CommandBuffer const &commandBuffer)
{
    _commandBuffers.push_back(commandBuffer);
}

void
HgiWebGPU::QueueSubmit()
{
    if( _commandBuffers.size() > 0)
    {
        // submit enqueued command buffers
        for (auto commandBuffer : _commandBuffers)
        {
            if (commandBuffer == nullptr)
                continue;

            _commandQueue.Submit(1, &commandBuffer);

            // free command buffers
            commandBuffer = nullptr;
        }
        _commandBuffers.clear();

        struct CompletedUserData
        {
            std::vector<HgiWebGPUCallback> completedHandlers;
            bool done;
        };
    }
}

int
HgiWebGPU::GetAPIVersion() const
{
    return GetCapabilities()->GetAPIVersion();
}

bool
HgiWebGPU::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
    TRACE_FUNCTION();

    if (cmds) {
        _workToFlush = Hgi::_SubmitCmds(cmds, wait);
        if (cmds == _currentCmds) {
            _currentCmds = nullptr;
        }
    }

    return _workToFlush;
}

PXR_NAMESPACE_CLOSE_SCOPE
