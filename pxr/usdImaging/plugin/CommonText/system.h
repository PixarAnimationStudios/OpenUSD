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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_SYSTEM_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_SYSTEM_H

#include "definitions.h"
#include "globalSetting.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
class CommonTextTrueTypeFontDevice;
class CommonTextSystemImp;
class CommonTextTrueTypeSimpleLayoutManager;
class CommonTextTrueTypeGenericLayoutManager;
class CommonTextMultiLanguageHandler;

/// \class CommonTextSystem
///
/// The text system.
///
class CommonTextSystem
{
private:
    static std::unique_ptr<CommonTextSystem> _instance;

    CommonTextGlobalSetting _setting;
    CommonTextFontSubstitutionSetting _fontSubstitutionSetting;
    std::unique_ptr<CommonTextSystemImp> _imp;
    std::shared_ptr<CommonTextMultiLanguageHandler> _multiLanguageHandler;

public:
    CommonTextSystem();

    ~CommonTextSystem();

    /// The text system is a singleton class.
    static CommonTextSystem* Instance();

    /// The default initialization.
    CommonTextStatus Initialize();

    /// Initialize the text system with global setting and font device.
    CommonTextStatus Initialize(const CommonTextGlobalSetting& setting, 
                                std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice = nullptr);

    /// Shut down the text system and release the singleton.
    CommonTextStatus ShutDown();

    /// Is the text system is initialized.
    bool IsInitialized() const;

    /// Register a truetype font device.
    void AddTrueTypeFontDevice(std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice);

    /// Set the current font device.
    CommonTextStatus SetCurrentFontDevice(const std::string& fontDeviceName);

    /// Get the current font device.
    const std::shared_ptr<CommonTextTrueTypeFontDevice> GetCurrentFontDevice() const;

    /// Get the layout manager for single line single style text.
    CommonTextTrueTypeSimpleLayoutManager GetSimpleLayoutManager(
        const UsdImagingTextStyle& style,
        bool allowKernings = true);

    /// Get the layout manager for multiple line multiple style text.
    CommonTextTrueTypeGenericLayoutManager GetGenericLayoutManager(bool allowKernings = true);

    /// Get the font device for a specific text style.
    std::shared_ptr<CommonTextTrueTypeFontDevice> GetFontDevice(const UsdImagingTextStyle& style);

    /// Return the font device to the CommonTextSystem.
    CommonTextStatus ReturnFontDevice(const UsdImagingTextStyle& style, 
                                      std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice);

    /// Get the TextGlobalSetting.
    const CommonTextGlobalSetting& GetTextGlobalSetting() const;

    /// Get the reference of the default setting for font substitution.
    CommonTextFontSubstitutionSetting& GetFontSubstitutionSetting() { return _fontSubstitutionSetting; }

    /// Get the multi-language handler.
    std::shared_ptr<CommonTextMultiLanguageHandler> GetMultiLanguageHandler()
    {
        return _multiLanguageHandler;
    }

    /// Acquire the CommonTextFontMapCache.
    std::shared_ptr<CommonTextFontMapCache> GetFontMapCache();

    /// Do default initialization for the CommonTextFontMapCache.
    CommonTextStatus AddDefaultFontToFontMapCache();

    /// Acquire the default TrueType font list.
    std::shared_ptr<CommonTextStringArray> GetDefaultTTFontList();
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_SYSTEM_H
