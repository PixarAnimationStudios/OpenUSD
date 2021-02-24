//
// Copyright 2018 Pixar
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
_Resample(_TypeList<>, float alpha, const VtValue& v0, const VtValue& v1,
          const TfType& valueType, VtValue* result)
{
    // If the VtValue's don't contain any of the types that can be
    // interpolated, just hold the preceding time sample's value.
    *result = (alpha < 1.f) ? v0 : v1;
}

template <typename T, typename... U>
void
_Resample(_TypeList<T, U...>, float alpha, const VtValue& v0,
          const VtValue& v1, const TfType& valueType, VtValue* result)
{
    // A VtValue containing T or a VtArray<T> is supported.
    if (_TryResample<T>(alpha, v0, v1, valueType, result)) {
        return;
    }
    if (_TryResample<VtArray<T>>(alpha, v0, v1, valueType, result)) {
        return;
    }

    _Resample(_TypeList<U...>(), alpha, v0, v1, valueType, result);
}

} // namespace

VtValue
HdResampleNeighbors(float alpha, const VtValue& v0, const VtValue& v1)
{
    // After verifying that the values have matching types, return the result
    // of HdResampleNeighbors() for the enclosed values.
    const TfType t0 = v0.GetType();
    if (!t0) {
        TF_RUNTIME_ERROR("Unknown sample value type '%s'",
                         v0.GetTypeName().c_str());
        return v0;
    }

    const TfType t1 = v1.GetType();
    if (t0 != t1) {
        TF_RUNTIME_ERROR("Mismatched sample value types '%s' and '%s'",
                         v0.GetTypeName().c_str(), v1.GetTypeName().c_str());
        return v0;
    }

    // The list of supported types to interpolate.
    using _InterpTypes =
        _TypeList<float, double, GfHalf, GfMatrix2d, GfMatrix3d, GfMatrix4d,
                  GfVec2d, GfVec2f, GfVec2h, GfVec3d, GfVec3f, GfVec3h, GfVec4d,
                  GfVec4f, GfVec4h, GfQuatd, GfQuatf, GfQuath>;

    VtValue result;
    _Resample(_InterpTypes(), alpha, v0, v1, t0, &result);
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
