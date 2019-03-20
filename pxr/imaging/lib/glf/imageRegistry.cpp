//
// Copyright 2016 Pixar
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
#include "pxr/imaging/glf/imageRegistry.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/rankedTypeMap.h"

#include "pxr/imaging/glf/debugCodes.h"

#include "pxr/usd/ar/resolver.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/type.h"

#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(GLF_IMAGE_PLUGIN_RESTRICTION, "",
                  "Restricts GlfImage plugin loading to the specified plugin");

TF_INSTANTIATE_SINGLETON(GlfImageRegistry);

GlfImageRegistry&
GlfImageRegistry::GetInstance()
{
    return TfSingleton<GlfImageRegistry>::GetInstance();
}

GlfImageRegistry::GlfImageRegistry() :
    _typeMap(new GlfRankedTypeMap)
{
    // Register all image types using plugin metadata.
    _typeMap->Add(TfType::Find<GlfImage>(), "imageTypes",
                 GLF_DEBUG_TEXTURE_IMAGE_PLUGINS,
                 TfGetEnvSetting(GLF_IMAGE_PLUGIN_RESTRICTION));
}

GlfImageSharedPtr
GlfImageRegistry::_ConstructImage(std::string const & filename)
{
    static GlfImageSharedPtr NULL_IMAGE;

    // Lookup the plug-in type name based on the filename.
    TfToken fileExtension(
            TfStringToLower(ArGetResolver().GetExtension(filename)));

    TfType const & pluginType = _typeMap->Find(fileExtension);

    if (!pluginType) {
        // Unknown prim type.
        TF_DEBUG(GLF_DEBUG_TEXTURE_IMAGE_PLUGINS).Msg(
                "[PluginLoad] Unknown image type '%s'\n",
                fileExtension.GetText());
        return NULL_IMAGE;
    }

    PlugRegistry& plugReg = PlugRegistry::GetInstance();
    PlugPluginPtr plugin = plugReg.GetPluginForType(pluginType);
    if (!plugin || !plugin->Load()) {
        TF_CODING_ERROR("[PluginLoad] PlugPlugin could not be loaded for "
                "TfType '%s'\n",
                pluginType.GetTypeName().c_str());
        return NULL_IMAGE;
    }

    GlfImageFactoryBase* factory = pluginType.GetFactory<GlfImageFactoryBase>();
    if (!factory) {
        TF_CODING_ERROR("[PluginLoad] Cannot manufacture type '%s' "
                "for image type '%s'\n",
                pluginType.GetTypeName().c_str(),
                fileExtension.GetText());

        return NULL_IMAGE;
    }

    GlfImageSharedPtr instance = factory->New();
    if (!instance) {
        TF_CODING_ERROR("[PluginLoad] Cannot construct instance of type '%s' "
                "for image type '%s'\n",
                pluginType.GetTypeName().c_str(),
                fileExtension.GetText());
        return NULL_IMAGE;
    }

    TF_DEBUG(GLF_DEBUG_TEXTURE_IMAGE_PLUGINS).Msg(
    	        "[PluginLoad] Loaded plugin '%s' for image type '%s'\n",
                pluginType.GetTypeName().c_str(),
                fileExtension.GetText());

    return instance;
}

bool
GlfImageRegistry::IsSupportedImageFile(std::string const & filename)
{
    // We support image files for which we can construct an image object.
    return _ConstructImage(filename) != 0;
}

PXR_NAMESPACE_CLOSE_SCOPE

