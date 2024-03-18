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

#include "system.h"
#include "fontDevice.h"
#include "freeTypeFontDevice.h"
#include "genericLayout.h"
#include "languageAttribute.h"
#include "multiLanguageHandler.h"
#include "simpleLayout.h"
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE
/// The implementation class of CommonTextSystem.
class CommonTextSystemImp
{
    friend CommonTextSystem;

private:
    std::unordered_map<std::string, std::shared_ptr<CommonTextTrueTypeFontDevice>> _fontDeviceMap;
    std::shared_ptr<CommonTextTrueTypeFontDevice> _currentFontDevice;

    typedef tbb::concurrent_queue<std::shared_ptr<CommonTextTrueTypeFontDevice>> ConcurrentDeviceQueue;
    tbb::concurrent_unordered_map<UsdImagingTextStyle, std::unique_ptr<ConcurrentDeviceQueue>,
        std::hash<UsdImagingTextStyle>>
        _styleToFontDeviceMap;

public:
    CommonTextSystemImp() {}
    ~CommonTextSystemImp() { ShutDown(); }

    /// The default initialization.
    CommonTextStatus Initialize()
    {
        std::shared_ptr<CommonTextFreeTypeFontDevice> device = std::make_shared<CommonTextFreeTypeFontDevice>();

        // If _currentFontDevice is not set, we will try to set Freetype as default font device.
        if (!_currentFontDevice)
        {
            // Initialize the fontdevice.
            device->Initialize();

            // If the font device is available in the current OS, set it as current font device.
            if (device->IsAvailable())
                _currentFontDevice = device;
        }

        // Add the font device to the map.
        _fontDeviceMap.emplace(device->Name(), device);

        // Initialize the language attribute sets.
        InitializeLanguageAttributeSet();

        return CommonTextStatus::CommonTextStatusSuccess;
    }

    /// Initialize the text system with global setting and font device.
    CommonTextStatus Initialize(std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice)
    {
        // Add the fontDevice to fontDeviceMap.
        AddTrueTypeFontDevice(fontDevice);
        return Initialize();
    }

    /// Shutdown the text system.
    void ShutDown()
    {
        for (auto&& it : _fontDeviceMap)
            it.second->ShutDown();
        _fontDeviceMap.clear();
        _styleToFontDeviceMap.clear();
        _currentFontDevice = nullptr;
    }

    /// Is the text system is initialized.
    bool IsInitialized() const { return _currentFontDevice != nullptr; }

    /// Register a truetype font device.
    void AddTrueTypeFontDevice(std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice)
    {
        if (fontDevice)
        {
            // If _currentFontDevice is not set, we will try to set the font device as default font
            // device.
            if (!_currentFontDevice)
            {
                // Initialize the fontdevice.
                fontDevice->Initialize();

                // If the font device is available in the current OS, set it as current font device.
                if (fontDevice->IsAvailable())
                    _currentFontDevice = fontDevice;
            }

            // Add the font device to the map.
            _fontDeviceMap.emplace(fontDevice->Name(), fontDevice);
        }
    }

    /// Set the current font device.
    CommonTextStatus SetCurrentFontDevice(const std::string& fontDeviceName)
    {
        // Set the font device if it is already added to the map and it is available in current OS.
        auto it = _fontDeviceMap.find(fontDeviceName);
        if (it != _fontDeviceMap.end() && it->second)
        {
            // Initialize the fontdevice.
            it->second->Initialize();

            if (it->second->IsAvailable())
            {
                _currentFontDevice = it->second;
                return CommonTextStatus::CommonTextStatusSuccess;
            }
        }
        return CommonTextStatus::CommonTextStatusFail;
    }

    /// Get the current font device.
    const std::shared_ptr<CommonTextTrueTypeFontDevice> GetCurrentFontDevice() const
    {
        return _currentFontDevice;
    }

    /// Get a font device from specified text style.
    std::shared_ptr<CommonTextTrueTypeFontDevice> GetFontDevice(const UsdImagingTextStyle& style)
    {
        // Return nullptr if the CommonTextSystem is not initialized.
        if (!IsInitialized())
            return nullptr;

        // Find the queue of font devices which is applied with the style.
        auto it = _styleToFontDeviceMap.find(style);
        if (it == _styleToFontDeviceMap.end())
        {
            // Create a queue if the queue is not found.
            auto pair =
                _styleToFontDeviceMap.emplace(style, std::make_unique<ConcurrentDeviceQueue>());
            it = pair.first;
        }

        std::shared_ptr<CommonTextTrueTypeFontDevice> ttFontDevice;
        // Try to pop up a font device from the queue.
        if (it->second && it->second->try_pop(ttFontDevice))
        {
            return ttFontDevice;
        }
        else
        {
            // If the queue is empty, we will clone the current font device, and apply the text
            // style. We don't add the font device to the queue, but directly return it. The font
            // device will be added to the queue when the user doesn't require it.
            ttFontDevice = _currentFontDevice->Clone();
            if (ttFontDevice->ApplyTextStyle(style) == CommonTextStatus::CommonTextStatusSuccess)
            {
                return ttFontDevice;
            }
            else
                return nullptr;
        }
    }

