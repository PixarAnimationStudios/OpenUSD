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
#include "pxr/usd/usdShade/interfaceAttribute.h"

#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

// Windows does a #define for "interface", so 
// interface -> interfaceNS (for interface namespace)
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((interfaceNS, "interface:"))
    ((interfaceRecipientsOf, "interfaceRecipientsOf:"))
);

static std::string
_GetRelPrefix(const TfToken& renderTarget)
{
    return TfStringPrintf("%s:%s", 
            renderTarget.GetText(),
            _tokens->interfaceRecipientsOf.GetText());
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
    if (TfStringStartsWith(interfaceAttrName, _tokens->interfaceNS)){
        _attr = attr;
        _name = TfToken(interfaceAttrName.GetString().substr(_tokens->interfaceNS.GetString().size()));
    }
}

/* static */ TfToken
UsdShadeInterfaceAttribute::_GetName(
        const TfToken& interfaceAttrName)
{
    if (TfStringStartsWith(interfaceAttrName, _tokens->interfaceNS)) {
        return interfaceAttrName;
    }
    
    return TfToken(TfStringPrintf("%s%s",
            _tokens->interfaceNS.GetText(),
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
    if (not _attr) {
        _attr = prim.CreateAttribute(attrName, typeName, /* custom = */ false);
    }
}

bool
UsdShadeInterfaceAttribute::Get(
        VtValue* value,
        UsdTimeCode time) const
{
    if (not _attr) {
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
    if (not _attr) {
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
    if (not _attr) {
        return false;
    }

    return _attr.SetDocumentation(docs);
}

std::string 
UsdShadeInterfaceAttribute::GetDocumentation() const
{
    if (not _attr) {
        return "";
    }

    return _attr.GetDocumentation();
}

bool 
UsdShadeInterfaceAttribute::SetDisplayGroup(
        const std::string& docs) const
{
    if (not _attr) {
        return false;
    }

    return _attr.SetDisplayGroup(docs);
}

std::string 
UsdShadeInterfaceAttribute::GetDisplayGroup() const
{
    if (not _attr) {
        return "";
    }

    return _attr.GetDisplayGroup();
}

