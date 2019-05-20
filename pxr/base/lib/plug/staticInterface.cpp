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
/// \file staticInterface.cpp

#include "pxr/pxr.h"
#include "pxr/base/plug/staticInterface.h"
#include "pxr/base/plug/interfaceFactory.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/scoped.h"
#include <mutex>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

static std::mutex _initializationMutex;

void
Plug_StaticInterfaceBase::_LoadAndInstantiate(const std::type_info& type) const
{
    // Double checked locking.
    std::lock_guard<std::mutex> lock(_initializationMutex);
    if (_initialized) {
        // Someone beat us to the initialization.
        return;
    }

    // We attempt initialization only once so set _initialized when we return
    // even if we fail to load or instantiate.  We must not set it before we
    // return because other threads would be able to observe partial
    // initialization.
    TfScoped<> initializeOnReturn{[this]() { _initialized = true; }};

    // Validate type.
    // We use FindByName because Find requres that std::type_info has been
    // regisered, but that won't happen until the plugin is loaded.
    const TfType &tfType =
        TfType::FindByName(TfType::GetCanonicalTypeName(type));
    if (!tfType) {
        TF_CODING_ERROR("Failed to load plugin interface: "
                        "Can't find type %s", type.name());
        return;
    }
    if (tfType.IsRoot()) {
        TF_CODING_ERROR("Failed to load plugin interface: "
                        "Can't manufacture type %s",
                        tfType.GetTypeName().c_str());
        return;
    }

    // Get the plugin with type.
    PlugPluginPtr plugin = PlugRegistry::GetInstance().GetPluginForType(tfType);
    if (!plugin) {
        TF_RUNTIME_ERROR("Failed to load plugin interface: "
                         "Can't find plugin that defines type %s",
                         tfType.GetTypeName().c_str());
        return;
    }

    // Load the plugin.
    if (!plugin->Load()) {
        // Error already reported.
        return;
    }

    // Manufacture the type.
    Plug_InterfaceFactory::Base* factory =
        tfType.GetFactory<Plug_InterfaceFactory::Base>();
    if (!factory) {
        TF_CODING_ERROR("Failed to load plugin interface: "
                        "No default constructor for type %s",
                        tfType.GetTypeName().c_str());
        return;
    }
    _ptr = factory->New();

    // Report on error.
    if (!_ptr) {
        TF_CODING_ERROR("Failed to load plugin interface: "
                        "Plugin didn't manufacture an instance of %s",
                        tfType.GetTypeName().c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
