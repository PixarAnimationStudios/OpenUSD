//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrmanLoader/rendererPlugin.h"

#include "pxr/base/arch/env.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/stringUtils.h"
#if HD_API_VERSION >= 90
#include "pxr/imaging/hd/renderDelegateInfo.h"
#endif
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#if HD_API_VERSION >= 90
#include "pxr/imaging/hd/retainedDataSource.h"
#if HD_API_VERSION >= 101
#include "pxr/imaging/hd/sceneIndexCreateArgsSchema.h"
#else
#include "pxr/imaging/hd/sceneIndexInputArgsSchema.h"
#endif
#endif

#include <filesystem>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdPrmanLoaderTokens, HDPRMAN_LOADER_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (ri)
    (mtlx)
    (OSL)
    (RmanCpp)

    ((outputsRi, "outputs:ri"))
);

typedef HdRenderDelegate* (*CreateDelegateFunc)(
    HdRenderSettingsMap const& settingsMap,
    TfToken const& rileyVariant,
    int xpuCpuConfig,
    std::vector<int> xpuGpuConfig);
typedef void (*DeleteDelegateFunc)(
    HdRenderDelegate* renderDelegate);

static const std::string k_RMANTREE("RMANTREE");
#if defined(ARCH_OS_WINDOWS)
static const std::string k_PATH("PATH");
#endif    

// This holds the OS specific plugin info data
static struct HdPrmanLoader
{
    static void Load();
    ~HdPrmanLoader();
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    void* libprman = nullptr;
#endif
    void* hdPrman = nullptr;
    CreateDelegateFunc createFunc = nullptr;
    DeleteDelegateFunc deleteFunc = nullptr;
    bool valid = false;
    std::string errorMsg;
} _hdPrman;

void 
HdPrmanLoader::Load()
{
}

HdPrmanLoader::~HdPrmanLoader()
{
    if (hdPrman) {
        // Note: OSX does not support clean unloading of hdPrman.dylib symbols
        ArchLibraryClose(hdPrman);
        hdPrman = nullptr;
    }
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    if (libprman) {
        ArchLibraryClose(libprman);
        libprman = nullptr;
    }
#endif
}

// Register the hdPrman loader plugin
TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdPrmanLoaderRendererPlugin>();
}

