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
#ifndef PXR_BASE_TF_GETENV_H
#define PXR_BASE_TF_GETENV_H

/// \file tf/getenv.h
/// \ingroup group_tf_SystemsExt
/// Functions for accessing environment variables.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \addtogroup group_tf_SystemsExt
///@{

/// Return an environment variable as a string.
///
/// Return the value of the environment variable \c envName
/// as a string.  If the variable is unset, or is the empty string,
/// then \c defaultValue is returned.
TF_API
std::string TfGetenv(const std::string& envName,
                     const std::string& defaultValue = "");

/// Return an environment variable as an integer.
///
/// Return the value of the environment variable \c envName as an integer.  If
/// the variable is unset, or is the empty string, then \c defaultValue is
/// returned.  Otherwise, the function uses atoi() to convert the string to an
/// integer: the implication being that if the string is not a valid integer,
/// you get back whatever value atoi() comes up with.
TF_API
int TfGetenvInt(const std::string& envName, int defaultValue);

/// Return an environment variable as a boolean.
///
/// Return the value of the environment variable \c envName as a boolean.  If
/// the variable is unset, or is the empty string, then \c defaultValue is
/// returned. A value of \c true is returned if the environment variable is
/// any of "true", "yes", "on" or "1"; the match is not case sensitive. All
/// other values yield a return value of \c false.
TF_API
bool TfGetenvBool(const std::string&, bool defaultValue);

/// Return an environment variable as a double.
///
/// Return the value of the environment variable \c envName as a double.  If
/// the variable is unset, or is the empty string, then \c defaultValue is
/// returned. Otherwise, the function uses TfStringToDouble() to convert the
/// string to a double: the implication being that if the string is not a
/// valid double, you get back whatever value TfStringToDouble() comes up
/// with.
TF_API
double TfGetenvDouble(const std::string& envName, double defaultValue);

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
