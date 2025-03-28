//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
