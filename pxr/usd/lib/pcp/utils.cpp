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
///
/// \file Pcp/Utils.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/utils.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"

PXR_NAMESPACE_OPEN_SCOPE

static bool
_TargetIsSpecifiedInIdentifier(
    const std::string& identifier)
{
    std::string layerPath;
    SdfLayer::FileFormatArguments layerArgs;
    return SdfLayer::SplitIdentifier(identifier, &layerPath, &layerArgs)
        && layerArgs.find(SdfFileFormatTokens->TargetArg) != layerArgs.end();
}

SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForTargetSchema(
    const std::string& identifier,
    const std::string& targetSchema)
{
    SdfLayer::FileFormatArguments args;
    Pcp_GetArgumentsForTargetSchema(identifier, targetSchema, &args);
    return args;
}

void 
Pcp_GetArgumentsForTargetSchema(
    const std::string& identifier,
    const std::string& targetSchema,
    SdfLayer::FileFormatArguments* args)
{
    if (!targetSchema.empty() && !_TargetIsSpecifiedInIdentifier(identifier)) {
        (*args)[SdfFileFormatTokens->TargetArg] = targetSchema;
    }
}

SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForTargetSchema(
    const std::string& targetSchema)
{
    if (targetSchema.empty()) {
        return {};
    }
    return {{ SdfFileFormatTokens->TargetArg, targetSchema }};
}

const SdfLayer::FileFormatArguments&
Pcp_GetArgumentsForTargetSchema(
    const std::string& identifier,
    const SdfLayer::FileFormatArguments* defaultArgs,
    SdfLayer::FileFormatArguments* localArgs)
{
    if (!_TargetIsSpecifiedInIdentifier(identifier)) {
        return *defaultArgs;
    }

    *localArgs = *defaultArgs;
    localArgs->erase(SdfFileFormatTokens->TargetArg);
    return *localArgs;
}

PXR_NAMESPACE_CLOSE_SCOPE
