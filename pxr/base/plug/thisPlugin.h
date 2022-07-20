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
#ifndef PXR_BASE_PLUG_THIS_PLUGIN_H
#define PXR_BASE_PLUG_THIS_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_OPEN_SCOPE

/// The name of the current library registered with the plugin system. This
/// may be used to lookup the current library in the plugin registry.
/// This uses the value of the MFB_PACKAGE_NAME macro defined at compile-time
/// unless overridden.
#ifndef PLUG_THIS_PLUGIN_NAME
#define PLUG_THIS_PLUGIN_NAME MFB_PACKAGE_NAME
#endif

/// Returns a plugin registered with the name of the current library (as
/// defined by PLUG_THIS_PLUGIN_NAME). Note that plugin registration occurs as a
/// side effect of using this macro, at the point in time the code at the
/// macro site is invoked.
#define PLUG_THIS_PLUGIN \
    PlugRegistry::GetInstance().GetPluginWithName(\
        TF_PP_STRINGIZE(PLUG_THIS_PLUGIN_NAME))

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_PLUG_THIS_PLUGIN_H
