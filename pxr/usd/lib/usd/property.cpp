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
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/pcp/targetIndex.h"

#include <boost/iterator/transform_iterator.hpp>

PXR_NAMESPACE_OPEN_SCOPE


SdfPropertySpecHandleVector 
UsdProperty::GetPropertyStack(UsdTimeCode time) const
{
    return _GetStage()->_GetPropertyStack(*this, time); 
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


std::string
UsdProperty::GetDisplayName() const
{
    std::string result;
    GetMetadata(SdfFieldKeys->DisplayName, &result);
    return result;
}

bool
UsdProperty::SetDisplayName(const std::string& newDisplayName) const
{
    return SetMetadata(SdfFieldKeys->DisplayName, newDisplayName);
}

bool
UsdProperty::ClearDisplayName() const
{
    return ClearMetadata(SdfFieldKeys->DisplayName);
}

bool
UsdProperty::HasAuthoredDisplayName() const
{
    return HasAuthoredMetadata(SdfFieldKeys->DisplayName);
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
                SdfAbstractDataSpecId(&res.GetLocalPath(), &_PropName())))
            return true;
    }
    return false;
}

bool
UsdProperty::IsAuthoredAt(const UsdEditTarget &editTarget) const
{
    if (editTarget.IsValid()) {
        SdfPath mappedPath = editTarget.MapToSpecPath(GetPrimPath());
        return !mappedPath.IsEmpty() &&
            editTarget.GetLayer()->HasSpec(
                SdfAbstractDataSpecId(&mappedPath, &_PropName()));
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

// Map from path to replacement for remapping target paths during flattening.
using _PathMap = std::vector<std::pair<SdfPath, SdfPath>>;

// Apply path remappings to a list of target paths.
static SdfPath
_MapPath(_PathMap const &map, SdfPath const &path)
{
    using boost::make_transform_iterator;

    if (map.empty()) {
        return path;
    }

    auto it = SdfPathFindLongestPrefix(
        make_transform_iterator(map.begin(), TfGet<0>()),
        make_transform_iterator(map.end(), TfGet<0>()), path);
    if (it.base() != map.end()) {
        return path.ReplacePrefix(it.base()->first, it.base()->second);
    }
    return path;
}

bool
UsdProperty::_GetTargets(SdfSpecType specType, SdfPathVector *out) const
{
    if (!TF_VERIFY(specType == SdfSpecTypeAttribute ||
                   specType == SdfSpecTypeRelationship)) {
        return false;
    }

    TRACE_FUNCTION();

    UsdStage *stage = _GetStage();
    PcpErrorVector pcpErrors;
    std::vector<std::string> otherErrors;
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

    if (!targetIndex.paths.empty() && _Prim()->IsInMaster()) {

        // Walk up to the root while we're in (nested) instance-land.  When we
        // hit an instance or a master, add a mapping for the master source prim
        // index path to this particular instance (proxy) path.
        _PathMap pathMap;

        // This prim might be an instance proxy inside a master, if so use its
        // master, but be sure to skip up to the parent if *this* prim is an
        // instance.  Target paths on *this* prim are in the "space" of its next
        // ancestral master, just as how attribute & metadata values come from
        // the instance itself, not its master.
        UsdPrim prim = GetPrim();
        if (prim.IsInstance()) {
            prim = prim.GetParent();
        }
        for (; prim; prim = prim.GetParent()) {
            UsdPrim master;
            if (prim.IsInstance()) {
                master = prim.GetMaster();
            } else if (prim.IsMaster()) {
                master = prim;
            }
            if (master) {
                pathMap.emplace_back(master._GetSourcePrimIndex().GetPath(),
                                     prim.GetPath());
            }
        };
        std::sort(pathMap.begin(), pathMap.end());

        // Now map the targets.
        for (SdfPath const &target : targetIndex.paths) {
            out->push_back(_MapPath(pathMap, target));
            if (out->back().IsEmpty()) {
                out->pop_back();
            }
        }
    }
    else {
        out->swap(targetIndex.paths);
    }

    // TODO: handle errors
    const bool isClean = pcpErrors.empty() && otherErrors.empty();
    if (!isClean) {
        stage->_ReportErrors(
            pcpErrors, otherErrors,
            TfStringPrintf(specType == SdfSpecTypeAttribute ?
                           "getting connections for attribute <%s>" :
                           "getting targets for relationship <%s>",
                           GetPath().GetText()));
    }

    return isClean;
}

PXR_NAMESPACE_CLOSE_SCOPE

