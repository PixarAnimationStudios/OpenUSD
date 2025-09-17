//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_FUNCTION_H
#define PXR_BASE_ARCH_FUNCTION_H

/// \file arch/function.h
/// Define preprocessor function name macros.
///
/// This file extents the functionality of pxr/base/arch/functionLite.h.
/// This file needs to be public but shouldn't be included directly by
/// anything outside of \c lib/tf.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/functionLite.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Return well formatted function name.
///
/// This function assumes \c function is __ARCH_FUNCTION__ and
/// \c prettyFunction is __ARCH_PRETTY_FUNCTION__, and attempts to
/// reconstruct a well formatted function name.
///
/// \ingroup group_arch_Diagnostic
ARCH_API
std::string ArchGetPrettierFunctionName(const std::string &function,
                                        const std::string &prettyFunction);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_FUNCTION_H 
