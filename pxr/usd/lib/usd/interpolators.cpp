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
#include "pxr/usd/usd/interpolators.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/usd/stage.h"

#include <boost/preprocessor/seq/for_each.hpp>

bool 
Usd_UntypedInterpolator::Interpolate(
    const UsdAttribute& attr, 
    const SdfLayerRefPtr& layer, const SdfAbstractDataSpecId& specId,
    double time, double lower, double upper)
{
    return _Interpolate(attr, layer, specId, time, lower, upper);
}

bool 
Usd_UntypedInterpolator::Interpolate(
    const UsdAttribute& attr, 
    const Usd_ClipRefPtr& clip, const SdfAbstractDataSpecId& specId,
    double time, double lower, double upper)
{
    return _Interpolate(attr, clip, specId, time, lower, upper);
}

template <class Src>
bool 
Usd_UntypedInterpolator::_Interpolate(
    const UsdAttribute& attr, 
    const Src& src, const SdfAbstractDataSpecId& specId,
    double time, double lower, double upper)
{
    if (attr.GetStage()->GetInterpolationType() == UsdInterpolationTypeHeld) {
        return Usd_HeldInterpolator<VtValue>(_result).Interpolate(
            attr, src, specId, time, lower, upper);
    }

    // Since we're working with type-erased objects, we have no
    // choice but to do a series of runtime type checks to determine 
    // what kind of interpolation is supported for the attribute's
    // value.

    const TfType attrValueType = attr.GetTypeName().GetType();
    if (!attrValueType) {
        TF_RUNTIME_ERROR(
            "Unknown value type '%s' for attribute '%s'",
            attr.GetTypeName().GetAsToken().GetText(),
            attr.GetPath().GetString().c_str());
        return false;
    }

#define _MAKE_CLAUSE(r, unused, type)                                   \
    {                                                                   \
        static const TfType valueType = TfType::Find<type>();           \
        if (attrValueType == valueType) {                               \
            type result;                                                \
            if (Usd_LinearInterpolator<type>(&result).Interpolate(      \
                    attr, src, specId, time, lower, upper)) {           \
                *_result = result;                                      \
                return true;                                            \
            }                                                           \
            return false;                                               \
        }                                                               \
    }

    BOOST_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, USD_LINEAR_INTERPOLATION_TYPES)
#undef _MAKE_CLAUSE

    return Usd_HeldInterpolator<VtValue>(_result).Interpolate(
        attr, src, specId, time, lower, upper);
}
