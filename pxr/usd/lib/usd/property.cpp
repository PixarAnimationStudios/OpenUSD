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
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/stage.h"

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

    if (not TF_VERIFY(delim != fullName.size()-1))
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

    if (not TF_VERIFY(delim != fullName.size()-1))
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
        return not mappedPath.IsEmpty() and
            editTarget.GetLayer()->HasSpec(
                SdfAbstractDataSpecId(&mappedPath, &_PropName()));
    }
    return false;
}
