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
#ifndef TF_SETENV_H
#define TF_SETENV_H

/// \file tf/setenv.h
/// \ingroup group_tf_SystemsExt
/// Functions for setting and unsetting environment variables

#include "pxr/pxr.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Set an environment variable.
///
/// Sets an environment variable and ensures that it appears in the Python
/// environment if Python is initialized. If Python has not yet been
/// initialized, the variable \p envName is set to \p value in the environment
/// using \c setenv. Otherwise, it is set both in the environment and in
/// Python using \c TfPySetenv. The new value overwrites any existing value.
///
/// If the value cannot be set, false is returned and a warning is posted.
/// Otherwise, the return value is true.
///
/// \ingroup group_tf_SystemsExt
bool TfSetenv(const std::string& envName, const std::string& value);

/// Unset an environment variable.
///
/// Unsets an environment variable and ensures that it is also removed from
/// the Python environment if Python is initialized. If Python has not yet
/// been initialized, the variable \p envName is unset in the environment
/// using \c unsetenv. Otherwise, it is unset both in the environment and in
/// Python using \c TfPyUnsetenv.
/// 
/// If the value cannot be unset, false is returned and a warning is posted.
/// Otherwise, the return value is true.
///
/// \ingroup group_tf_SystemsExt
bool TfUnsetenv(const std::string& envName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_SETENV_H
