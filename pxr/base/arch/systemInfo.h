//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_SYSTEM_INFO_H
#define PXR_BASE_ARCH_SYSTEM_INFO_H

/// \file arch/systemInfo.h
/// \ingroup group_arch_SystemFunctions
/// Provide architecture-specific system information.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \addtogroup group_arch_SystemFunctions
///@{

/// Return current working directory as a string.
ARCH_API
std::string ArchGetCwd();

/// Return the path to the program's executable.
ARCH_API
std::string ArchGetExecutablePath();

/// Return the system's memory page size.  Safe to assume power-of-two.
ARCH_API
int ArchGetPageSize();

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_SYSTEM_INFO_H
