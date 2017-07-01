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
#ifndef ARCH_ENV_H
#define ARCH_ENV_H

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

///
/// Architecture dependent access to environment variables.
/// \ingroup group_arch_SystemFunctions
/// 

///
/// Returns \c true if and only if the current environment contains \c name.
/// \ingroup group_arch_SystemFunctions
///
ARCH_API 
bool
ArchHasEnv(const std::string &name);

///
/// Gets a value from the current environment identified by \c name.
/// \ingroup group_arch_SystemFunctions
///
ARCH_API 
std::string
ArchGetEnv(const std::string &name);

///
/// Creates or modifies an environment variable.
/// \ingroup group_arch_SystemFunctions
///
ARCH_API
bool
ArchSetEnv(const std::string &name, const std::string &value, bool overwrite);

///
/// Removes an environment variable.
/// \ingroup group_arch_SystemFunctions
///
ARCH_API
bool
ArchRemoveEnv(const std::string &name);

///
/// Expands environment variables in \c str.
/// \ingroup group_arch_SystemFunctions
///
ARCH_API
std::string
ArchExpandEnvironmentVariables(const std::string& str);

///
/// Return an array of the environment variables.
/// \ingroup group_arch_SystemFunctions
///
ARCH_API
char**
ArchEnviron();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // ARCH_ENV_H
