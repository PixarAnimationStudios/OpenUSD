//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/debugCodes.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

#define NOT_EMPTY_NAME(s) ((s).empty() ? "UNNAMED" : (s).c_str())

#define HGI_TEST_CODING_ERROR(cond, ...) if (!(cond))\
    { TF_CODING_ERROR(__VA_ARGS__); }

TF_DEFINE_ENV_SETTING(HGI_ENABLE_VULKAN, 0,
                      "Enable Vulkan as platform default Hgi backend (WIP)");

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<Hgi>();
}

Hgi::Hgi()
    : _uniqueIdCounter(1)
{
}

Hgi::~Hgi() = default;

HgiTextureHandle
Hgi::CreateTexture(HgiTextureDesc const& desc)
{
    HGI_TEST_CODING_ERROR(desc.dimensions[0] != 0,
        "%s: x dimension must be non-zero",
        NOT_EMPTY_NAME(desc.debugName));
    HGI_TEST_CODING_ERROR(desc.dimensions[0] != 0,
        "%s: x dimension must be non-zero",
        NOT_EMPTY_NAME(desc.debugName));
    HGI_TEST_CODING_ERROR(desc.usage != 0,
        "%s: Usage must be defined",
        NOT_EMPTY_NAME(desc.debugName));
    HGI_TEST_CODING_ERROR(desc.sampleCount == HgiSampleCount1 || 
            desc.type == HgiTextureType2D,
            "%s: multisampled textures must be 2d",
            NOT_EMPTY_NAME(desc.debugName));
    if (HgiIsCompressed(desc.format)) {
        if (desc.type == HgiTextureType2D) {
            HGI_TEST_CODING_ERROR(
                desc.dimensions[0] % 4 == 0 && desc.dimensions[1] % 4 == 0,
                "%s: compressed textures must be block sized",
                    NOT_EMPTY_NAME(desc.debugName));
        } else if (desc.type == HgiTextureType3D) {
            HGI_TEST_CODING_ERROR(desc.dimensions[0] % 4 == 0
                && desc.dimensions[1] % 4 == 0
                && desc.dimensions[2] % 4 == 0,
                "%s: compressed textures must be block sized",
                    NOT_EMPTY_NAME(desc.debugName));
        }
        else {
            TF_CODING_ERROR(
                "%s: Compressed texture must be of type "
                "HgiTextureType2D or HgiTextureType3D",
                    NOT_EMPTY_NAME(desc.debugName));
        }
    }
    return _CreateTexture(desc);
}

HgiTextureViewHandle
Hgi::CreateTextureView(HgiTextureViewDesc const& desc)
{
    HGI_TEST_CODING_ERROR(desc.sourceTexture, "%s: source texture invalid",
        NOT_EMPTY_NAME(desc.debugName));
    return _CreateTextureView(desc);
}

HgiBufferHandle
Hgi::CreateBuffer(HgiBufferDesc const& desc)
{
    HGI_TEST_CODING_ERROR(desc.byteSize != 0,
        "%s: buffers must be non-zero sized",
        NOT_EMPTY_NAME(desc.debugName));
    HGI_TEST_CODING_ERROR(
        (desc.usage & HgiBufferUsageVertex) == 0 || desc.vertexStride != 0,
        "%s: vertex buffers must provide stride!",
            NOT_EMPTY_NAME(desc.debugName));
    return _CreateBuffer(desc);
}

