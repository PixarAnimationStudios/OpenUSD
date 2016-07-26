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
#include "pxr/usd/usdShade/pShaderUtils.h"
#include <vector>
#include <string>

using std::vector;
using std::string;

/// \hideinitializer
#define USD_USAGE_VALS                               \
        ((Attribute,        "attribute"))            \
        ((CoshaderInstance, "coshaderInstance"))     \
        ((Parameter,        "parameter"))            \
        ((Binding,          "binding"))

// XXX
// We have introduced one change from the current Presto shading OM:
// 1) We are holding off on adding "rib" until we really need it; it does not
// yet appear in any src or inst menva file on cake.
//

/// \anchor UsdUsageVals
/// \brief <b>UsdUsageVals</b> provides the core values for the "Usage"
/// metadata that specifies how an Attribute or Relationship is meant to be
/// used by clients. It is implicit that no attribute can have more than one
/// "use"
///
/// Values include UsdUsageVals->XX for XX in:
/// \li <b>Attribute</b> : use as an "inherited attribute", inspired by 
///                        RiAttribute.  Note relationships can also serve as
///                        "Attribute" with the meaning that the targetted
///                        prim defines a shader to be emitted.
/// \li <b>CoshaderInstance</b> : the attribute's value names a coshader asset.
///                               Optional "coshaderHandle" metadatum on
///                               the attribute specifies the name by
///                               which other shaders can refer to this
///                               specific coshader instance as a parameter.
/// \li <b>Parameter</b> : the attribute or relationship should serve as a
///                        parameter to the shader or procedural represented by
///                        the prim on which the attribute is defined.
/// \li <b>Binding</b> :       a relationship should be consumed as a
///                            shader binding.  The targetted shader(s) can be
///                            resolved via 
///                            UsdRelationship::ComposeForwardedTargets

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (surface)
    (attribute)
    (parameter)
    (rib)
    (coshaderInstance)
    (coshaderHandle)
    (binding)
    (isCoshaderArray)
    (displayColor)
    (displayOpacity)
    (riName)
    (riType)
);

////////////////////////////////////////////////////////////////////////

template <typename TYPE>
static bool
_GetMetadataAs(const UsdObject &obj, const std::string &key, TYPE *result)
{
    VtValue val;
    if (obj.GetMetadata(TfToken(key), &val)) {
        if (val.CanCast<TYPE>()) {
            val = val.Cast<TYPE>();
            *result = val.Get<TYPE>();
            return true;
        }
    }
    return false;
}

static std::string
_GetRiType(const UsdObject &obj)
{
    std::string riType;
    _GetMetadataAs(obj, _tokens->riType, &riType);
    return riType;
}

static std::string
_GetRiName(const UsdObject &obj)
{
    std::string riName;
    if (_GetMetadataAs(obj, _tokens->riName, &riName)) {
        return riName;
    }
    return obj.GetPath().GetName();
}

static string
_GetPath(const UsdObject &obj)
{
    return obj.GetPath().GetString();
}

////////////////////////////////////////////////////////////////////////

bool UsdShdIsShaderBinding(const UsdRelationship &rel)
{
    if (rel.GetPath().GetName() == _tokens->surface) {
        return true;
    }

    std::string riType = _GetRiType(rel);
    return (riType == _tokens->binding or riType == _tokens->attribute);
}

std::string UsdShdGetSloArgName(const UsdAttribute &prop)
{
    std::string riType = _GetRiType(prop);
    if (riType == _tokens->parameter or
        riType == _tokens->coshaderInstance) {
        return _GetRiName(prop);
    }
    return std::string();
}

std::string UsdShdGetSloArgName(const UsdRelationship &prop)
{
    std::string riType = _GetRiType(prop);
    if (riType == _tokens->parameter or
        riType == _tokens->coshaderInstance) {
        return _GetRiName(prop);
    }
    return std::string();
}

std::string UsdShdGetRibAttributeName(const UsdAttribute &prop)
{
    if (_GetRiType(prop) == _tokens->attribute) {
        return _GetRiName(prop);
    }
    return std::string();
}

std::string UsdShdGetRibAttributeName(const UsdRelationship &prop)
{
    if (_GetRiType(prop) == _tokens->attribute) {
        return _GetRiName(prop);
    }
    return std::string();
}

std::string UsdShdGetCoshaderHandle(const UsdAttribute &prop)
{
    std::string coshaderHandle;
    _GetMetadataAs(prop, _tokens->coshaderHandle, &coshaderHandle);
    return coshaderHandle;
}

std::string UsdShdGetCoshaderHandle(const UsdRelationship &prop)
{
    std::string coshaderHandle;
    _GetMetadataAs(prop, _tokens->coshaderHandle, &coshaderHandle);
    return coshaderHandle;
}

bool UsdShdIsCoshaderInstance(const UsdAttribute &prop)
{
    return (_GetRiType(prop) == _tokens->coshaderInstance);
}

bool UsdShdIsCoshaderInstance(const UsdRelationship &prop)
{
    return (_GetRiType(prop) == _tokens->coshaderInstance);
}

bool UsdShdIsExplicitRib(const UsdAttribute &prop)
{
    return (_GetRiType(prop) == _tokens->rib);
}

bool UsdShdIsExplicitRib(const UsdRelationship &prop)
{
    return (_GetRiType(prop) == _tokens->rib);
}

bool UsdShdIsCoshaderArray(const UsdAttribute &prop)
{
    bool isCoshaderArray = false;
    _GetMetadataAs(prop, _tokens->isCoshaderArray, &isCoshaderArray);
    return isCoshaderArray;
}

bool UsdShdIsCoshaderArray(const UsdRelationship &prop)
{
    bool isCoshaderArray = false;
    _GetMetadataAs(prop, _tokens->isCoshaderArray, &isCoshaderArray);
    return isCoshaderArray;
}

string ShdGetPath(const UsdPrim &obj)
{
    return obj.GetPath().GetString();
}

string ShdGetShaderHandle(const UsdPrim &obj)
{
    std::string riName = _GetRiName(obj);
    return not riName.empty() ? riName : _GetPath(obj);
}

//////////////////////////////////////////////////////////////////////////////

bool UsdShdSplitRibAttributeName(const std::string &name,
                                 std::string *attrName, std::string *argName)
{
    size_t i = name.find(':');
    if (i == name.npos) {

        // If we don't find ':', fall back to '_'
        i = name.find('_');
        if (i == name.npos) {
            return false;
        }
    }
    
    *attrName = name.substr(0,i);
    *argName = name.substr(i+1);
    return true;
}
