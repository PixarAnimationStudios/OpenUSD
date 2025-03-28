//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_INTERPOLATION_H
#define PXR_USD_USD_INTERPOLATION_H

/// \file usd/interpolation.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/declare.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \enum UsdInterpolationType
///
/// Attribute value interpolation options. 
///
/// See \ref Usd_AttributeInterpolation for more details.
///
enum UsdInterpolationType
{
    UsdInterpolationTypeHeld,  ///< Held interpolation
    UsdInterpolationTypeLinear ///< Linear interpolation
};

/// \anchor USD_LINEAR_INTERPOLATION_TYPES
/// Sequence of value types that support linear interpolation.
/// These types and VtArrays of these types are supported:
/// \li <b>GfHalf</b>
/// \li <b>float</b>
/// \li <b>double</b>
/// \li <b>SdfTimeCode</b>
/// \li <b>GfMatrix2d</b>
/// \li <b>GfMatrix3d</b>
/// \li <b>GfMatrix4d</b>
/// \li <b>GfVec2d</b>
/// \li <b>GfVec2f</b>
/// \li <b>GfVec2h</b>
/// \li <b>GfVec3d</b>
/// \li <b>GfVec3f</b>
/// \li <b>GfVec3h</b>
/// \li <b>GfVec4d</b>
/// \li <b>GfVec4f</b>
/// \li <b>GfVec4h</b>
/// \li <b>GfQuatd</b> (via quaternion slerp)
/// \li <b>GfQuatf</b> (via quaternion slerp)
/// \li <b>GfQuath</b> (via quaternion slerp)
/// \hideinitializer
#define USD_LINEAR_INTERPOLATION_TYPES               \
    (GfHalf) (VtArray<GfHalf>)                       \
    (float) (VtArray<float>)                         \
    (double) (VtArray<double>)                       \
    (SdfTimeCode) (VtArray<SdfTimeCode>)             \
    (GfMatrix2d) (VtArray<GfMatrix2d>)               \
    (GfMatrix3d) (VtArray<GfMatrix3d>)               \
    (GfMatrix4d) (VtArray<GfMatrix4d>)               \
    (GfVec2d) (VtArray<GfVec2d>)                     \
    (GfVec2f) (VtArray<GfVec2f>)                     \
    (GfVec2h) (VtArray<GfVec2h>)                     \
    (GfVec3d) (VtArray<GfVec3d>)                     \
    (GfVec3f) (VtArray<GfVec3f>)                     \
    (GfVec3h) (VtArray<GfVec3h>)                     \
    (GfVec4d) (VtArray<GfVec4d>)                     \
    (GfVec4f) (VtArray<GfVec4f>)                     \
    (GfVec4h) (VtArray<GfVec4h>)                     \
    (GfQuatd) (VtArray<GfQuatd>)                     \
    (GfQuatf) (VtArray<GfQuatf>)                     \
    (GfQuath) (VtArray<GfQuath>)

/// \struct UsdLinearInterpolationTraits
///
/// Traits class describing whether a particular C++ value type
/// supports linear interpolation.
///
/// UsdLinearInterpolationTraits<T>::isSupported will be true for all
/// types listed in the USD_LINEAR_INTERPOLATION_TYPES sequence.
template <class T>
struct UsdLinearInterpolationTraits
{
    static const bool isSupported = false;
};

/// \cond INTERNAL
#define _USD_DECLARE_INTERPOLATION_TRAITS(unused, type)         \
template <>                                                     \
struct UsdLinearInterpolationTraits<type>                       \
{                                                               \
    static const bool isSupported = true;                       \
};

TF_PP_SEQ_FOR_EACH(_USD_DECLARE_INTERPOLATION_TRAITS, ~,
                   USD_LINEAR_INTERPOLATION_TYPES)

#undef _USD_DECLARE_INTERPOLATION_TRAITS
/// \endcond


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INTERPOLATION_H
