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
#include "pxr/usd/usd/attributeQuery.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/valueUtils.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/base/trace/trace.h"

#include <boost/preprocessor/seq/for_each.hpp>

PXR_NAMESPACE_OPEN_SCOPE


UsdAttributeQuery::UsdAttributeQuery(
    const UsdAttribute& attr)
{
    _Initialize(attr);
}

UsdAttributeQuery::UsdAttributeQuery(
    const UsdPrim& prim, const TfToken& attrName)
{
    UsdAttribute attr = prim.GetAttribute(attrName);
    if (!attr) {
        TF_CODING_ERROR(
            "Invalid attribute '%s' on prim <%s>",
            attrName.GetText(), prim.GetPath().GetString().c_str());
        return;
    }

    _Initialize(attr);
}

std::vector<UsdAttributeQuery>
UsdAttributeQuery::CreateQueries(
    const UsdPrim& prim, const TfTokenVector& attrNames)
{
    std::vector<UsdAttributeQuery> rval;
    rval.reserve(attrNames.size());
    for (const auto& attrName : attrNames) {
        rval.push_back(UsdAttributeQuery(prim, attrName));
    }

    return rval;
}

UsdAttributeQuery::UsdAttributeQuery()
{
}

void
UsdAttributeQuery::_Initialize(const UsdAttribute& attr)
{
    TRACE_FUNCTION();

    if (!attr) {
        TF_CODING_ERROR("Invalid attribute");
        return;
    }

    const UsdStage* stage = attr._GetStage();
    stage->_GetResolveInfo(attr, &_resolveInfo);

    _attr = attr;
}

const UsdAttribute& 
UsdAttributeQuery::GetAttribute() const
{
    return _attr;
}

template <typename T>
USD_API
bool 
UsdAttributeQuery::_Get(T* value, UsdTimeCode time) const
{
    return _attr._GetStage()->_GetValueFromResolveInfo(
        _resolveInfo, time, _attr, value);
}

template <>
USD_API
bool
UsdAttributeQuery::_Get(VtArray<SdfAssetPath>* assetPaths, 
                        UsdTimeCode time) const
{
    auto stage = _attr._GetStage();

    if (stage->_GetValueFromResolveInfo(_resolveInfo, time, _attr, assetPaths)){
        stage->_MakeResolvedAssetPaths(time, _attr, assetPaths->data(), 
                                       assetPaths->size());
        return true;
    }

    return false;
}


template <>
USD_API
bool
UsdAttributeQuery::_Get(SdfAssetPath* assetPath, UsdTimeCode time) const
{
    auto stage = _attr._GetStage();

    if (stage->_GetValueFromResolveInfo(_resolveInfo, time, _attr, assetPath)) {
        stage->_MakeResolvedAssetPaths(time, _attr, assetPath, 1);
        return true;
    }

    return false;
}

bool 
UsdAttributeQuery::Get(VtValue* value, UsdTimeCode time) const
{
    auto stage = _attr._GetStage();
    bool foundValue = stage->_GetValueFromResolveInfo(_resolveInfo, time, 
                                                      _attr, value);

    if (foundValue && value) {
        stage->_MakeResolvedAssetPaths(time, _attr, value);
    }

    return foundValue;
}

bool 
UsdAttributeQuery::GetTimeSamples(std::vector<double>* times) const
{
    return _attr._GetStage()->_GetTimeSamplesInIntervalFromResolveInfo(
        _resolveInfo, _attr, GfInterval::GetFullInterval(), times);
}

bool
UsdAttributeQuery::GetTimeSamplesInInterval(const GfInterval& interval,
                                            std::vector<double>* times) const
{
    return _attr._GetStage()->_GetTimeSamplesInIntervalFromResolveInfo(
        _resolveInfo, _attr, interval, times);
}

/* static */
bool 
UsdAttributeQuery::GetUnionedTimeSamples(
    const std::vector<UsdAttributeQuery> &attrQueries, 
    std::vector<double> *times)
{
    return GetUnionedTimeSamplesInInterval(attrQueries, 
            GfInterval::GetFullInterval(), times);
}

/* static */
bool 
UsdAttributeQuery::GetUnionedTimeSamplesInInterval(
    const std::vector<UsdAttributeQuery> &attrQueries, 
    const GfInterval &interval,
    std::vector<double> *times)
{
    // Clear the vector first before proceeding to accumulate sample times.
    times->clear();

    if (attrQueries.empty()) {
        return true;
    }

    bool success = true;

    // Vector that holds the per-attribute sample times.
    std::vector<double> attrSampleTimes;

    // Temporary vector used to hold the union of two time-sample vectors.
    std::vector<double> tempUnionSampleTimes;

    for (const auto &attrQuery : attrQueries) {
        const UsdAttribute &attr = attrQuery.GetAttribute();
        if (!attr)
            continue;

        // This will work even if the attributes belong to different 
        // USD stages.
        success = attr.GetStage()->_GetTimeSamplesInIntervalFromResolveInfo(
                attrQuery._resolveInfo, attr, interval, &attrSampleTimes) 
                && success;

        // Merge attrSamplesTimes into the times vector.
        Usd_MergeTimeSamples(times, attrSampleTimes, &tempUnionSampleTimes);
    }
    
    return success;
}

size_t
UsdAttributeQuery::GetNumTimeSamples() const
{
    return _attr._GetStage()->_GetNumTimeSamplesFromResolveInfo(
        _resolveInfo, _attr);
}

bool 
UsdAttributeQuery::GetBracketingTimeSamples(double desiredTime, 
                                            double* lower, 
                                            double* upper, 
                                            bool* hasTimeSamples) const
{
    return _attr._GetStage()->_GetBracketingTimeSamplesFromResolveInfo(
        _resolveInfo, _attr, desiredTime, /* authoredOnly */ false,
        lower, upper, hasTimeSamples);
}

bool 
UsdAttributeQuery::HasValue() const
{
    return _resolveInfo._source != UsdResolveInfoSourceNone;  
}

bool 
UsdAttributeQuery::HasAuthoredValueOpinion() const
{
    return _resolveInfo.HasAuthoredValueOpinion();
}

bool 
UsdAttributeQuery::HasAuthoredValue() const
{
    return _resolveInfo.HasAuthoredValue();
}

bool
UsdAttributeQuery::HasFallbackValue() const
{
    return _attr.HasFallbackValue();
}

bool 
UsdAttributeQuery::ValueMightBeTimeVarying() const
{
    return _attr._GetStage()->_ValueMightBeTimeVaryingFromResolveInfo(
        _resolveInfo, _attr);
}

ARCH_PRAGMA_PUSH
ARCH_PRAGMA_INSTANTIATION_AFTER_SPECIALIZATION

// Explicitly instantiate templated getters for all Sdf value
// types.
#define _INSTANTIATE_GET(r, unused, elem)                               \
    template USD_API bool UsdAttributeQuery::_Get(                      \
        SDF_VALUE_CPP_TYPE(elem)*, UsdTimeCode) const;                  \
    template USD_API bool UsdAttributeQuery::_Get(                      \
        SDF_VALUE_CPP_ARRAY_TYPE(elem)*, UsdTimeCode) const;

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_GET, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_GET

ARCH_PRAGMA_POP

PXR_NAMESPACE_CLOSE_SCOPE

