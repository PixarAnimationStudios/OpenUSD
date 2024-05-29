//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_ENV_H
#define PXR_BASE_ARCH_ENV_H

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

#endif // PXR_BASE_ARCH_ENV_H
