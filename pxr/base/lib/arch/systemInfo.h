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
#ifndef ARCH_SYSTEMINFO_H
#define ARCH_SYSTEMINFO_H

/// \file arch/systemInfo.h
/// \ingroup group_arch_SystemFunctions
/// Provide architecture-specific system information.

#include <string>

/// \addtogroup group_arch_SystemFunctions
///@{

/// Return current working directory as a string.
std::string ArchGetCwd();

/// Return user's home directory.
///
/// If \p login is not supplied, the home directory of the current user is
/// returned.  Otherwise, the home directory of the user with the specified
/// login is returned.  If the home directory cannot be determined, the empty
/// string is returned.
std::string ArchGetHomeDirectory(const std::string &login = std::string());

/// Return user name.
///
/// If the user name cannot determined, the empty string is returned.
std::string ArchGetUserName();

/// Return the path to the program's executable.
std::string ArchGetExecutablePath();

///@}

#endif // ARCH_SYSTEMINFO_H
