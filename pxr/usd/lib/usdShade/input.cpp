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
#include "pxr/usd/usdShade/input.h"

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

UsdShadeInput::UsdShadeInput(const UsdAttribute &attr)
    : _attr(attr)
{
}

TfToken 
UsdShadeInput::GetBaseName() const
{
    string name = GetFullName();
    if (TfStringStartsWith(name, UsdShadeTokens->inputs)) {
        return TfToken(name.substr(UsdShadeTokens->inputs.GetString().size()));
    }
    return GetFullName();
}

SdfValueTypeName 
UsdShadeInput::GetTypeName() const
{ 
    return _attr.GetTypeName();
}

static TfToken
_GetInputAttrName(const TfToken inputName) 
{
    return TfToken(UsdShadeTokens->inputs.GetString() + inputName.GetString());
}

UsdShadeInput::UsdShadeInput(
    UsdPrim prim,
    TfToken const &name,
    SdfValueTypeName const &typeName)
{
    // XXX what do we do if the type name doesn't match and it exists already?
    TfToken inputAttrName = _GetInputAttrName(name);
    if (prim.HasAttribute(inputAttrName)) {
        _attr = prim.GetAttribute(inputAttrName);
    }

    if (UsdShadeUtils::ReadOldEncoding()) {
        if (prim.HasAttribute(name)) {
            _attr = prim.GetAttribute(name);
        }
        else {
            TfToken interfaceAttrName(UsdShadeTokens->interface_.GetString() + 
                                      name.GetString());
            if (prim.HasAttribute(interfaceAttrName)) {
                _attr = prim.GetAttribute(interfaceAttrName);
            }
        }
    }

    if (!_attr) {
        if (UsdShadeUtils::WriteNewEncoding()) {
            _attr = prim.CreateAttribute(inputAttrName, typeName, 
                /* custom = */ false);
        } else {
            UsdShadeConnectableAPI connectable(prim);
            // If this is a node-graph and the name already contains "interface:" 
            // namespace in it, just create the attribute with the requested 
            // name.
            if (connectable.IsNodeGraph() and 
                TfStringStartsWith(name.GetString(),UsdShadeTokens->interface_))
            {
                _attr = prim.CreateAttribute(name, typeName, /*custom*/ false);
            } else {
                // fallback to creating an old style UsdShadeParameter.
                _attr = prim.CreateAttribute(name, typeName, /*custom*/ false);
            }
        }
    }
}

bool
UsdShadeInput::Set(const VtValue& value, UsdTimeCode time) const
{
    return _attr.Set(value, time);
}

bool 
UsdShadeInput::SetRenderType(TfToken const& renderType) const
{
    return _attr.SetMetadata(_tokens->renderType, renderType);
}

TfToken 
UsdShadeInput::GetRenderType() const
{
    TfToken renderType;
    _attr.GetMetadata(_tokens->renderType, &renderType);
    return renderType;
}

bool 
UsdShadeInput::HasRenderType() const
{
    return _attr.HasMetadata(_tokens->renderType);
}

/* static */
bool 
UsdShadeInput::IsInput(const UsdAttribute &attr)
{
    return attr and attr.IsDefined() and 
            // Check the attribute's namespace only if reading of old encoding 
            // is not supported.
            (UsdShadeUtils::ReadOldEncoding() ? true :
             TfStringStartsWith(attr.GetName().GetString(), 
                                UsdShadeTokens->inputs));
}

PXR_NAMESPACE_CLOSE_SCOPE

