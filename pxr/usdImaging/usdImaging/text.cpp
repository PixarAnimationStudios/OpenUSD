//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdImaging/text.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "pxr/usdImaging/usdImaging/textBlock.h"
#include "pxr/usdImaging/usdImaging/textLine.h"
#include "pxr/usdImaging/usdImaging/textParagraph.h"
#include "pxr/usdImaging/usdImaging/textRegistry.h"
#include "pxr/usdImaging/usdImaging/textRun.h"
#include "pxr/usdImaging/usdImaging/textStyleChange.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdImagingText>();
}

UsdImagingTextSharedPtr UsdImagingText::_textSystem = nullptr;
std::mutex UsdImagingText::_initializeMutex;

//
// As this class is a pure interface class, it does not need a
// vtable.  However, it is possible that some users will use rtti.
// This will cause a problem for some of our compilers:
//
// In particular clang will throw a warning: -wweak-vtables
// For gcc, there is an issue were the rtti typeid's are different.
//
// As destruction of the class is not on the performance path,
// the body of the deleter is provided here, so a vtable is created
// in this compilation unit.
UsdImagingText::~UsdImagingText() = default;

bool
UsdImagingText::DefaultInitialize()
{
    std::lock_guard<std::mutex> lock(_initializeMutex);
    if (IsInitialized())
        return true;

    // Initialize the text system with default setting.
    std::string defaultFontDir;
#if defined(ARCH_OS_WINDOWS)
    char infoBuf[255];
    if (::GetSystemDirectory(infoBuf, 255) != 0)
    {
        defaultFontDir = infoBuf;
        defaultFontDir += "/../Fonts";
    }
#else
    // Use default system fonts folder
    defaultFontDir = "/System/Library/Fonts/Supplemental";
#endif
    UsdImagingText::TextSettingMap textSetting;
    textSetting[UsdImagingTextTokens->fontFolder] = defaultFontDir;
    textSetting[UsdImagingTextTokens->fontSubstitution] = "default";
    textSetting[UsdImagingTextTokens->tabSize] = "4";

    // Get and initialize a plugin from the registry.
    UsdImagingTextRegistry& registry = UsdImagingTextRegistry::GetInstance();
    _textSystem = registry._GetText(textSetting);
    return _textSystem != nullptr;
}

bool 
UsdImagingText::Initialize(const TextSettingMap& setting)
{
    if (IsInitialized())
        return true;
    else
    {
        // Get and initialize a plugin from the registry.
        UsdImagingTextRegistry& registry = UsdImagingTextRegistry::GetInstance();
        _textSystem = registry._GetText(setting);
        return _textSystem != nullptr;
    }
}

bool 
UsdImagingText::GenerateMarkupTextGeometries(UsdImagingTextRendererSharedPtr renderer, 
                                             std::shared_ptr<UsdImagingMarkupText> markupText,
                                             VtVec3fArray& geometries,
                                             VtVec4fArray& textCoords,
                                             VtVec3fArray& textColor,
                                             VtFloatArray& textOpacity,
                                             VtVec3fArray& lineColors,
                                             VtFloatArray& lineOpacities,
                                             VtVec3fArray& lineGeometries)
{
    if (!IsInitialized())
        return false;
    return _textSystem->_GenerateMarkupTextGeometries(renderer, markupText, geometries, textCoords,
        textColor, textOpacity, lineColors, lineOpacities, lineGeometries);
}

bool 
UsdImagingText::GenerateSimpleTextGeometries(UsdImagingTextRendererSharedPtr renderer, 
                                             const std::string& textData,
                                             const UsdImagingTextStyle& style, 
                                             VtVec3fArray& geometries,
                                             VtVec4fArray& textCoords, 
                                             VtVec3fArray& lineGeometries)
{
    if (!IsInitialized())
        return false;
    return _textSystem->_GenerateSimpleTextGeometries(renderer, textData, style,
        geometries, textCoords, lineGeometries);
}

PXR_NAMESPACE_CLOSE_SCOPE

