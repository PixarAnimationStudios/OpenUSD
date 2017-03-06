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
#include "pxr/usd/usdShade/output.h"

#include "pxr/usd/usdShade/utils.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdShade/connectableAPI.h"

#include <stdlib.h>
#include <algorithm>

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE


using std::vector;
using std::string;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderType)
);

UsdShadeOutput::UsdShadeOutput(const UsdAttribute &attr)
    : _prop(attr)
{
}

UsdShadeOutput::UsdShadeOutput(const UsdRelationship &rel)
    : _prop(rel)
{
}

UsdShadeOutput::UsdShadeOutput(const UsdProperty &prop)
    : _prop(prop)
{
}

TfToken 
UsdShadeOutput::GetBaseName() const
{
    string name = GetFullName();
    if (TfStringStartsWith(name, UsdShadeTokens->outputs)) {
        return TfToken(name.substr(UsdShadeTokens->outputs.GetString().size()));
    }
    return GetFullName();
}

SdfValueTypeName 
UsdShadeOutput::GetTypeName() const
{ 
    if (UsdAttribute attr = GetAttr()) {
        return  attr.GetTypeName();
    }

    // Fallback to token for outputs that represent terninals.
    return SdfValueTypeNames->Token;
}

static TfToken
_GetOutputAttrName(const TfToken outputName) 
{
    return TfToken(UsdShadeTokens->outputs.GetString() + outputName.GetString());
}

UsdShadeOutput::UsdShadeOutput(
    UsdPrim prim,
    TfToken const &name,
    SdfValueTypeName const &typeName)
{
    // XXX what do we do if the type name doesn't match and it exists already?
    TfToken attrName = _GetOutputAttrName(name);
    _prop = prim.GetAttribute(attrName);
    if (!_prop) {
        _prop = prim.CreateAttribute(attrName, typeName, /* custom = */ false);
    }
}

bool
UsdShadeOutput::Set(const VtValue& value,
                    UsdTimeCode time) const
{
    if (UsdAttribute attr = GetAttr()) {
        return attr.Set(value, time);
    }
    return false;
}

bool 
UsdShadeOutput::SetRenderType(
        TfToken const& renderType) const
{
    return _prop.SetMetadata(_tokens->renderType, renderType);
}

TfToken 
UsdShadeOutput::GetRenderType() const
{
    TfToken renderType;
    _prop.GetMetadata(_tokens->renderType, &renderType);
    return renderType;
}

bool 
UsdShadeOutput::HasRenderType() const
{
    return _prop.HasMetadata(_tokens->renderType);
}

/* static */
bool 
UsdShadeOutput::IsOutput(const UsdAttribute &attr)
{
    return TfStringStartsWith(attr.GetName().GetString(), 
                              UsdShadeTokens->outputs);
}

PXR_NAMESPACE_CLOSE_SCOPE

