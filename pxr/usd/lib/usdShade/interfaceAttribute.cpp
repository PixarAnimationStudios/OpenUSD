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
#include "pxr/usd/usdShade/interfaceAttribute.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/parameter.h"
#include "pxr/usd/usdShade/output.h"

#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_OPEN_SCOPE


static std::string
_GetRelPrefix(const TfToken& renderTarget)
{
    return renderTarget.IsEmpty() ? UsdShadeTokens->interfaceRecipientsOf:
            TfStringPrintf("%s:%s", renderTarget.GetText(),
                UsdShadeTokens->interfaceRecipientsOf.GetText());
}

/* static */ std::string
UsdShadeInterfaceAttribute::_GetInterfaceAttributeRelPrefix(
        const TfToken& renderTarget)
{
    return _GetRelPrefix(renderTarget);
}

UsdShadeInterfaceAttribute::UsdShadeInterfaceAttribute(
        const UsdAttribute &attr)
{
    TfToken const &interfaceAttrName = attr.GetName();
    if (TfStringStartsWith(interfaceAttrName, UsdShadeTokens->interface_)){
        _attr = attr;
        _name = TfToken(interfaceAttrName.GetString().substr(
            UsdShadeTokens->interface_.GetString().size()));
    }
}

/* static */ TfToken
UsdShadeInterfaceAttribute::_GetName(
        const TfToken& interfaceAttrName)
{
    if (TfStringStartsWith(interfaceAttrName, UsdShadeTokens->interface_)) {
        return interfaceAttrName;
    }
    
    return TfToken(TfStringPrintf("%s%s",
            UsdShadeTokens->interface_.GetText(),
            interfaceAttrName.GetText()));
}

static TfToken 
_GetInterfaceAttributeRelName(
        const TfToken& renderTarget,
        const UsdShadeInterfaceAttribute& interfaceAttr)
{
    return TfToken(
            _GetRelPrefix(renderTarget) 
            + interfaceAttr.GetName().GetString());
}

UsdShadeInterfaceAttribute::UsdShadeInterfaceAttribute(
        const UsdPrim& prim,
        TfToken const& interfaceAttrName,
        SdfValueTypeName const& typeName)
    : _name(interfaceAttrName)
{
    TfToken attrName = _GetName(interfaceAttrName);
    _attr = prim.GetAttribute(attrName);
    if (!_attr) {
        _attr = prim.CreateAttribute(attrName, typeName, /* custom = */ false);
    }
}

bool
UsdShadeInterfaceAttribute::Get(
        VtValue* value,
        UsdTimeCode time) const
{
    if (!_attr) {
        return false;
    }

    return _attr.Get(value, time);
}

TfToken
UsdShadeInterfaceAttribute::GetName() const
{
    return _name;
}

std::vector<UsdShadeParameter>
UsdShadeInterfaceAttribute::GetRecipientParameters(
        const TfToken& renderTarget) const
{
    UsdPrim prim = _attr.GetPrim();
    std::vector<UsdShadeParameter> ret;
    if (UsdRelationship rel = prim.GetRelationship(
                _GetInterfaceAttributeRelName(renderTarget, *this))) {
        std::vector<SdfPath> targets;
        rel.GetTargets(&targets);
        TF_FOR_ALL(targetIter, targets) {
            const SdfPath& target = *targetIter;
            if (target.IsPropertyPath()) {
                if (UsdPrim targetPrim = prim.GetStage()->GetPrimAtPath(
                            target.GetPrimPath())) {
                    if (UsdAttribute attr = targetPrim.GetAttribute(
                                target.GetNameToken())) {
                        ret.push_back(UsdShadeParameter(attr));
                    }
                }
            }
        }
    }
    return ret;
}

bool
UsdShadeInterfaceAttribute::Set(
        const VtValue& value,
        UsdTimeCode time) const
{
    if (!_attr) {
        return false;
    }

    return _attr.Set(value, time);
}

bool 
UsdShadeInterfaceAttribute::SetRecipient(
        const TfToken& renderTarget,
        const SdfPath& recipientPath) const
{
    if (UsdRelationship rel = _attr.GetPrim().CreateRelationship(
                _GetInterfaceAttributeRelName(renderTarget, *this), 
                /* custom = */ false)) {
        SdfPathVector targets;
        targets.push_back(recipientPath);
        return rel.SetTargets(targets);

        // TODO: Do add target and support Disconnect, etc.
        //return rel.AddTarget(recipientPath);
    }
    return false;
}
bool
UsdShadeInterfaceAttribute::SetRecipient(
        const TfToken& renderTarget,
        const UsdShadeParameter& recipient) const
{
    return SetRecipient(renderTarget, recipient.GetAttr().GetPath());
}

bool 
UsdShadeInterfaceAttribute::SetDocumentation(
        const std::string& docs) const
{
    if (!_attr) {
        return false;
    }

    return _attr.SetDocumentation(docs);
}