    CommonTextStatus ReturnFontDevice(const UsdImagingTextStyle& style, 
                                      std::shared_ptr<CommonTextTrueTypeFontDevice> device)
    {
        // Return nullptr if the CommonTextSystem is not initialized.
        if (!IsInitialized())
            return CommonTextStatus::CommonTextStatusNotInitialized;

        // Find the queue for the textStyle, and add the fontDevice to the queue.
        auto it = _styleToFontDeviceMap.find(style);
        if (it != _styleToFontDeviceMap.end() && it->second)
        {
            it->second->push(device);
            return CommonTextStatus::CommonTextStatusSuccess;
        }
        else
            // If the queue is not found, there should be some logic error.
            return CommonTextStatus::CommonTextStatusInvalidArg;
    }
};

std::unique_ptr<CommonTextSystem> CommonTextSystem::_instance = nullptr;
CommonTextSystem* 
CommonTextSystem::Instance()
{
    if (!_instance)
        _instance = std::make_unique<CommonTextSystem>();
    return _instance.get();
}

CommonTextSystem::CommonTextSystem()
{
    _imp = std::make_unique<CommonTextSystemImp>();
}

CommonTextSystem::~CommonTextSystem() {}

CommonTextStatus 
CommonTextSystem::Initialize()
{
    if (_multiLanguageHandler == nullptr)
        _multiLanguageHandler = std::make_shared<CommonTextMultiLanguageHandler>();

    // Get the multi-language handler implementation during initialization.
    // Waiting until it is needed, can cause threading issues on Windows
    // because CoInitialize() might be called on a different thread than
    // CoUninitialize().
    _multiLanguageHandler->AcquireImplementation();

    // Add default font device to map.
    return _imp->Initialize();
}

CommonTextStatus 
CommonTextSystem::Initialize(
    const CommonTextGlobalSetting& setting, 
    std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice)
{
    _setting = setting;

    if (_multiLanguageHandler == nullptr)
        _multiLanguageHandler = std::make_shared<CommonTextMultiLanguageHandler>();

    // Get the multi-language handler implementation during initialization.
    // Waiting until it is needed, can cause threading issues on Windows
    // because CoInitialize() might be called on a different thread than
    // CoUninitialize().
    _multiLanguageHandler->AcquireImplementation();
    if (fontDevice)
        return _imp->Initialize(fontDevice);
    else
        return _imp->Initialize();
}

CommonTextStatus 
CommonTextSystem::ShutDown()
{
    _imp->ShutDown();
    return CommonTextStatus::CommonTextStatusSuccess;
}

bool 
CommonTextSystem::IsInitialized() const
{
    return _imp->IsInitialized();
}

void 
CommonTextSystem::AddTrueTypeFontDevice(
    std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice)
{
    _imp->AddTrueTypeFontDevice(fontDevice);
}

CommonTextStatus 
CommonTextSystem::SetCurrentFontDevice(const std::string& fontDeviceName)
{
    return _imp->SetCurrentFontDevice(fontDeviceName);
}

const std::shared_ptr<CommonTextTrueTypeFontDevice> 
CommonTextSystem::GetCurrentFontDevice() const
{
    return _imp->GetCurrentFontDevice();
}

CommonTextTrueTypeSimpleLayoutManager 
CommonTextSystem::GetSimpleLayoutManager(
    const UsdImagingTextStyle& style, bool /*allowKernings*/)
{
    if (IsInitialized())
    {
        return CommonTextTrueTypeSimpleLayoutManager(this, style);
    }
    else
        // Return an empty layout manager if CommonTextSystem is not initialized.
        return CommonTextTrueTypeSimpleLayoutManager(nullptr, style);
}

CommonTextTrueTypeGenericLayoutManager 
CommonTextSystem::GetGenericLayoutManager(bool /*allowKernings*/)
{
    if (IsInitialized())
    {
        return CommonTextTrueTypeGenericLayoutManager(this);
    }
    else
        // Return an empty layout manager if CommonTextSystem is not initialized.
        return CommonTextTrueTypeGenericLayoutManager(nullptr);
}

std::shared_ptr<CommonTextTrueTypeFontDevice> 
CommonTextSystem::GetFontDevice(const UsdImagingTextStyle& style)
{
    return _imp->GetFontDevice(style);
}

CommonTextStatus 
CommonTextSystem::ReturnFontDevice(
    const UsdImagingTextStyle& style, 
    std::shared_ptr<CommonTextTrueTypeFontDevice> fontDevice)
{
    return _imp->ReturnFontDevice(style, fontDevice);
}

const CommonTextGlobalSetting& 
CommonTextSystem::GetTextGlobalSetting() const
{
    return _setting;
}

std::shared_ptr<CommonTextFontMapCache> 
CommonTextSystem::GetFontMapCache()
{
    return _multiLanguageHandler->GetFontMapCache();
}

CommonTextStatus 
CommonTextSystem::AddDefaultFontToFontMapCache()
{
    if (_multiLanguageHandler == nullptr)
        _multiLanguageHandler = std::make_shared<CommonTextMultiLanguageHandler>();

    return _multiLanguageHandler->AddDefaultFontToFontMapCache();
}

std::shared_ptr<CommonTextStringArray> 
CommonTextSystem::GetDefaultTTFontList()
{
    return _multiLanguageHandler->GetDefaultTTFontList();
}

PXR_NAMESPACE_CLOSE_SCOPE
