//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hio/imageRegistry.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/rankedTypeMap.h"

#include "pxr/imaging/hio/debugCodes.h"

#include "pxr/usd/ar/resolver.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/trace/trace.h"

#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(HIO_IMAGE_PLUGIN_RESTRICTION, "",
                  "Restricts HioImage plugin loading to the specified plugin");

TF_INSTANTIATE_SINGLETON(HioImageRegistry);

HioImageRegistry&
HioImageRegistry::GetInstance()
{
    return TfSingleton<HioImageRegistry>::GetInstance();
}

HioImageRegistry::HioImageRegistry() :
    _typeMap(std::make_unique<HioRankedTypeMap>())
{
    // Register all image types using plugin metadata.
    _typeMap->Add(TfType::Find<HioImage>(), "imageTypes",
                 HIO_DEBUG_TEXTURE_IMAGE_PLUGINS,
                 TfGetEnvSetting(HIO_IMAGE_PLUGIN_RESTRICTION));
}

HioImageSharedPtr
HioImageRegistry::_ConstructImage(std::string const & filename)
{
    TRACE_FUNCTION();

    // Lookup the plug-in type name based on the filename.
    const TfToken fileExtension(
            TfStringToLowerAscii(ArGetResolver().GetExtension(filename)));

    TfType const & pluginType = _typeMap->Find(fileExtension);

    if (!pluginType) {
        // Unknown prim type.
        TF_DEBUG(HIO_DEBUG_TEXTURE_IMAGE_PLUGINS).Msg(
                "[PluginLoad] Unknown image type '%s' for file '%s'\n",
                fileExtension.GetText(),
                filename.c_str());
        return nullptr;
    }

    PlugRegistry& plugReg = PlugRegistry::GetInstance();
    PlugPluginPtr const plugin = plugReg.GetPluginForType(pluginType);
    if (!plugin || !plugin->Load()) {
        TF_CODING_ERROR("[PluginLoad] PlugPlugin could not be loaded for "
                "TfType '%s'\n",
                pluginType.GetTypeName().c_str());
        return nullptr;
    }

    HioImageFactoryBase* const factory =
        pluginType.GetFactory<HioImageFactoryBase>();
    if (!factory) {
        TF_CODING_ERROR("[PluginLoad] Cannot manufacture type '%s' "
                "for image type '%s' for file '%s'\n",
                pluginType.GetTypeName().c_str(),
                fileExtension.GetText(),
                filename.c_str());

        return nullptr;
    }

    HioImageSharedPtr const instance = factory->New();
    if (!instance) {
        TF_CODING_ERROR("[PluginLoad] Cannot construct instance of type '%s' "
                "for image type '%s' for file '%s'\n",
                pluginType.GetTypeName().c_str(),
                fileExtension.GetText(),
                filename.c_str());
        return nullptr;
    }

    TF_DEBUG(HIO_DEBUG_TEXTURE_IMAGE_PLUGINS).Msg(
    	        "[PluginLoad] Loaded plugin '%s' for image type '%s' for "
                "file '%s'\n",
                pluginType.GetTypeName().c_str(),
                fileExtension.GetText(),
                filename.c_str());

    return instance;
}

bool
HioImageRegistry::IsSupportedImageFile(std::string const & filename)
{
    // We support image files for which we can construct an image object.
    return _ConstructImage(filename) != 0;
}

PXR_NAMESPACE_CLOSE_SCOPE

