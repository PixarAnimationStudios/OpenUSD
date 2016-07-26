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
#include "pxr/usd/usd/attribute.h"

#include "pxr/usd/usd/conversions.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/interpolators.h"
#include "pxr/usd/usd/resolveInfo.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"

// NOTE: this is not actually used, but AttributeSpec requires it
#include "pxr/usd/sdf/relationshipSpec.h"

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/utility/enable_if.hpp>
#include <vector>

// ------------------------------------------------------------------------- //
// UsdAttribute 
// ------------------------------------------------------------------------- //

SdfVariability
UsdAttribute::GetVariability() const
{
    return _GetStage()->_GetVariability(*this);
}

bool 
UsdAttribute::SetVariability(SdfVariability variability) const
{
    return SetMetadata(SdfFieldKeys->Variability, variability);
}

SdfValueTypeName
UsdAttribute::GetTypeName() const
{
    TfToken typeName;
    GetMetadata(SdfFieldKeys->TypeName, &typeName);
    return SdfSchema::GetInstance().FindType(typeName);
}

TfToken
UsdAttribute::GetRoleName() const
{
    return GetTypeName().GetRole();
}

bool 
UsdAttribute::SetTypeName(const SdfValueTypeName& typeName) const
{
    return SetMetadata(SdfFieldKeys->TypeName, typeName.GetAsToken());
}

void 
UsdAttribute::Block() 
{
    Clear();
    Set(VtValue(SdfValueBlock()), UsdTimeCode::Default()); 
}

bool
UsdAttribute::GetTimeSamples(std::vector<double>* times) const 
{
    return _GetStage()->_GetTimeSamplesInInterval(*this, 
        GfInterval::GetFullInterval(), times);
}

size_t
UsdAttribute::GetNumTimeSamples() const 
{
    return _GetStage()->_GetNumTimeSamples(*this);
}

bool 
UsdAttribute::GetBracketingTimeSamples(double desiredTime, 
                                       double* lower, 
                                       double* upper, 
                                       bool* hasTimeSamples) const
{
    return _GetStage()->_GetBracketingTimeSamples(
        *this, desiredTime, /*requireAuthored*/ false,
        lower, upper, hasTimeSamples);
}

bool
UsdAttribute::GetTimeSamplesInInterval(const GfInterval& interval,
                                       std::vector<double>* times) const
{
    return _GetStage()->_GetTimeSamplesInInterval(*this, interval, times);  
}

bool 
UsdAttribute::HasAuthoredValueOpinion() const
{
    Usd_ResolveInfo resolveInfo;
    _GetStage()->_GetResolveInfo(*this, &resolveInfo);
    bool authoredValueFound = 
        resolveInfo.source == Usd_ResolveInfoSourceDefault
        or resolveInfo.source == Usd_ResolveInfoSourceTimeSamples
        or resolveInfo.source == Usd_ResolveInfoSourceValueClips;

    return authoredValueFound or resolveInfo.valueIsBlocked;
}

bool 
UsdAttribute::HasValue() const
{
    Usd_ResolveInfo resolveInfo;
    _GetStage()->_GetResolveInfo(*this, &resolveInfo);
    return resolveInfo.source != Usd_ResolveInfoSourceNone;
}

bool
UsdAttribute::HasFallbackValue() const
{
    return _GetStage()->_GetAttributeDefinition(*this)->HasDefaultValue();
}

bool 
UsdAttribute::ValueMightBeTimeVarying() const
{
    return _GetStage()->_ValueMightBeTimeVarying(*this);
}

template <typename T>
bool 
UsdAttribute::_Get(T* value, UsdTimeCode time) const 
{
    return _GetStage()->_GetValue(time, *this, value);
}

bool 
UsdAttribute::Get(VtValue* value, UsdTimeCode time) const 
{
    auto stage = _GetStage();
    bool foundValue = stage->_GetValue(time, *this, value);

    // Special case for SdfAssetPath -- compute the resolved asset path.
    if (foundValue and value) {
        stage->_MakeResolvedAssetPaths(time, *this, value);
    }

    return foundValue;
}

// Specializations for SdfAssetPath(Array) that do path resolution.
template <>
bool
UsdAttribute::_Get(SdfAssetPath *assetPath, UsdTimeCode time) const
{
    auto stage = _GetStage();

    if (stage->_GetValue(time, *this, assetPath)) {
        stage->_MakeResolvedAssetPaths(time, *this, assetPath, 1);
        return true;
    }

    return false;
}

template <>
bool
UsdAttribute::_Get(VtArray<SdfAssetPath> *assetPaths, UsdTimeCode time) const
{
    auto stage = _GetStage();

    if (stage->_GetValue(time, *this, assetPaths)) {
        stage->_MakeResolvedAssetPaths(time, *this, assetPaths->data(), 
                                       assetPaths->size());
        return true;
    }

    return false;
}

bool 
UsdAttribute::_UntypedSet(const SdfAbstractDataConstValue& value,
                          UsdTimeCode time) const
{
    return _GetStage()->_SetValue(time, *this, value);
}

bool 
UsdAttribute::Set(const VtValue& value, UsdTimeCode time) const 
{ 
    return _GetStage()->_SetValue(time, *this, value);
}

bool
UsdAttribute::Clear() const
{
    return ClearDefault() 
       and ClearMetadata(SdfFieldKeys->TimeSamples);
}

bool
UsdAttribute::ClearAtTime(UsdTimeCode time) const
{
    return _GetStage()->_ClearValue(time, *this);
}

bool
UsdAttribute::ClearDefault() const
{
    return ClearAtTime(UsdTimeCode::Default());
}

SdfAttributeSpecHandle
UsdAttribute::_CreateSpec(const SdfValueTypeName& typeName, bool custom,
                          const SdfVariability &variability) const
{
    UsdStage *stage = _GetStage();
    
    if (variability != SdfVariabilityVarying and 
        variability != SdfVariabilityUniform){
        TF_CODING_ERROR("UsdAttributes can only possess variability "
                        "varying or uniform.  Cannot create attribute %s.%s",
                        GetPrimPath().GetText(), _PropName().GetText());
        return TfNullPtr;
    }
    
    // Try to create a spec for editing either from the definition or from
    // copying existing spec info.
    TfErrorMark m;
    if (SdfAttributeSpecHandle attrSpec =
        stage->_CreateAttributeSpecForEditing(*this))
        return attrSpec;

    // If creating the spec on the stage failed without issuing an error, that
    // means there was no existing authored scene description to go on (i.e. no
    // builtin info from prim type, and no existing authored spec).  Stamp a
    // spec with the provided default values.
    if (m.IsClean()) {
        SdfChangeBlock block;
        return SdfAttributeSpec::New(
            stage->_CreatePrimSpecForEditing(GetPrimPath()),
            _PropName(), typeName, variability, custom);
    }
    return TfNullPtr;
}

bool
UsdAttribute::_Create(const SdfValueTypeName& typeName, bool custom,
                      const SdfVariability &variability) const
{
    return _CreateSpec(typeName, custom, variability);
}

// Explicitly instantiate templated getters for all Sdf value
// types.
#define _INSTANTIATE_GET(r, unused, elem)                               \
    template bool UsdAttribute::_Get(                                   \
        SDF_VALUE_TRAITS_TYPE(elem)::Type*, UsdTimeCode) const;         \
    template bool UsdAttribute::_Get(                                   \
        SDF_VALUE_TRAITS_TYPE(elem)::ShapedType*, UsdTimeCode) const;

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_GET, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_GET