HgiResourceBindingsHandle
Hgi::CreateResourceBindings(HgiResourceBindingsDesc const& desc)
{
    HGI_TEST_CODING_ERROR(!desc.buffers.empty() || !desc.textures.empty(),
        "%s: buffers and textures vectors cannot be empty",
        NOT_EMPTY_NAME(desc.debugName));
    for (uint32_t i = 0; i < desc.buffers.size(); i++) {
        const HgiBufferBindDesc& bufDesc = desc.buffers[i];
        HGI_TEST_CODING_ERROR(bufDesc.buffers.size() == bufDesc.offsets.size(),
            "%s.buffers[%u]: Buffer count (%zu) != Offset count (%zu)",
                NOT_EMPTY_NAME(desc.debugName), i,
                bufDesc.buffers.size(), bufDesc.offsets.size());
        
        if (!bufDesc.sizes.empty()) {
            HGI_TEST_CODING_ERROR(
                bufDesc.buffers.size() == bufDesc.sizes.size(),
                "%s.buffers[%u]: Buffer count (%zu) != Sizes count (%zu)",
                    NOT_EMPTY_NAME(desc.debugName), i,
                    bufDesc.buffers.size(), bufDesc.sizes.size());
        }

        HGI_TEST_CODING_ERROR(
            bufDesc.resourceType == HgiBindResourceTypeUniformBuffer ||
            bufDesc.resourceType == HgiBindResourceTypeStorageBuffer,
            "%s.buffers[%u]: Unknown buffer type to bind",
                NOT_EMPTY_NAME(desc.debugName), i);
        
        uint32_t const offset = bufDesc.offsets.front();
        uint32_t const size = bufDesc.sizes.empty() ? 0 : bufDesc.sizes.front();
        HGI_TEST_CODING_ERROR(size != 0 || offset == 0,
            "%s.buffers[%u]: Invalid size (%u) for buffer with offset (%u)",
                NOT_EMPTY_NAME(desc.debugName), i, size, offset);

        for (uint32_t j = 0; j < bufDesc.buffers.size(); j++) {
            HGI_TEST_CODING_ERROR(bufDesc.buffers[j],
                "%s.buffers[%u].buffers[%u] invalid",
                NOT_EMPTY_NAME(desc.debugName), i, j);
        }
    }

    for (uint32_t i = 0; i < desc.textures.size(); i++) {
        const HgiTextureBindDesc& texDesc = desc.textures[i];

        HGI_TEST_CODING_ERROR(
            texDesc.resourceType == HgiBindResourceTypeSampledImage ||
            texDesc.resourceType == HgiBindResourceTypeCombinedSamplerImage ||
            texDesc.resourceType == HgiBindResourceTypeStorageImage,
            "%s.textures[%u]: Unknown texture type to bind",
                NOT_EMPTY_NAME(desc.debugName), i);

        for (uint32_t j = 0; j < texDesc.textures.size(); j++) {
            HGI_TEST_CODING_ERROR(texDesc.textures[j],
                "%s.textures[%u].textures[%u] invalid",
                    NOT_EMPTY_NAME(desc.debugName), i, j);
        }
        for (uint32_t j = 0; j < texDesc.samplers.size(); j++) {
            // Don't need a sampler for StorageImage type
            if (texDesc.resourceType != HgiBindResourceTypeStorageImage) {
                HGI_TEST_CODING_ERROR(texDesc.samplers[j],
                "%s.textures[%u].samplers[%u] invalid",
                    NOT_EMPTY_NAME(desc.debugName), i, j);
            }
        }
    }
    return _CreateResourceBindings(desc);
}

void
Hgi::SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
    TRACE_FUNCTION();

    if (cmds && TF_VERIFY(!cmds->IsSubmitted())) {
        _SubmitCmds(cmds, wait);
        cmds->_SetSubmitted();
    }
}

static Hgi*
_MakeNewPlatformDefaultHgi()
{
    TF_DEBUG(HGI_DEBUG_INSTANCE_CREATION).Msg("Attempting to create platform "
        "default Hgi\n");
    
    // We use the plugin system to construct derived Hgi classes to avoid any
    // linker complications.

    PlugRegistry& plugReg = PlugRegistry::GetInstance();

    const char* hgiType = 
        #if defined(ARCH_OS_LINUX)
            "HgiGL";
        #elif defined(ARCH_OS_DARWIN)
            "HgiMetal";
        #elif defined(ARCH_OS_WINDOWS)
            "HgiGL";
        #else
            ""; 
            #error Unknown Platform
            return nullptr;
        #endif

    if (TfGetEnvSetting(HGI_ENABLE_VULKAN)) {
        #if defined(PXR_VULKAN_SUPPORT_ENABLED)
            hgiType = "HgiVulkan";
        #else
            TF_CODING_ERROR(
                "Build requires PXR_VULKAN_SUPPORT_ENABLED=true to use Vulkan");
        #endif
    }

    TF_DEBUG(HGI_DEBUG_INSTANCE_CREATION).Msg("Platform default Hgi: "
        "%s\n", hgiType);

    const TfType plugType = plugReg.FindDerivedTypeByName<Hgi>(hgiType);

    PlugPluginPtr plugin = plugReg.GetPluginForType(plugType);
    if (!plugin || !plugin->Load()) {
        TF_CODING_ERROR(
            "[PluginLoad] PlugPlugin could not be loaded for TfType '%s'\n",
            plugType.GetTypeName().c_str());
        return nullptr;
    }

    HgiFactoryBase* factory = plugType.GetFactory<HgiFactoryBase>();
    if (!factory) {
        TF_CODING_ERROR("[PluginLoad] Cannot manufacture type '%s' \n",
                plugType.GetTypeName().c_str());
        return nullptr;
    }

    Hgi* instance = factory->New();
    if (!instance) {
        TF_CODING_ERROR("[PluginLoad] Cannot construct instance of type '%s'\n",
                plugType.GetTypeName().c_str());
        return nullptr;
    }

    if (!instance->IsBackendSupported()) {
        TF_DEBUG(HGI_DEBUG_INSTANCE_CREATION).Msg("Hgi %s is not supported\n",
            hgiType);
        // XXX Currently, returning nullptr (rather than a non-supported hgi 
        // instance) causes a crash in one of our studio tests. We disable the
        // desired behavior until we can fix the test. 
        return instance;
        // delete instance;
        // return nullptr;
    }

    TF_DEBUG(HGI_DEBUG_INSTANCE_CREATION).Msg("Successfully created platform "
        "default Hgi %s\n", hgiType);

    return instance;
}

