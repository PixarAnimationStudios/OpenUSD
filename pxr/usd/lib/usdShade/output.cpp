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

static UsdRelationship
_GetOutputConnectionRel(const UsdShadeOutput &output, bool create)
{
    const UsdAttribute & attr = output.GetAttr();
    const UsdPrim& prim = attr.GetPrim();
    const TfToken& relName = output.GetConnectionRelName();
    if (UsdRelationship rel = prim.GetRelationship(relName)) {
        return rel;
    }

    if (create) {
        return prim.CreateRelationship(relName, /* custom = */ false);
    }
    else {
        return UsdRelationship();
    }
}

UsdShadeOutput::UsdShadeOutput(const UsdAttribute &attr)
    : _attr(attr)
{
}

TfToken 
UsdShadeOutput::GetOutputName() const
{
    string name = GetName();
    return TfToken(name.substr(UsdShadeTokens->outputs.GetString().size()));
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
    _attr = prim.GetAttribute(attrName);
    if (!_attr) {
        _attr = prim.CreateAttribute(attrName, typeName, /* custom = */ false);
    }
}

bool
UsdShadeOutput::Set(const VtValue& value,
                    UsdTimeCode time) const
{
    return _attr.Set(value, time);
}

bool 
UsdShadeOutput::SetRenderType(
        TfToken const& renderType) const
{
    return _attr.SetMetadata(_tokens->renderType, renderType);
}

TfToken 
UsdShadeOutput::GetRenderType() const
{
    TfToken renderType;
    _attr.GetMetadata(_tokens->renderType, &renderType);
    return renderType;
}

bool 
UsdShadeOutput::HasRenderType() const
{
    return _attr.HasMetadata(_tokens->renderType);
}


bool 
UsdShadeOutput::ConnectToSource(
        UsdShadeConnectableAPI const &source, 
        TfToken const &outputName,
        bool outputIsParameter) const
{
    // Only outputs belonging to subgraphs are connectable. We don't allow 
    // connecting outputs of shaders as it's not meaningful.
    // 
    // Note: this warning will not be issued if the prim is untyped.
    if (UsdShadeConnectableAPI(GetAttr().GetPrim()).IsShader()) {
        TF_WARN("Attempted to connect an output of a shader <%s> to <%s>.",
            GetAttr().GetPath().GetText(), 
            source.GetPath().AppendProperty(outputName).GetText());
        return false;
    }

    // Ensure that the source prim is a descendent of the subgraph owning 
    // the output.
    const SdfPath sourcePrimPath = source.GetPrim().GetPath();
    const SdfPath outputOwnerPath = GetAttr().GetPrim().GetPath();
    if (not sourcePrimPath.HasPrefix(outputOwnerPath)) {
        TF_WARN("Source of output '%s' on subgraph at path <%s> is outside the "
            "subgraph: <%s>", outputName.GetText(), outputOwnerPath.GetText(), 
            sourcePrimPath.GetText());
        // XXX: Should we disallow this or simply continue?
    }

    if (UsdRelationship rel = _GetOutputConnectionRel(*this, true)) {
        return UsdShadeConnectableAPI::MakeConnection(rel, source, outputName,
            GetTypeName(), outputIsParameter);
    } 

    return false;
}
    
bool 
UsdShadeOutput::ConnectToSource(UsdShadeOutput const &output) const
{
    UsdShadeConnectableAPI source(output.GetAttr().GetPrim());
    return ConnectToSource(source, output.GetOutputName(), 
                           /* outputIsParameter */ false);
}

bool 
UsdShadeOutput::ConnectToSource(UsdShadeParameter const &param) const
{
    UsdShadeConnectableAPI source(param.GetAttr().GetPrim());
    return ConnectToSource(source, param.GetName(),
                           /* outputIsParameter */ true);
}

bool 
UsdShadeOutput::DisconnectSource() const
{
    bool success = true;
    if (UsdRelationship rel = _GetOutputConnectionRel(*this, false)) {
        success = rel.BlockTargets();
    }
    return success;
}

bool
UsdShadeOutput::ClearSource() const
{
    bool success = true;
    if (UsdRelationship rel = _GetOutputConnectionRel(*this, false)) {
        success = rel.ClearTargets(/* removeSpec = */ true);
    }
    return success;
}

bool 
UsdShadeOutput::GetConnectedSource(
        UsdShadeConnectableAPI *source, 
        TfToken *outputName) const
{
    if (!(source && outputName)){
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output parameters");
        return false;
    }
    if (UsdRelationship rel = _GetOutputConnectionRel(*this, false)) {
        return UsdShadeConnectableAPI::EvaluateConnection(rel, 
            source, outputName);
    }
    else {
        *source = UsdShadeConnectableAPI();
        return false;
    }
}

bool 
UsdShadeOutput::IsConnected() const
{
    /// This MUST have the same semantics as GetConnectedSource(s).
    /// XXX someday we might make this more efficient through careful
    /// refactoring, but safest to just call the exact same code.
    UsdShadeConnectableAPI source;
    TfToken        outputName;
    return GetConnectedSource(&source, &outputName);
}

TfToken 
UsdShadeOutput::GetConnectionRelName() const
{
    return TfToken(UsdShadeTokens->connectedSourceFor.GetString() + 
           _attr.GetName().GetString());
}

/* static */
bool 
UsdShadeOutput::IsOutput(const UsdAttribute &attr)
{
    return TfStringStartsWith(attr.GetName().GetString(), 
                              UsdShadeTokens->outputs);
}