std::string 
UsdShadeInterfaceAttribute::GetDocumentation() const
{
    if (!_attr) {
        return "";
    }

    return _attr.GetDocumentation();
}

bool 
UsdShadeInterfaceAttribute::SetDisplayGroup(
        const std::string& docs) const
{
    if (!_attr) {
        return false;
    }

    return _attr.SetDisplayGroup(docs);
}

std::string 
UsdShadeInterfaceAttribute::GetDisplayGroup() const
{
    if (!_attr) {
        return "";
    }

    return _attr.GetDisplayGroup();
}

static TfToken 
_GetConnectionRelName(const TfToken &attrName)
{
    return TfToken(UsdShadeTokens->connectedSourceFor.GetString() + 
                   attrName.GetString());
}

static
UsdRelationship 
_GetConnectionRel(const UsdAttribute &interfaceAttr, 
                  bool create)
{
    if (!interfaceAttr) {
        TF_WARN("Invalid attribute: %s", UsdDescribe(interfaceAttr).c_str());
        return UsdRelationship();
    }

    const UsdPrim& prim = interfaceAttr.GetPrim();
    const TfToken& relName = _GetConnectionRelName(interfaceAttr.GetName());
    if (UsdRelationship rel = prim.GetRelationship(relName)) {
        return rel;
    }
    else if (create) {
        return prim.CreateRelationship(relName, /* custom = */ false);
    }

    return UsdRelationship();
}

bool 
UsdShadeInterfaceAttribute::ConnectToSource(
        UsdShadeConnectableAPI const &source, 
        TfToken const &sourceName,
        UsdShadeAttributeType const sourceType) const
{
    return UsdShadeConnectableAPI::ConnectToSource(GetAttr(), 
            source, sourceName, sourceType, GetTypeName());
}

bool 
UsdShadeInterfaceAttribute::ConnectToSource(const SdfPath &sourcePath) const
{
    // sourcePath needs to be a property path for us to make a connection.
    if (!sourcePath.IsPropertyPath())
        return false;

    UsdPrim sourcePrim = GetAttr().GetStage()->GetPrimAtPath(
        sourcePath.GetPrimPath());

    // We don't validate UsdShadeConnectableAPI as the type of the source prim 
    // may be unknown. (i.e. it could be a pure over or a typeless def).
    UsdShadeConnectableAPI source(sourcePrim);

    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    std::tie(sourceName, sourceType) = UsdShadeUtils::GetBaseNameAndType(
        sourcePath.GetNameToken());

    return ConnectToSource(source, sourceName, sourceType);
}

bool 
UsdShadeInterfaceAttribute::ConnectToSource(UsdShadeOutput const &output) const
{
    UsdShadeConnectableAPI source(output.GetAttr().GetPrim());
    return ConnectToSource(source, output.GetBaseName(), 
        /* sourceType */ UsdShadeAttributeType::Output);
}

bool 
UsdShadeInterfaceAttribute::ConnectToSource(UsdShadeParameter const &param) const
{
    UsdShadeConnectableAPI source(param.GetAttr().GetPrim());
    return ConnectToSource(source, param.GetName(),
        /* sourceType */ UsdShadeAttributeType::Parameter);
}

bool 
UsdShadeInterfaceAttribute::ConnectToSource(
    UsdShadeInterfaceAttribute const &interfaceAttr) const
{
    UsdShadeConnectableAPI source(interfaceAttr.GetAttr().GetPrim());
    return ConnectToSource(source, interfaceAttr.GetName(),
        /* sourceType */ UsdShadeAttributeType::Parameter);
}

bool 
UsdShadeInterfaceAttribute::DisconnectSource() const
{
    bool success = true;
    if (UsdRelationship rel = _GetConnectionRel(GetAttr(), false)) {
        success = rel.BlockTargets();
    }
    return success;
}

bool
UsdShadeInterfaceAttribute::ClearSource() const
{
    bool success = true;
    if (UsdRelationship rel = _GetConnectionRel(GetAttr(), false)) {
        success = rel.ClearTargets(/* removeSpec = */ true);
    }
    return success;
}

bool 
UsdShadeInterfaceAttribute::GetConnectedSource(
        UsdShadeConnectableAPI *source, 
        TfToken *outputName,
        UsdShadeAttributeType *sourceType) const
{
    if (!(source && outputName)){
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output parameters");
        return false;
    }
    
    return UsdShadeConnectableAPI::GetConnectedSource(GetAttr(), 
            source, outputName, sourceType);
}

bool 
UsdShadeInterfaceAttribute::IsConnected() const
{
    /// This MUST have the same semantics as GetConnectedSource(s).
    /// XXX someday we might make this more efficient through careful
    /// refactoring, but safest to just call the exact same code.
    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    return GetConnectedSource(&source, &sourceName, &sourceType);
}

PXR_NAMESPACE_CLOSE_SCOPE

