//
// Copyright 2024 Pixar
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
#include "pxr/usdImaging/usdImaging/textRendererRegistry.h"
#include "pxr/usdImaging/usdImaging/textRenderer.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(UsdImagingTextRendererRegistry);

TF_MAKE_STATIC_DATA(TfType, _textRendererBaseType) {
    *_textRendererBaseType = TfType::Find<UsdImagingTextRenderer>();
}

UsdImagingTextRendererRegistry&
UsdImagingTextRendererRegistry::GetInstance()
{
    return TfSingleton<UsdImagingTextRendererRegistry>::GetInstance();
}

UsdImagingTextRendererRegistry::UsdImagingTextRendererRegistry()
{
}

UsdImagingTextRendererSharedPtr
UsdImagingTextRendererRegistry::_GetTextRenderer(const std::string& renderer)
{
    PlugRegistry& plugReg = PlugRegistry::GetInstance();
    std::set<TfType> types;
    PlugRegistry::GetAllDerivedTypes(*_textRendererBaseType, &types);
    TF_FOR_ALL(typeIt, types) {
        PlugPluginPtr plugin = plugReg.GetPluginForType(*typeIt);
        if (!plugin) {
            continue;
        }

        UsdImagingTextRendererFactoryBase* const factory =
            typeIt->GetFactory<UsdImagingTextRendererFactoryBase>();
        if (!factory) {
            return nullptr;
        }

        UsdImagingTextRendererSharedPtr const instance = factory->New();
        if (!instance) {
            return nullptr;
        }

        // If the renderer string is empty, we just return the first renderer.
        // Or else, we will check if it is the renderer's name.
        if (renderer.empty() || instance->Name() == renderer)
        {
            return instance;
        }
    }
    return nullptr;
}


PXR_NAMESPACE_CLOSE_SCOPE

