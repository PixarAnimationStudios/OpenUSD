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
#ifndef PLUG_THISPLUGIN_H
#define PLUG_THISPLUGIN_H

#include "pxr/base/plug/registry.h"
#include <boost/preprocessor/stringize.hpp>

/// Returns a plugin registered with the name of the current library (uses the
/// define for MFB_PACKAGE_NAME). Note that plugin registration occurs as a
/// side effect of using this macro, at the point in time the code at the
/// macro site is invoked.
#define PLUG_THIS_PLUGIN \
    PlugRegistry::GetInstance().GetPluginWithName(\
        BOOST_PP_STRINGIZE(MFB_PACKAGE_NAME))

#endif // PLUG_THISPLUGIN_H