HdPrmanLoaderRendererPlugin::HdPrmanLoaderRendererPlugin()
{
    static bool inited = false;
    if (inited) {
        return;
    }
    inited = true;

    const std::string rmantree = TfGetenv(k_RMANTREE);
    if (rmantree.empty()) {
        _hdPrman.errorMsg = "The hdPrmanLoader backend requires $RMANTREE to be set.";
        return;
    }

    // Lookup USD plugins from the PlugRegistry and preload 
    // them incase they are not in our LD_LIBRARY_PATH.
    // Also save the usdLibPath so we can also find MaterialX libs.
    static const char* preloadUsdPlugins[] = { 
        "hio", "hdMtlx", "hdsi", "usdVolImaging", "usdMtlx" 
    };
    std::string usdLibPath;
    for (const char* preloadUsdPlugin : preloadUsdPlugins) {
        PlugPluginPtr plugin = 
            PlugRegistry::GetInstance().GetPluginWithName(preloadUsdPlugin);
        if (plugin && !plugin->GetPath().empty()) {
            usdLibPath = TfGetPathName(plugin->GetPath());
            if (!plugin->IsLoaded()) {
                ArchLibraryOpen(
                    plugin->GetPath(), 
                    ARCH_LIBRARY_NOW | ARCH_LIBRARY_GLOBAL
                );
            }
        }
    }

    // Preload the MaterialX and hdMtlx libs incase they are 
    // not in our LD_LIBRARY_PATH.
    if (!usdLibPath.empty()) {
        for (const auto& entry 
            : std::filesystem::directory_iterator(usdLibPath)) {
            const std::string filename = entry.path().filename().string();
            if ((TfStringStartsWith(filename, "libMaterialX") 
            || filename.find("hdMtlx") != std::string::npos
            || filename.find("hdsi") != std::string::npos)
                && TfStringEndsWith(filename, ARCH_LIBRARY_SUFFIX)) {
                ArchLibraryOpen(
                    entry.path().string(), 
                    ARCH_LIBRARY_NOW | ARCH_LIBRARY_GLOBAL
                );
            }
        }
    }

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    // Open $RMANTREE/lib/libprman.so into the global namespace
    const std::string libprmanPath =
        TfStringCatPaths(rmantree, "lib/libprman" ARCH_LIBRARY_SUFFIX);
    _hdPrman.libprman = ArchLibraryOpen(
        libprmanPath,
        ARCH_LIBRARY_NOW | ARCH_LIBRARY_GLOBAL);
    if (!_hdPrman.libprman) {
        _hdPrman.errorMsg = TfStringPrintf("Could not load libprman: %s",
            ArchLibraryError().c_str());
        return;
    }

#elif defined(ARCH_OS_WINDOWS)
    // Append PATH environment with %RMANTREE%\bin and %RMANTREE%\lib
    std::string path = TfGetenv(k_PATH);
    path += ';' + TfStringCatPaths(rmantree, "bin");
    path += ';' + TfStringCatPaths(rmantree, "lib");
    TfSetenv(k_PATH, path.c_str());
#endif

    // HdPrman is assumed to be next to hdPrmanLoader (this plugin)
    PlugPluginPtr plugin =
        PlugRegistry::GetInstance().GetPluginWithName("hdPrman");

    if (!plugin) {
        _hdPrman.errorMsg =
            TfStringPrintf("Could not find hdPrman plugin registration.");
        return;
    }

    _hdPrman.hdPrman = ArchLibraryOpen(
        plugin->GetPath(),
        ARCH_LIBRARY_NOW | ARCH_LIBRARY_LOCAL);

    if (!_hdPrman.hdPrman) {
        _hdPrman.errorMsg =
            TfStringPrintf("Could not load versioned hdPrman backend: %s",
                ArchLibraryError().c_str());
        return;
    }

    _hdPrman.createFunc = reinterpret_cast<CreateDelegateFunc>(
        ArchLibraryGetSymbolAddress(_hdPrman.hdPrman,
                                    "HdPrmanLoaderCreateDelegate"));
    _hdPrman.deleteFunc = reinterpret_cast<DeleteDelegateFunc>(
        ArchLibraryGetSymbolAddress(_hdPrman.hdPrman,
                                    "HdPrmanLoaderDeleteDelegate"));

    if (!_hdPrman.createFunc || !_hdPrman.deleteFunc) {
        _hdPrman.errorMsg = "hdPrmanLoader factory methods could not be found "
            "in hdPrman plugin.";
        return;
    }

    _hdPrman.valid = true;
}

HdPrmanLoaderRendererPlugin::~HdPrmanLoaderRendererPlugin() = default;

#if HD_API_VERSION >= 90

static
HdRenderDelegateInfo
_GetRenderDelegateInfo()
{
    HdRenderDelegateInfo info;

    // Re-implemented from HdPrmanRenderDelegate::GetMaterialBindingPurpose()
    info.materialBindingPurpose = HdTokens->full;

    // Re-implemented from HdPrmanRenderDelegate::GetMaterialRenderContexts()
    info.materialRenderContexts = {
          _tokens->ri
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
        , _tokens->mtlx
#endif
    };

    // Re-reimplemented from HdPrmanRenderDelegate::GetRenderSettingsNamespaces().
    info.renderSettingsNamespaces = {
        _tokens->ri, _tokens->outputsRi
    };

    // Default from HdRenderDelegate:IsPrimvarFilteringNeeded()
    info.isPrimvarFilteringNeeded = false;

    // Re-implemented from HdPrmanMaterial::GetShaderSourceTypes()
    if (TfGetenvInt("PRMAN_OSL_BEFORE_RIXPLUGINS", 1)) {
        info.shaderSourceTypes = {
            _tokens->OSL
            , _tokens->RmanCpp
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
            , _tokens->mtlx
#endif
        };
    } else {
        info.shaderSourceTypes = {
            _tokens->RmanCpp
            , _tokens->OSL
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
            , _tokens->mtlx
#endif
        };
    }

    // true since HdPrmanDelegate::GetSupportedSprimTypes contains
    // coordSys.
    info.isCoordSysSupported = true;

    return info;
}

