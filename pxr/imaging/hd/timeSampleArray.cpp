//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/timeSampleArray.h"

#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
/// Returns the result of HdResampleNeighbors() for the enclosed values, if
/// they are of type T.
template <typename T>
bool
_TryResample(float alpha, const VtValue& v0, const VtValue& v1,
             const TfType& valueType, VtValue* result)
{
    static const TfType type = TfType::Find<T>();

    if (valueType == type) {
        const T& val0 = v0.Get<T>();
        const T& val1 = v1.Get<T>();
        *result = VtValue(HdResampleNeighbors(alpha, val0, val1));
        return true;
    }

    return false;
}

template <typename... T>
struct _TypeList {};

void
_Resample(float alpha, const VtValue& v0, const VtValue& v1,
          const TfType& valueType, VtValue* result, _TypeList<>)
{
    // If the VtValue's don't contain any of the types that can be
    // interpolated, just hold the preceding time sample's value.
    *result = (alpha < 1.f) ? v0 : v1;
}

template <typename T, typename... U>
void
_Resample(float alpha, const VtValue& v0, const VtValue& v1,
          const TfType& valueType, VtValue* result, _TypeList<T, U...>)
{
    // A VtValue containing T or a VtArray<T> is supported.
    if (_TryResample<T>(alpha, v0, v1, valueType, result)) {
        return;
    }
    if (_TryResample<VtArray<T>>(alpha, v0, v1, valueType, result)) {
        return;
    }

    _Resample(alpha, v0, v1, valueType, result, _TypeList<U...>());
}

} // namespace

VtValue
HdResampleNeighbors(float alpha, const VtValue& v0, const VtValue& v1)
{
    // After verifying that the values have matching types, return the result
    // of HdResampleNeighbors() for the enclosed values.
    const TfType t0 = v0.GetType();
    if (!t0) {
        TF_CODING_ERROR("Unknown sample value type '%s'",
                         v0.GetTypeName().c_str());
        return v0;
    }

    const TfType t1 = v1.GetType();
    if (t0 != t1) {
        TF_CODING_ERROR("Mismatched sample value types '%s' and '%s'",
                         v0.GetTypeName().c_str(), v1.GetTypeName().c_str());
        return v0;
    }

    // The list of supported types to interpolate.
    using _InterpTypes =
        _TypeList<float, double, GfHalf, GfMatrix2d, GfMatrix3d, GfMatrix4d,
                  GfVec2d, GfVec2f, GfVec2h, GfVec3d, GfVec3f, GfVec3h, GfVec4d,
                  GfVec4f, GfVec4h, GfQuatd, GfQuatf, GfQuath>;

    VtValue result;
    _Resample(alpha, v0, v1, t0, &result, _InterpTypes());
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
