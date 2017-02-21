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
#include "pxr/pxr.h"
#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stringUtils.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USD_SHADE_READ_OLD_ENCODING, true,
    "Set to false to disable support for reading old-style of encoding with "
    "parameters, interface attributes and terminals.");

TF_DEFINE_ENV_SETTING(
    USD_SHADE_WRITE_NEW_ENCODING, false,
    "Set to true to enable the authoring of shading networks using the new "
    "encoding (with inputs and outputs, in place of parameters, interface "
    "attributes and terminals.");

using std::vector;
using std::string;

/* static */
string 
UsdShadeUtils::GetPrefixForAttributeType(UsdShadeAttributeType sourceType)
{
    switch (sourceType) {
        case UsdShadeAttributeType::Input:
            return UsdShadeTokens->inputs.GetString();
        case UsdShadeAttributeType::Output:
            return UsdShadeTokens->outputs.GetString();
        case UsdShadeAttributeType::Parameter: 
            return string();
        case UsdShadeAttributeType::InterfaceAttribute:
            return UsdShadeTokens->interface.GetString();
        default:
            return string();
    }
}

/* static */
std::pair<TfToken, UsdShadeAttributeType> 
UsdShadeUtils::GetBaseNameAndType(const TfToken &fullName)
{
    static const size_t inputsPrefixLen = 
        UsdShadeTokens->inputs.GetString().size();
    static const size_t outputsPrefixLen = 
        UsdShadeTokens->outputs.GetString().size();
    static const size_t interfaceAttrPrefixLen = 
        UsdShadeTokens->interface.GetString().size();

    if (TfStringStartsWith(fullName, UsdShadeTokens->inputs)) {
        TfToken sourceName(fullName.GetString().substr(inputsPrefixLen));
        return std::make_pair(sourceName, UsdShadeAttributeType::Input);
    } else if (TfStringStartsWith(fullName, UsdShadeTokens->outputs)) {
        TfToken sourceName(fullName.GetString().substr(outputsPrefixLen));
        return std::make_pair(sourceName, UsdShadeAttributeType::Output);
    } else if (TfStringStartsWith(fullName, UsdShadeTokens->interface)) {
        TfToken sourceName(fullName.GetString().substr(interfaceAttrPrefixLen));
        return std::make_pair(sourceName, 
                              UsdShadeAttributeType::InterfaceAttribute);
    } else {
        return std::make_pair(fullName, 
                              UsdShadeAttributeType::Parameter);
    }
}

/* static */
TfToken 
UsdShadeUtils::GetFullName(const TfToken &baseName, 
                           const UsdShadeAttributeType type)
{
    return TfToken(UsdShadeUtils::GetPrefixForAttributeType(type) + 
                   baseName.GetString());
}

/* static */
bool 
UsdShadeUtils::ReadOldEncoding()
{
    return TfGetEnvSetting(USD_SHADE_READ_OLD_ENCODING);
}

/* static */
bool 
UsdShadeUtils::WriteNewEncoding()
{
    return TfGetEnvSetting(USD_SHADE_WRITE_NEW_ENCODING);
}

PXR_NAMESPACE_CLOSE_SCOPE

