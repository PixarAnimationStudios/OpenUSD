//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/execInput.h"

#include "pxr/usd/usdExec/execConnectableAPI.h"
#include "pxr/usd/usdExec/execOutput.h"
#include "pxr/usd/usdExec/execUtils.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/relationship.h"

#include "pxr/base/tf/smallVector.h"

#include <stdlib.h>
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


using std::vector;
using std::string;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (connectability)
    (renderType)
);

UsdExecInput::UsdExecInput(const UsdAttribute &attr)
    : _attr(attr)
{
}

TfToken 
UsdExecInput::GetBaseName() const
{
    string name = GetFullName();
    if (TfStringStartsWith(name, UsdExecTokens->inputs)) {
        return TfToken(name.substr(UsdExecTokens->inputs.GetString().size()));
    }
    
    return GetFullName();
}

SdfValueTypeName 
UsdExecInput::GetTypeName() const
{ 
    return _attr.GetTypeName();
}

static TfToken
_GetInputAttrName(const TfToken inputName) 
{
    return TfToken(UsdExecTokens->inputs.GetString() + inputName.GetString());
}

UsdExecInput::UsdExecInput(
    UsdPrim prim,
    TfToken const &name,
    SdfValueTypeName const &typeName)
{
    // XXX what do we do if the type name doesn't match and it exists already?
    TfToken inputAttrName = _GetInputAttrName(name);
    if (prim.HasAttribute(inputAttrName)) {
        _attr = prim.GetAttribute(inputAttrName);
    }

    if (!_attr) {
        _attr = prim.CreateAttribute(inputAttrName, typeName, 
            /* custom = */ false);
    }
}

bool
UsdExecInput::Get(VtValue* value, UsdTimeCode time) const
{
    if (!_attr) {
        return false;
    }

    return _attr.Get(value, time);
}

bool
UsdExecInput::Set(const VtValue& value, UsdTimeCode time) const
{
    return _attr.Set(value, time);
}

bool 
UsdExecInput::SetRenderType(TfToken const& renderType) const
{
    return _attr.SetMetadata(_tokens->renderType, renderType);
}

TfToken 
UsdExecInput::GetRenderType() const
{
    TfToken renderType;
    _attr.GetMetadata(_tokens->renderType, &renderType);
    return renderType;
}

bool 
UsdExecInput::HasRenderType() const
{
    return _attr.HasMetadata(_tokens->renderType);
}


NdrTokenMap
UsdExecInput::GetExecMetadata() const
{
    NdrTokenMap result;

    VtDictionary execMetadata;
    if (GetAttr().GetMetadata(UsdExecTokens->execMetadata, &execMetadata)){
        for (const auto &it : execMetadata) {
            result[TfToken(it.first)] = TfStringify(it.second);
        }
    }

    return result;
}

std::string 
UsdExecInput::GetExecMetadataByKey(const TfToken &key) const
{
    VtValue val;
    GetAttr().GetMetadataByDictKey(UsdExecTokens->execMetadata, key, &val);
    return TfStringify(val);
}
    
void 
UsdExecInput::SetExecMetadata(const NdrTokenMap &execMetadata) const
{
    for (auto &i: execMetadata) {
        SetExecMetadataByKey(i.first, i.second);
    }
}

void 
UsdExecInput::SetExecMetadataByKey(
    const TfToken &key, 
    const std::string &value) const
{
    GetAttr().SetMetadataByDictKey(UsdExecTokens->execMetadata, key, value);
}

bool 
UsdExecInput::HasExecMetadata() const
{
    return GetAttr().HasMetadata(UsdExecTokens->execMetadata);
}

bool 
UsdExecInput::HasExecMetadataByKey(const TfToken &key) const
{
    return GetAttr().HasMetadataDictKey(UsdExecTokens->execMetadata, key);
}

void 
UsdExecInput::ClearExecMetadata() const
{
    GetAttr().ClearMetadata(UsdExecTokens->execMetadata);
}

void
UsdExecInput::ClearExecMetadataByKey(const TfToken &key) const
{
    GetAttr().ClearMetadataByDictKey(UsdExecTokens->execMetadata, key);
}

/* static */
bool 
UsdExecInput::IsInput(const UsdAttribute &attr)
{
    return attr && attr.IsDefined() && 
             TfStringStartsWith(attr.GetName().GetString(), 
                                UsdExecTokens->inputs);
}

/* static */
bool
UsdExecInput::IsInterfaceInputName(const std::string & name)
{
    if (TfStringStartsWith(name, UsdExecTokens->inputs)) {
        return true;
    }

    return false;
}

bool 
UsdExecInput::SetDocumentation(const std::string& docs) const
{
    if (!_attr) {
        return false;
    }

    return _attr.SetDocumentation(docs);
}

std::string 
UsdExecInput::GetDocumentation() const
{
    if (!_attr) {
        return "";
    }

    return _attr.GetDocumentation();
}