static Hgi*
_MakeNamedHgi(const TfToken& hgiToken)
{
    TF_DEBUG(HGI_DEBUG_INSTANCE_CREATION).Msg("Attempting to create named Hgi "
        "%s\n", hgiToken.GetText());
    
    std::string hgiType;

    if (hgiToken == HgiTokens->OpenGL) {
#if defined(PXR_GL_SUPPORT_ENABLED)
        hgiType = "HgiGL";
#endif
    } else if (hgiToken == HgiTokens->Vulkan) {
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
        hgiType = "HgiVulkan";
#endif
    } else if (hgiToken == HgiTokens->Metal) {
#if defined(PXR_METAL_SUPPORT_ENABLED)
        hgiType = "HgiMetal";
#endif
    } else if (hgiToken.IsEmpty()) {
        return _MakeNewPlatformDefaultHgi();
    } else {
        // If an invalid token is provided, return nullptr.
        TF_CODING_ERROR("Unsupported token %s was provided.",
                        hgiToken.GetText());
        return nullptr;
    }

    // If a valid, non-empty token was provided but that Hgi type is 
    // unsupported by the build, return nullptr.
    if (hgiType.empty()) {
        TF_CODING_ERROR("Build does not support proposed Hgi type %s on "
                        "this platform.", hgiType.c_str());
        return nullptr;
    }

    PlugRegistry& plugReg = PlugRegistry::GetInstance();

    const TfType plugType = plugReg.FindDerivedTypeByName<Hgi>(hgiType);

    PlugPluginPtr plugin = plugReg.GetPluginForType(plugType);
    if (!plugin || !plugin->Load()) {
        TF_CODING_ERROR(
            "[PluginLoad] PlugPlugin could not be loaded for TfType '%s'\n",
            plugType.GetTypeName().c_str());
        return nullptr;
    }

    HgiFactoryBase* factory = plugType.GetFactory<HgiFactoryBase>();
    if (!factory) {
        TF_CODING_ERROR("[PluginLoad] Cannot manufacture type '%s' \n",
            plugType.GetTypeName().c_str());
        return nullptr;
    }

    Hgi* instance = factory->New();
    if (!instance) {
        TF_CODING_ERROR("[PluginLoad] Cannot construct instance of type '%s'\n",
            plugType.GetTypeName().c_str());
        return nullptr;
    }

    if (!instance->IsBackendSupported()) {
        TF_DEBUG(HGI_DEBUG_INSTANCE_CREATION).Msg("Hgi %s is not supported\n",
            hgiType.c_str());
        delete instance;
        return nullptr;
    }

    TF_DEBUG(HGI_DEBUG_INSTANCE_CREATION).Msg("Successfully created named Hgi "
        "%s\n", hgiType.c_str());

    return instance;
}

Hgi*
Hgi::GetPlatformDefaultHgi()
{
    TF_WARN("GetPlatformDefaultHgi is deprecated. "
            "Please use CreatePlatformDefaultHgi");

    return _MakeNewPlatformDefaultHgi();
}

HgiUniquePtr
Hgi::CreatePlatformDefaultHgi()
{
    return HgiUniquePtr(_MakeNewPlatformDefaultHgi());
}

HgiUniquePtr 
Hgi::CreateNamedHgi(const TfToken& hgiToken)
{
    return HgiUniquePtr(_MakeNamedHgi(hgiToken));
}

bool
Hgi::IsSupported(const TfToken& hgiToken)
{
    // TODO: By current design, a Hgi instance is created and initialized as a 
    // method of confirming support on a platform. Once this is done, the 
    // instance is destroyed along with the created API contexts. This is not 
    // the best way to check for support on a platform and we'd like to change 
    // this approach in the future.

    HgiUniquePtr instance = nullptr;
    if (hgiToken.IsEmpty()) {
        instance = CreatePlatformDefaultHgi();
    } else {
        instance = CreateNamedHgi(hgiToken);
    }

    if (instance) {
        return instance->IsBackendSupported();
    }

    return false;
}

uint64_t
Hgi::GetUniqueId()
{
    return _uniqueIdCounter.fetch_add(1);
}

bool
Hgi::_SubmitCmds(HgiCmds* cmds, HgiSubmitWaitType wait)
{
    return cmds->_Submit(this, wait);
}

PXR_NAMESPACE_CLOSE_SCOPE
