//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/interpolators.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_OPEN_SCOPE

bool 
Usd_UntypedInterpolator::Interpolate(
    const SdfLayerRefPtr& layer, const SdfPath& path,
    double time, double lower, double upper)
{
    return _Interpolate(layer, path, time, lower, upper);
}

bool 
Usd_UntypedInterpolator::Interpolate(
    const Usd_ClipSetRefPtr& clipSet, const SdfPath& path,
    double time, double lower, double upper)
{
    return _Interpolate(clipSet, path, time, lower, upper);
}

template <class Src>
bool 
Usd_UntypedInterpolator::_Interpolate(
    const Src& src, const SdfPath& path,
    double time, double lower, double upper)
{
    if (_attr.GetStage()->GetInterpolationType() == UsdInterpolationTypeHeld) {
        return Usd_HeldInterpolator<VtValue>(_result).Interpolate(
            src, path, time, lower, upper);
    }

    // Since we're working with type-erased objects, we have no
    // choice but to do a series of runtime type checks to determine 
    // what kind of interpolation is supported for the attribute's
    // value.

    const TfType attrValueType = _attr.GetTypeName().GetType();
    if (!attrValueType) {
        TF_RUNTIME_ERROR(
            "Unknown value type '%s' for attribute '%s'",
            _attr.GetTypeName().GetAsToken().GetText(),
            _attr.GetPath().GetString().c_str());
        return false;
    }

#define _MAKE_CLAUSE(unused, type)                                      \
    {                                                                   \
        static const TfType valueType = TfType::Find<type>();           \
        if (attrValueType == valueType) {                               \
            type result;                                                \
            if (Usd_LinearInterpolator<type>(&result).Interpolate(      \
                    src, path, time, lower, upper)) {                   \
                *_result = result;                                      \
                return true;                                            \
            }                                                           \
            return false;                                               \
        }                                                               \
    }

    TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, USD_LINEAR_INTERPOLATION_TYPES)
#undef _MAKE_CLAUSE

    return Usd_HeldInterpolator<VtValue>(_result).Interpolate(
        src, path, time, lower, upper);
}

PXR_NAMESPACE_CLOSE_SCOPE