bool 
UsdExecInput::SetDisplayGroup(const std::string& docs) const
{
    if (!_attr) {
        return false;
    }

    return _attr.SetDisplayGroup(docs);
}

std::string 
UsdExecInput::GetDisplayGroup() const
{
    if (!_attr) {
        return "";
    }

    return _attr.GetDisplayGroup();
}

bool 
UsdExecInput::CanConnect(const UsdAttribute &source) const 
{
    return UsdExecConnectableAPI::CanConnect(*this, source);
}

bool 
UsdExecInput::CanConnect(const UsdExecInput &sourceInput) const 
{
    return CanConnect(sourceInput.GetAttr());
}

bool 
UsdExecInput::CanConnect(const UsdExecOutput &sourceOutput) const
{
    return CanConnect(sourceOutput.GetAttr());
}

bool
UsdExecInput::ConnectToSource(
    UsdExecConnectionSourceInfo const &source,
    ConnectionModification const mod) const
{
    return UsdExecConnectableAPI::ConnectToSource(*this, source, mod);
}

bool 
UsdExecInput::ConnectToSource(
    UsdExecConnectableAPI const &source, 
    TfToken const &sourceName, 
    UsdExecAttributeType const sourceType,
    SdfValueTypeName typeName) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, source,
        sourceName, sourceType, typeName);
}

bool 
UsdExecInput::ConnectToSource(SdfPath const &sourcePath) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, sourcePath);
}

bool 
UsdExecInput::ConnectToSource(UsdExecInput const &sourceInput) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, sourceInput);
}

bool 
UsdExecInput::ConnectToSource(UsdExecOutput const &sourceOutput) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, sourceOutput);
}

bool
UsdExecInput::SetConnectedSources(
    std::vector<UsdExecConnectionSourceInfo> const &sourceInfos) const
{
    return UsdExecConnectableAPI::SetConnectedSources(*this, sourceInfos);
}

UsdExecInput::SourceInfoVector
UsdExecInput::GetConnectedSources(SdfPathVector *invalidSourcePaths) const
{
    return UsdExecConnectableAPI::GetConnectedSources(*this,
                                                       invalidSourcePaths);
}

bool 
UsdExecInput::GetConnectedSource(UsdExecConnectableAPI *source, 
                                  TfToken *sourceName,
                                  UsdExecAttributeType *sourceType) const 
{
    return UsdExecConnectableAPI::GetConnectedSource(*this, source, 
        sourceName, sourceType);
}

bool 
UsdExecInput::GetRawConnectedSourcePaths(SdfPathVector *sourcePaths) const 
{
    return UsdExecConnectableAPI::GetRawConnectedSourcePaths(*this, 
        sourcePaths);
}

bool 
UsdExecInput::HasConnectedSource() const 
{
    return UsdExecConnectableAPI::HasConnectedSource(*this);
}

bool 
UsdExecInput::DisconnectSource(UsdAttribute const &sourceAttr) const
{
    return UsdExecConnectableAPI::DisconnectSource(*this, sourceAttr);
}

bool
UsdExecInput::ClearSources() const
{
    return UsdExecConnectableAPI::ClearSources(*this);
}

bool 
UsdExecInput::ClearSource() const 
{
    return UsdExecConnectableAPI::ClearSources(*this);
}

bool 
UsdExecInput::SetConnectability(const TfToken &connectability) const
{
    return _attr.SetMetadata(_tokens->connectability, connectability);
}

TfToken 
UsdExecInput::GetConnectability() const
{
    TfToken connectability; 
    _attr.GetMetadata(_tokens->connectability, &connectability);

    // If there's an authored non-empty connectability value, then return it. 
    // If not, return "full".
    if (!connectability.IsEmpty()) {
        return connectability;
    }

    return UsdExecTokens->full;
}

bool 
UsdExecInput::ClearConnectability() const
{
    return _attr.ClearMetadata(_tokens->connectability);
}

UsdExecAttributeVector
UsdExecInput::GetValueProducingAttributes(
    bool outputsOnly) const
{
    return UsdExecUtils::GetValueProducingAttributes(*this,
                                                      outputsOnly);
}

UsdAttribute
UsdExecInput::GetValueProducingAttribute(UsdExecAttributeType* attrType) const
{
    // Call the multi-connection aware version
    UsdExecAttributeVector valueAttrs =
        UsdExecUtils::GetValueProducingAttributes(*this);

    if (valueAttrs.empty()) {
        if (attrType) {
            *attrType = UsdExecAttributeType::Invalid;
        }
        return UsdAttribute();
    } else {
        // If we have valid connections extract the first one
        if (valueAttrs.size() > 1) {
            TF_WARN("More than one value producing attribute for shading input "
                    "%s. GetValueProducingAttribute will only report the first "
                    "one. Please use GetValueProducingAttributes to retrieve "
                    "all.", GetAttr().GetPath().GetText());
        }

        UsdAttribute attr = valueAttrs[0];
        if (attrType) {
            *attrType = UsdExecUtils::GetType(attr.GetName());
        }

        return attr;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
