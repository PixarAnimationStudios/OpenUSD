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
#include "pxr/usd/usdShade/parameter.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/interfaceAttribute.h"
#include "pxr/usd/usdShade/utils.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/relationship.h"


#include <stdlib.h>
#include <algorithm>

#include "pxr/base/tf/envSetting.h"

#include "debugCodes.h"

using std::vector;
using std::string;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderType)
    (outputName)
);

struct _PropertyLessThan {
    bool operator()(UsdProperty const &p1, UsdProperty const &p2){
        return TfDictionaryLessThan()(p1.GetName(), p2.GetName());
    }
};

UsdShadeParameter::UsdShadeParameter(
        const UsdAttribute &attr)
    :
        _attr(attr)
{
}

UsdShadeParameter::UsdShadeParameter(
        UsdPrim prim,
        TfToken const &name,
        SdfValueTypeName const &typeName)
{
    // XXX what do we do if the type name doesn't match and it exists already?

    _attr = prim.GetAttribute(name);
    if (!_attr) {
        _attr = prim.CreateAttribute(name, typeName, /* custom = */ false);
    }
}

bool 
UsdShadeParameter::SetRenderType(
        TfToken const& renderType) const
{
    return _attr.SetMetadata(_tokens->renderType, renderType);
}

TfToken 
UsdShadeParameter::GetRenderType() const
{
    TfToken renderType;
    _attr.GetMetadata(_tokens->renderType, &renderType);
    return renderType;
}

bool 
UsdShadeParameter::HasRenderType() const
{
    return _attr.HasMetadata(_tokens->renderType);
}

bool 
UsdShadeParameter::ConnectToSource(
        UsdShadeConnectableAPI const &source, 
        TfToken const &outputName,
        UsdShadeAttributeType const sourceType) const
{
    UsdRelationship rel = UsdShadeConnectableAPI::GetConnectionRel(GetAttr(), 
                                                                   true);

    return UsdShadeConnectableAPI::MakeConnection(rel, source, outputName,
                                                  _attr.GetTypeName(),
                                                  sourceType);
}

bool 
UsdShadeParameter::ConnectToSource(const SdfPath &sourcePath) const
{
    // sourcePath needs to be a property path for us to make a connection.
    if (not sourcePath.IsPropertyPath())
        return false;

    UsdPrim sourcePrim = GetAttr().GetStage()->GetPrimAtPath(
        sourcePath.GetPrimPath());
    UsdShadeConnectableAPI source(sourcePrim);

    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    std::tie(sourceName, sourceType) = UsdShadeUtilsGetBaseNameAndType(
        sourcePath.GetNameToken());

    return ConnectToSource(source, sourceName, sourceType);
}

bool 
UsdShadeParameter::ConnectToSource(UsdShadeOutput const &output) const
{
    UsdShadeConnectableAPI source(output.GetAttr().GetPrim());
    return ConnectToSource(source, output.GetBaseName(), 
                           /* sourceType */ UsdShadeAttributeType::Output);
}

bool 
UsdShadeParameter::ConnectToSource(UsdShadeParameter const &param) const
{
    UsdShadeConnectableAPI source(param.GetAttr().GetPrim());
    return ConnectToSource(source, param.GetName(),
        /* sourceType */ UsdShadeAttributeType::Parameter);
}

bool 
UsdShadeParameter::ConnectToSource(
    UsdShadeInterfaceAttribute const &interfaceAttribute) const
{
    UsdShadeConnectableAPI source(interfaceAttribute.GetAttr().GetPrim());
    // interfaceAttribute.GetName() returns the un-namespaced interface 
    // attribute name.
    return ConnectToSource(source, interfaceAttribute.GetName(),
        /* sourceType */ UsdShadeAttributeType::InterfaceAttribute);
}
    
bool 
UsdShadeParameter::DisconnectSource() const
{
    bool success = true;
    if (UsdRelationship rel = UsdShadeConnectableAPI::GetConnectionRel(
        GetAttr(), false)) {
        success = rel.BlockTargets();
    }
    return success;
}

bool
UsdShadeParameter::ClearSource() const
{
    bool success = true;
    if (UsdRelationship rel = UsdShadeConnectableAPI::GetConnectionRel(
        GetAttr(), false)) {
        success = rel.ClearTargets(/* removeSpec = */ true);
    }
    return success;
}

bool 
UsdShadeParameter::GetConnectedSource(
        UsdShadeConnectableAPI *source, 
        TfToken *sourceName,
        UsdShadeAttributeType *sourceType) const
{
    if (!(source && sourceName)){
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output parameters");
        return false;
    }
    
    return UsdShadeConnectableAPI::EvaluateConnection(GetAttr(), source, 
            sourceName, sourceType);
}

bool 
UsdShadeParameter::IsConnected() const
{
    /// This MUST have the same semantics as GetConnectedSource(s).
    /// XXX someday we might make this more efficient through careful
    /// refactoring, but safest to just call the exact same code.
    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    return GetConnectedSource(&source, &sourceName, &sourceType);
}

TfToken 
UsdShadeParameter::GetConnectionRelName() const
{
    return TfToken(UsdShadeTokens->connectedSourceFor.GetString() +
           _attr.GetName().GetString());
}
