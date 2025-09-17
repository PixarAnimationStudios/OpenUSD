//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/pcp/targetIndex.h"

PXR_NAMESPACE_OPEN_SCOPE


SdfPropertySpecHandleVector 
UsdProperty::GetPropertyStack(UsdTimeCode time) const
{
    return _GetStage()->_GetPropertyStack(*this, time); 
}

std::vector<std::pair<SdfPropertySpecHandle, SdfLayerOffset>> 
UsdProperty::GetPropertyStackWithLayerOffsets(UsdTimeCode time) const
{
    return _GetStage()->_GetPropertyStackWithLayerOffsets(*this, time); 
}

TfToken
UsdProperty::GetBaseName() const
{
    std::string const &fullName = _PropName().GetString();
    size_t delim = fullName.rfind(GetNamespaceDelimiter());

    if (!TF_VERIFY(delim != fullName.size()-1))
        return TfToken();

    return ((delim == std::string::npos) ?
            _PropName() :
            TfToken(fullName.c_str() + delim+1));
}

TfToken
UsdProperty::GetNamespace() const
{
    std::string const &fullName = _PropName().GetString();
    size_t delim = fullName.rfind(GetNamespaceDelimiter());

    if (!TF_VERIFY(delim != fullName.size()-1))
        return TfToken();

    return ((delim == std::string::npos) ?
            TfToken() :
            TfToken(fullName.substr(0, delim)));
}

std::vector<std::string>
UsdProperty::SplitName() const
{
    return SdfPath::TokenizeIdentifier(_PropName());
}

std::string
UsdProperty::GetDisplayGroup() const
{
    std::string result;
    GetMetadata(SdfFieldKeys->DisplayGroup, &result);
    return result;
}

bool
UsdProperty::SetDisplayGroup(const std::string& displayGroup) const
{
    return SetMetadata(SdfFieldKeys->DisplayGroup, displayGroup);
}

bool
UsdProperty::ClearDisplayGroup() const
{
    return ClearMetadata(SdfFieldKeys->DisplayGroup);
}

bool
UsdProperty::HasAuthoredDisplayGroup() const
{
    return HasAuthoredMetadata(SdfFieldKeys->DisplayGroup);
}

std::vector<std::string> 
UsdProperty::GetNestedDisplayGroups() const
{
    return TfStringTokenize(GetDisplayGroup(), ":");
}

bool
UsdProperty::SetNestedDisplayGroups(const std::vector<std::string>& nestedGroups) const
{
    return SetDisplayGroup(SdfPath::JoinIdentifier(nestedGroups));
}

bool
UsdProperty::IsCustom() const
{
    return _GetStage()->_IsCustom(*this);
}

bool
UsdProperty::SetCustom(bool isCustom) const
{
    return SetMetadata(SdfFieldKeys->Custom, isCustom);
}

bool
UsdProperty::IsDefined() const
{
    return IsValid();
}

bool
UsdProperty::IsAuthored() const
{
    // Look for the strongest authored property spec.
    for (Usd_Resolver res(
             &GetPrim().GetPrimIndex()); res.IsValid(); res.NextLayer()) {
        if (res.GetLayer()->HasSpec(
                res.GetLocalPath().AppendProperty(_PropName()))) {
            return true;
        }
    }
    return false;
}

bool
UsdProperty::IsAuthoredAt(const UsdEditTarget &editTarget) const
{
    if (editTarget.IsValid()) {
        SdfPath mappedPath = editTarget.MapToSpecPath(GetPath());
        return !mappedPath.IsEmpty() &&
            editTarget.GetLayer()->HasSpec(mappedPath);
    }
    return false;
}

UsdProperty 
UsdProperty::FlattenTo(const UsdPrim &parent) const
{
    return _GetStage()->_FlattenProperty(*this, parent, GetName());
}

UsdProperty 
UsdProperty::FlattenTo(const UsdPrim &parent, const TfToken &propName) const
{
    return _GetStage()->_FlattenProperty(*this, parent, propName);
}

UsdProperty 
UsdProperty::FlattenTo(const UsdProperty &property) const
{
    return _GetStage()->_FlattenProperty(
        *this, property.GetPrim(), property.GetName());
}

bool
UsdProperty::_GetTargets(SdfSpecType specType, SdfPathVector *out,
                         bool *foundErrors) const
{
    if (!TF_VERIFY(specType == SdfSpecTypeAttribute ||
                   specType == SdfSpecTypeRelationship)) {
        return false;
    }

    TRACE_FUNCTION();

    UsdStage *stage = _GetStage();
    PcpErrorVector pcpErrors;
    PcpTargetIndex targetIndex;
    {
        // Our intention is that the following code requires read-only
        // access to the PcpCache, so use a const-ref.
        const PcpCache& pcpCache(*stage->_GetPcpCache());
        // In USD mode, Pcp does not cache property indexes, so we
        // compute one here ourselves and use that.  First, we need
        // to get the prim index of the owning prim.
        const PcpPrimIndex &primIndex = _Prim()->GetPrimIndex();
        // PERFORMANCE: Here we can't avoid constructing the full property path
        // without changing the Pcp API.  We're about to do serious
        // composition/indexing, though, so the added expense may be neglible.
        const PcpSite propSite(pcpCache.GetLayerStackIdentifier(), GetPath());
        PcpPropertyIndex propIndex;
        PcpBuildPrimPropertyIndex(propSite.path, pcpCache, primIndex,
                                  &propIndex, &pcpErrors);
        PcpBuildTargetIndex(propSite, propIndex, specType,
                            &targetIndex, &pcpErrors);
    }

    if (!targetIndex.paths.empty() && _Prim()->IsInPrototype()) {
        UsdPrim::_ProtoToInstancePathMap pathMap =
            GetPrim()._GetProtoToInstancePathMap();
        // Now map the targets.
        for (SdfPath const &target : targetIndex.paths) {
            out->push_back(pathMap.MapProtoToInstance(target));
            if (out->back().IsEmpty()) {
                out->pop_back();
            }
        }
    }
    else {
        out->swap(targetIndex.paths);
    }

    // TODO: handle errors
    const bool isClean = pcpErrors.empty();
    if (!isClean) {
        stage->_ReportPcpErrors(pcpErrors,
            TfStringPrintf(specType == SdfSpecTypeAttribute ?
                           "getting connections for attribute <%s>" :
                           "getting targets for relationship <%s>",
                           GetPath().GetText()));
        if (foundErrors) {
            *foundErrors = true;
        }
    }

    return isClean && targetIndex.hasTargetOpinions;
}

PXR_NAMESPACE_CLOSE_SCOPE

