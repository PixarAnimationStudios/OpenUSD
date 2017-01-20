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
#include "pxr/usd/usdShade/output.h"

#include "pxr/usd/usdShade/parameter.h"
#include "pxr/usd/usdShade/interfaceAttribute.h"
#include "pxr/usd/usdShade/utils.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/usdShade/connectableAPI.h"

#include <stdlib.h>
#include <algorithm>

#include "pxr/base/tf/envSetting.h"

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

static 
TfToken
_GetPropertyName(const TfToken &sourceName, 
                 const UsdShadeAttributeType sourceType)
{
    return TfToken(UsdShadeUtilsGetPrefixForAttributeType(sourceType) + 
                   sourceName.GetString());
}

bool 
UsdShadeOutput::ConnectToSource(
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName,
        UsdShadeAttributeType const sourceType) const
{
    // Only outputs belonging to subgraphs are connectable. We don't allow 
    // connecting outputs of shaders as it's not meaningful.
    // 
    // Note: this warning will not be issued if the prim is untyped or if the 
    // type is unknown.
    if (UsdShadeConnectableAPI(GetAttr().GetPrim()).IsShader()) {
        TF_WARN("Attempted to connect an output of a shader <%s> to <%s>.",
            GetAttr().GetPath().GetText(), 
            source.GetPath().AppendProperty(
                _GetPropertyName(sourceName, sourceType)).GetText());
        return false;
    }

    // Ensure that the source prim is a descendent of the subgraph owning 
    // the output.
    const SdfPath sourcePrimPath = source.GetPrim().GetPath();
    const SdfPath outputOwnerPath = GetAttr().GetPrim().GetPath();
    if (not sourcePrimPath.HasPrefix(outputOwnerPath)) {
        TF_WARN("Source of output '%s' on subgraph at path <%s> is outside the "
            "subgraph: <%s>", sourceName.GetText(), outputOwnerPath.GetText(), 
            sourcePrimPath.GetText());
        // XXX: Should we disallow this or simply continue?
    }

    return UsdShadeConnectableAPI::MakeConnection(GetProperty(), source, 
            sourceName, GetTypeName(), sourceType);
}

bool 
UsdShadeOutput::ConnectToSource(const SdfPath &sourcePath) const
{
    // sourcePath needs to be a property path for us to make a connection.
    if (not sourcePath.IsPropertyPath())
        return false;

    UsdPrim sourcePrim = GetAttr().GetStage()->GetPrimAtPath(
        sourcePath.GetPrimPath());
    UsdShadeConnectableAPI source(sourcePrim);
    // We don't validate UsdShadeConnectableAPI the type of the source prim 
    // may be unknown. (i.e. it could be a pure over or a typeless def).

    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    std::tie(sourceName, sourceType) = UsdShadeUtilsGetBaseNameAndType(
        sourcePath.GetNameToken());

    return ConnectToSource(source, sourceName, sourceType);
}

bool 
UsdShadeOutput::ConnectToSource(UsdShadeOutput const &output) const
{
    UsdShadeConnectableAPI source(output.GetAttr().GetPrim());
    return ConnectToSource(source, output.GetBaseName(), 
        /* sourceType */ UsdShadeAttributeType::Output);
}

bool 
UsdShadeOutput::ConnectToSource(UsdShadeParameter const &param) const
{
    UsdShadeConnectableAPI source(param.GetAttr().GetPrim());
    return ConnectToSource(source, param.GetName(),
        /* sourceType */ UsdShadeAttributeType::Parameter);
}

bool 
UsdShadeOutput::DisconnectSource() const
{
    bool success = true;
    if (UsdRelationship rel = UsdShadeConnectableAPI::GetConnectionRel(
            GetAttr(), false)) {
        success = rel.BlockTargets();
    }
    return success;
}

bool
UsdShadeOutput::ClearSource() const
{
    bool success = true;
    if (UsdRelationship rel = UsdShadeConnectableAPI::GetConnectionRel(
            GetAttr(), false)) {
        success = rel.ClearTargets(/* removeSpec = */ true);
    }
    return success;
}

bool 
UsdShadeOutput::GetConnectedSource(
        UsdShadeConnectableAPI *source, 
        TfToken *sourceName,
        UsdShadeAttributeType *sourceType) const
{
    if (!(source && sourceName)){
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output parameters");
        return false;
    }

    return UsdShadeConnectableAPI::EvaluateConnection(GetProperty(), 
            source, sourceName, sourceType);
}

bool 
UsdShadeOutput::IsConnected() const
{
    /// This MUST have the same semantics as GetConnectedSource(s).
    /// XXX someday we might make this more efficient through careful
    /// refactoring, but safest to just call the exact same code.
    UsdShadeConnectableAPI source;
    TfToken        sourceName;
    UsdShadeAttributeType sourceType;
    return GetConnectedSource(&source, &sourceName, &sourceType);
}

TfToken 
UsdShadeOutput::GetConnectionRelName() const
{
    return TfToken(UsdShadeTokens->connectedSourceFor.GetString() + 
           _prop.GetName().GetString());
}

/* static */
bool 
UsdShadeOutput::IsOutput(const UsdAttribute &attr)
{
    return TfStringStartsWith(attr.GetName().GetString(), 
                              UsdShadeTokens->outputs);
}
