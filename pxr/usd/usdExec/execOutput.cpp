//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/execOutput.h"
#include "pxr/usd/usdExec/execInput.h"
#include "pxr/usd/usdExec/execConnectableAPI.h"
#include "pxr/usd/usdExec/execUtils.h"

#include "pxr/usd/sdf/schema.h"


#include <stdlib.h>
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


using std::vector;
using std::string;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderType)
);

UsdExecOutput::UsdExecOutput(const UsdAttribute &attr)
    : _attr(attr)
{
}

TfToken 
UsdExecOutput::GetBaseName() const
{
    return TfToken(SdfPath::StripPrefixNamespace(
        GetFullName(), UsdExecTokens->outputs).first);
}

SdfValueTypeName 
UsdExecOutput::GetTypeName() const
{ 
    return _attr.GetTypeName();
}

static TfToken
_GetOutputAttrName(const TfToken outputName) 
{
    return TfToken(UsdExecTokens->outputs.GetString() + outputName.GetString());
}

UsdExecOutput::UsdExecOutput(
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
UsdExecOutput::Set(const VtValue& value,
                    UsdTimeCode time) const
{
    if (UsdAttribute attr = GetAttr()) {
        return attr.Set(value, time);
    }
    return false;
}

bool 
UsdExecOutput::SetRenderType(
        TfToken const& renderType) const
{
    return _attr.SetMetadata(_tokens->renderType, renderType);
}

TfToken 
UsdExecOutput::GetRenderType() const
{
    TfToken renderType;
    _attr.GetMetadata(_tokens->renderType, &renderType);
    return renderType;
}

bool 
UsdExecOutput::HasRenderType() const
{
    return _attr.HasMetadata(_tokens->renderType);
}

NdrTokenMap
UsdExecOutput::GetExecMetadata() const
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
UsdExecOutput::GetExecMetadataByKey(const TfToken &key) const
{
    VtValue val;
    GetAttr().GetMetadataByDictKey(UsdExecTokens->execMetadata, key, &val);
    return TfStringify(val);
}
    
void 
UsdExecOutput::SetExecMetadata(const NdrTokenMap &execMetadata) const
{
    for (auto &i: execMetadata) {
        SetExecMetadataByKey(i.first, i.second);
    }
}

void 
UsdExecOutput::SetExecMetadataByKey(
    const TfToken &key, 
    const std::string &value) const
{
    GetAttr().SetMetadataByDictKey(UsdExecTokens->execMetadata, key, value);
}

bool 
UsdExecOutput::HasExecMetadata() const
{
    return GetAttr().HasMetadata(UsdExecTokens->execMetadata);
}

bool 
UsdExecOutput::HasExecMetadataByKey(const TfToken &key) const
{
    return GetAttr().HasMetadataDictKey(UsdExecTokens->execMetadata, key);
}

void 
UsdExecOutput::ClearExecMetadata() const
{
    GetAttr().ClearMetadata(UsdExecTokens->execMetadata);
}

void
UsdExecOutput::ClearExecMetadataByKey(const TfToken &key) const
{
    GetAttr().ClearMetadataByDictKey(UsdExecTokens->execMetadata, key);
}

/* static */
bool 
UsdExecOutput::IsOutput(const UsdAttribute &attr)
{
    return TfStringStartsWith(attr.GetName().GetString(), 
                              UsdExecTokens->outputs);
}

bool 
UsdExecOutput::CanConnect(const UsdAttribute &source) const
{
    return UsdExecConnectableAPI::CanConnect(*this, source);
}

bool 
UsdExecOutput::CanConnect(const UsdExecInput &sourceInput) const 
{
    return CanConnect(sourceInput.GetAttr());
}

bool 
UsdExecOutput::CanConnect(const UsdExecOutput &sourceOutput) const 
{
    return CanConnect(sourceOutput.GetAttr());
}

bool
UsdExecOutput::ConnectToSource(
    UsdExecConnectionSourceInfo const &source,
    ConnectionModification const mod) const
{
    return UsdExecConnectableAPI::ConnectToSource(*this, source, mod);
}

bool 
UsdExecOutput::ConnectToSource(
    UsdExecConnectableAPI const &source, 
    TfToken const &sourceName, 
    UsdExecAttributeType const sourceType,
    SdfValueTypeName typeName) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, source,
        sourceName, sourceType, typeName);
}

bool 
UsdExecOutput::ConnectToSource(SdfPath const &sourcePath) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, sourcePath);
}

bool 
UsdExecOutput::ConnectToSource(UsdExecInput const &sourceInput) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, sourceInput);
}

bool 
UsdExecOutput::ConnectToSource(UsdExecOutput const &sourceOutput) const 
{
    return UsdExecConnectableAPI::ConnectToSource(*this, sourceOutput);
}

bool
UsdExecOutput::SetConnectedSources(
    std::vector<UsdExecConnectionSourceInfo> const &sourceInfos) const
{
    return UsdExecConnectableAPI::SetConnectedSources(*this, sourceInfos);
}

UsdExecOutput::SourceInfoVector
UsdExecOutput::GetConnectedSources(SdfPathVector *invalidSourcePaths) const
{
    return UsdExecConnectableAPI::GetConnectedSources(*this,
                                                       invalidSourcePaths);
}

bool 
UsdExecOutput::GetConnectedSource(
    UsdExecConnectableAPI *source, 
    TfToken *sourceName,
    UsdExecAttributeType *sourceType) const 
{
    return UsdExecConnectableAPI::GetConnectedSource(*this, source, 
        sourceName, sourceType);
}

bool 
UsdExecOutput::GetRawConnectedSourcePaths(SdfPathVector *sourcePaths) const 
{
    return UsdExecConnectableAPI::GetRawConnectedSourcePaths(*this, 
        sourcePaths);
}

bool 
UsdExecOutput::HasConnectedSource() const 
{
    return UsdExecConnectableAPI::HasConnectedSource(*this);
}

bool 
UsdExecOutput::DisconnectSource(UsdAttribute const &sourceAttr) const
{
    return UsdExecConnectableAPI::DisconnectSource(*this, sourceAttr);
}

bool
UsdExecOutput::ClearSources() const
{
    return UsdExecConnectableAPI::ClearSources(*this);
}

bool 
UsdExecOutput::ClearSource() const 
{
    return UsdExecConnectableAPI::ClearSources(*this);
}

UsdExecAttributeVector
UsdExecOutput::GetValueProducingAttributes(
    bool outputsOnly) const
{
    return UsdExecUtils::GetValueProducingAttributes(*this,
                                                      outputsOnly);
}

PXR_NAMESPACE_CLOSE_SCOPE

