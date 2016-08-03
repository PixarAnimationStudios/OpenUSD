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
///
/// \file Info.h

#ifndef GLF_INFO_H
#define GLF_INFO_H

#include "pxr/imaging/glf/api.h"
#include <string>

/// \brief Tests for GL extension support.
///
/// Returns \c true if each extension name listed in \a extensions
/// is supported by the current GL context.
GLF_API
bool GlfHasExtensions(std::string const & extensions);

/// \brief Tests whether the current GL context only has
/// minimal graphics support.  This is true for NX clients at least.
GLF_API
bool GlfHasLegacyGraphics();

#endif
