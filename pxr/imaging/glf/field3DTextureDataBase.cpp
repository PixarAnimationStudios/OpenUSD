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
#include "pxr/imaging/glf/field3DTextureDataBase.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

PXR_NAMESPACE_OPEN_SCOPE

static
GlfField3DTextureDataFactoryBase *
_GetFactory()
{
    const TfType baseType = TfType::Find<GlfField3DTextureDataBase>();
    if (!baseType) {
        TF_CODING_ERROR("No base type");
        return nullptr;
    }

    const std::vector<TfType> pluginTypes = baseType.GetDirectlyDerivedTypes();
    if (pluginTypes.empty())
    {
        TF_WARN("No implementation to read F3D textures.");
        return nullptr;
    }
    
    const TfType pluginType = pluginTypes[0];

    const PlugRegistry& plugReg = PlugRegistry::GetInstance();
    PlugPluginPtr plugin = plugReg.GetPluginForType(pluginType);
    if (!plugin || !plugin->Load()) {
        TF_CODING_ERROR("[PluginLoad] PlugPlugin could not be loaded for "
                        "TfType '%s'\n",
                        pluginType.GetTypeName().c_str());
        return nullptr;
    }

    GlfField3DTextureDataFactoryBase * const factory = 
        pluginType.GetFactory<GlfField3DTextureDataFactoryBase>();
    if (!factory) {
        TF_CODING_ERROR("[PluginLoad] Cannot manufacture factory for "
                        "F3D plugin");
    }
    return factory;
}

GlfField3DTextureDataBaseRefPtr
GlfField3DTextureDataBase::New(
    std::string const &filePath,
    std::string const &fieldName,
    const int fieldIndex,
    std::string const &fieldPurpose,
    const size_t targetMemory)
{
    static GlfField3DTextureDataFactoryBase * const factory = _GetFactory();
    if (!factory) {
        return TfNullPtr;
    }
    return factory->_New(
        filePath, fieldName, fieldIndex, fieldPurpose, targetMemory);
}

PXR_NAMESPACE_CLOSE_SCOPE
