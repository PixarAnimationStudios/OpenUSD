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
#ifndef ARCH_SYMBOLS_H
#define ARCH_SYMBOLS_H

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
/// and modifies the other arguments: \p objectPath is set to the path to the
/// executable or library the address is found in, \p baseAddress is the
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

#endif // ARCH_SYMBOLS_H