HdContainerDataSourceHandle
#if HD_API_VERSION >= 101
HdPrmanLoaderRendererPlugin::GetSceneIndexCreateArgs() const
#else
HdPrmanLoaderRendererPlugin::GetSceneIndexInputArgs() const
#endif
{
    static HdContainerDataSourceHandle const result =
#if HD_API_VERSION >= 101
        HdSceneIndexCreateArgsSchema::Builder()
#else
        HdSceneIndexInputArgsSchema::Builder()
#endif
            .SetMotionBlurSupport(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .SetCameraMotionBlurSupport(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .SetLegacyRenderDelegateInfo(
                HdRetainedTypedSampledDataSource<HdRenderDelegateInfo>::New(
                    _GetRenderDelegateInfo()))
            .Build();

    return result;
}

#endif // HD_API_VERSION >= 90

int HdPrmanLoaderRendererPlugin::_GetCpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    // Get CPU Config (default to on)
    int xpuCpuConfig = 1;

    // Check settingsMap override
    const auto& settingsXpuCpuConfig = settingsMap.find(
        HdPrmanLoaderTokens->xpuCpuConfig);
    if (settingsXpuCpuConfig != settingsMap.end() 
        && settingsXpuCpuConfig->second.IsHolding<int>()) {
        xpuCpuConfig = settingsXpuCpuConfig->second.UncheckedGet<int>();
    }

    // Check environment variable override
    xpuCpuConfig = TfGetenvInt("RMAN_XPU_CPUCONFIG", xpuCpuConfig);

    return xpuCpuConfig;
}

std::vector<int> HdPrmanLoaderRendererPlugin::_GetGpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    // Get GPU Config (default first gpu on)
    std::vector<int> xpuGpuConfig({0});

    // Check settingsMap override
    const auto& settingsXpuGpuConfig = settingsMap.find(
        HdPrmanLoaderTokens->xpuGpuConfig);
    if (settingsXpuGpuConfig != settingsMap.end() 
        && settingsXpuGpuConfig->second.IsHolding<std::vector<int>>()) {
        xpuGpuConfig = settingsXpuGpuConfig->second.UncheckedGet<std::vector<int>>();
    }

    // Check environment variable override
    if (ArchHasEnv("RMAN_XPU_GPUCONFIG")) {
        xpuGpuConfig = std::vector<int>();
        std::string xpuGpuConfigEnv = TfGetenv("RMAN_XPU_GPUCONFIG", "");
        if(!xpuGpuConfigEnv.empty()) {
            std::vector<std::string> gpus = TfStringSplit(xpuGpuConfigEnv, ",");
            for(const std::string& gpu : gpus) {
                if (gpu.empty()) continue;
                try {
                    xpuGpuConfig.push_back(std::stoi(gpu));
                }
                catch (const std::invalid_argument&) {
                    TF_WARN("Invalid gpu device in RMAN_XPU_GPUCONFIG environment"
                        " variable. Must be a comma seperated list of integers.");
                }
            }
        }
    }

    return xpuGpuConfig;
}

HdRenderDelegate*
HdPrmanLoaderRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    if (_hdPrman.valid) {
        return _hdPrman.createFunc(
            settingsMap, 
            _GetRenderVariant(), 
            _GetCpuConfig(settingsMap),
            _GetGpuConfig(settingsMap));
    } else {
        TF_WARN("Could not create hdPrman instance: %s",
                _hdPrman.errorMsg.c_str());
        return nullptr;
    }
}


HdRenderDelegate*
HdPrmanLoaderRendererPlugin::CreateRenderDelegate()
{
    return CreateRenderDelegate(HdRenderSettingsMap());
}

void
HdPrmanLoaderRendererPlugin::DeleteRenderDelegate(
    HdRenderDelegate *renderDelegate)
{
    if (_hdPrman.valid) {
        _hdPrman.deleteFunc(renderDelegate);
    }
}

static
bool
_IsSupported(std::string * const reasonWhyNot = nullptr)
{
    // TODO: Should we disable XPU gpus with gpuEnabled off?
    if (!_hdPrman.valid) {
        TF_DEBUG(HD_RENDERER_PLUGIN).Msg(
            "hdPrman renderer plugin unsupported: %s\n",
            _hdPrman.errorMsg.c_str());
        if (reasonWhyNot) {
            *reasonWhyNot = "hdPrman renderer plugin unsupported: " +
                _hdPrman.errorMsg;
        }
    }

    return _hdPrman.valid;
}

bool
HdPrmanLoaderRendererPlugin::IsSupported(
#if HD_API_VERSION >= 103
    const HdRendererCreateArgsSchema &rendererCreateArgs,
    std::string * reasonWhyNot
#elif HD_API_VERSION >= 83
    HdRendererCreateArgs const &rendererCreateArgs,
    std::string * reasonWhyNot
#elif PXR_VERSION < 2305
    bool gpuEnabled = true
#endif
    ) const
{
    return _IsSupported(
#if HD_API_VERSION >= 83
        reasonWhyNot
#endif
        );
}

PXR_NAMESPACE_CLOSE_SCOPE
