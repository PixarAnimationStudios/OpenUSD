//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SETENV_H
#define PXR_BASE_TF_SETENV_H

/// \file tf/setenv.h
/// \ingroup group_tf_SystemsExt
/// Functions for setting and unsetting environment variables

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
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
TF_API
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
TF_API
bool TfUnsetenv(const std::string& envName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SETENV_H
