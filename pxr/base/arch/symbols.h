//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_SYMBOLS_H
#define PXR_BASE_ARCH_SYMBOLS_H

/// \file arch/symbols.h
/// \ingroup group_arch_Diagnostics
/// Architecture-specific symbol lookup routines.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Returns information about the address \p address in the running program.
///
/// Returns \c false if no information can be found, otherwise returns \c true
/// and modifies the other arguments: \p objectPath is set to the absolute path 
/// to the executable or library the address is found in, \p baseAddress is the
/// address where that object is loaded, \p symbolName is the symbolic name of
/// the thing containing the address, and \p symbolAddress is the starting
/// address of that thing.  If no thing is found to contain the address then
/// \p symbolName is cleared and \p symbolAddress is set to \c NULL. Any of
/// the arguments except \p address can be \c NULL if the result isn't needed.
/// This will return \c false if \c NULL is passed to \p address.
///
/// \ingroup group_arch_Diagnostics
ARCH_API
bool ArchGetAddressInfo(void* address,
                        std::string* objectPath, void** baseAddress,
                        std::string* symbolName, void** symbolAddress);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_SYMBOLS_H
