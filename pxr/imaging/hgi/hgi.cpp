//
// Copyright 2019 Pixar
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
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/debugCodes.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

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
