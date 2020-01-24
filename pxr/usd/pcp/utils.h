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
#ifndef PXR_USD_PCP_UTILS_H
#define PXR_USD_PCP_UTILS_H

/// \file pcp/utils.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// Returns an SdfLayer::FileFormatArguments object with the "target" argument
// set to \p target if \p target is not empty.
SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& target);

// Returns an SdfLayer::FileFormatArguments object with the "target" argument
// set to \p target if \p target is not empty and a target is
// not embedded within the given \p identifier.
SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const std::string& target);

// \overload
// Same as above, but modifies \p args instead of returning by value.
void 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const std::string& target,
    SdfLayer::FileFormatArguments* args);

// If a target argument is embedded in \p identifier, copies contents of 
// \p defaultArgs to \p localArgs, removes the "target" argument, and returns
// a const reference to \p localArgs. Otherwise, returns a const reference to
// \p defaultArgs. This lets us avoid making a copy of \p defaultArgs unless 
// needed.
const SdfLayer::FileFormatArguments&
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const SdfLayer::FileFormatArguments* defaultArgs,
    SdfLayer::FileFormatArguments* localArgs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_UTILS_H
