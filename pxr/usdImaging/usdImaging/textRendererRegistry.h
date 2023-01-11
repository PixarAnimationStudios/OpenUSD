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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_RENDERER_REGISTRY_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_RENDERER_REGISTRY_H

/// \file usdImaging/textRegistry.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using UsdImagingTextRendererSharedPtr = std::shared_ptr<class UsdImagingTextRenderer>;


/// \class UsdImagingTextRendererRegistry
///
/// Manages plugin registration and loading for UsdImagingTextRenderer subclasses.
///
class UsdImagingTextRendererRegistry : public TfSingleton<UsdImagingTextRendererRegistry>
{
public:
    USDIMAGING_API
    static UsdImagingTextRendererRegistry& GetInstance();

private:
    friend class TfSingleton<UsdImagingTextRendererRegistry>;
    UsdImagingTextRendererRegistry();

    friend class UsdImagingTextRenderer;

    UsdImagingTextRendererSharedPtr _GetTextRenderer(const std::string& renderer);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_RENDERER_REGISTRY_H
