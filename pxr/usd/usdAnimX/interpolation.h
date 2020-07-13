//
// Copyright 2020 benmalartre
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
#ifndef PXR_USD_PLUGIN_ANIMX_INTERPOLATION_H
#define PXR_USD_PLUGIN_ANIMX_INTERPOLATION_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/usd/usdAnimX/curve.h"

#include <boost/preprocessor/seq/for_each.hpp>

#include <vector>
#include <iostream>


PXR_NAMESPACE_OPEN_SCOPE

/// \anchor USD_ANIMX_INTERPOLATION_TYPES
/// Sequence of value types that support animx interpolation.
/// These types are supported:
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
#define USD_ANIMX_INTERPOLATION_TYPES   \
    (GfHalf)                            \
    (float)                             \
    (double)                            \
    (SdfTimeCode)                       \
    (GfMatrix2d)                        \
    (GfMatrix3d)                        \
    (GfMatrix4d)                        \
    (GfVec2d)                           \
    (GfVec2f)                           \
    (GfVec2h)                           \
    (GfVec3d)                           \
    (GfVec3f)                           \
    (GfVec3h)                           \
    (GfVec4d)                           \
    (GfVec4f)                           \
    (GfVec4h)                           \
    (GfQuatd)                           \
    (GfQuatf)                           \
    (GfQuath) 

/// \struct UsdAnimXSupportedTraits
///
/// Traits class describing whether a particular C++ value type
/// supports animx interpolation.
///
/// UsdAnimXSupportedTraits<T>::isSupported will be true for all
/// types listed in the USD_ANIMX_SUPPORTED_TYPES sequence.
template <class T>
struct UsdAnimXSupportedTraits
{
    static const bool isSupported = false;
};

/// \cond INTERNAL
#define _USD_ANIMX_DECLARE_INTERPOLATION_TRAITS(r, unused, type)    \
template <>                                                         \
struct UsdAnimXSupportedTraits<type>                                \
{                                                                   \
    static const bool isSupported = true;                           \
};

BOOST_PP_SEQ_FOR_EACH(_USD_ANIMX_DECLARE_INTERPOLATION_TRAITS, ~, 
                      USD_ANIMX_INTERPOLATION_TYPES)

#undef _USD_ANIMX_DECLARE_INTERPOLATION_TRAITS
/// \endcond

// interpolate prototype
typedef bool(*InterpolateFunc)(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n);

template<typename T>
static 
inline void
_Interpolate(const UsdAnimXCurve* curves, T* value, double time)
{
    *value = curves[0].evaluate(time);
}

template<typename T>
static 
inline void
_Interpolate2(const UsdAnimXCurve* curves, T* value, double time)
{
    (*value)[0] = curves[0].evaluate(time);
    (*value)[1] = curves[1].evaluate(time);
}

template<typename T>
static 
inline void
_Interpolate3(const UsdAnimXCurve* curves, T* value, double time)
{
    (*value)[0] = curves[0].evaluate(time);
    (*value)[1] = curves[1].evaluate(time);
    (*value)[2] = curves[2].evaluate(time);
}

template<typename T>
static 
inline void
_Interpolate4(const UsdAnimXCurve* curves, T* value, double time)
{
    (*value)[0] = curves[0].evaluate(time);
    (*value)[1] = curves[1].evaluate(time);
    (*value)[2] = curves[2].evaluate(time);
    (*value)[3] = curves[3].evaluate(time);
}

template<typename T>
static
inline void
_InterpolateQuat(const UsdAnimXCurve* curves, T* value, double time)
{
    /*
    adsk::Quaternion q = adsk::evaluateQuaternionCurve(
        time,
        curves[0], curves[1], curves[2],
        interpolationMethod);
    *value = (T)(*(GfQuatD*)&q);
    */
}

static
bool 
UsdAnimXInterpolateBool(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size()!=1)return false;
    bool v;
    _Interpolate<bool>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateInt(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size()!=1)return false;
    int v;
    _Interpolate<int>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateHalf(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size()!=1)return false;
    GfHalf v;
    _Interpolate<GfHalf>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateFloat(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=1)return false;
    float v;
    _Interpolate<float>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateDouble(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=1)return false;
    double v;
    _Interpolate<double>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateTimeCode(const std::vector<UsdAnimXCurve>& curves,
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=1)return false;
    SdfTimeCode v;
    _Interpolate<SdfTimeCode>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateMatrix2d(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateMatrix3d(const std::vector<UsdAnimXCurve>& curves,
    VtValue* value, double time, size_t n=0)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateMatrix4d(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateVector2d(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=2)return false;
    GfVec2d v;
    _Interpolate2<GfVec2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector2f(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=2)return false;
    GfVec2f v;
    _Interpolate2<GfVec2f>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector2h(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=2)return false;
    GfVec2h v;
    _Interpolate2<GfVec2h>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector3d(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=3)return false;
    GfVec3d v;
    _Interpolate3<GfVec3d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector3f(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=3)return false;
    GfVec3f v;
    _Interpolate3<GfVec3f>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector3h(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=3)return false;
    GfVec3h v;
    _Interpolate3<GfVec3h>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector4d(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=4)return false;
    GfVec4d v;
    _Interpolate4<GfVec4d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector4f(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=4)return false;
    GfVec4f v;
    _Interpolate4<GfVec4f>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateVector4h(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=4)return false;
    GfVec4h v;
    _Interpolate4<GfVec4h>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateQuatd(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=4)return false;
    GfQuatd v;
    _InterpolateQuat<GfQuatd>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateQuatf(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=4)return false;
    GfQuatf v;
    _InterpolateQuat<GfQuatf>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

static
bool 
UsdAnimXInterpolateQuath(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=0)
{
    if(curves.size()!=4)return false;
    GfQuath v;
    _InterpolateQuat<GfQuath>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
}

/// Arrays
static
bool 
UsdAnimXInterpolateHalfArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size()!=n)return false;
    VtArray<GfHalf> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate<GfHalf>(&curves[i], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateFloatArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size()!=n)return false;
    VtArray<float> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate<float>(&curves[i], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateDoubleArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size()!=n)return false;
    VtArray<double> array(n);
    for(size_t i=0;i<n;++i) {
      _Interpolate<double>(&curves[i], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateTimeCodeArray(const std::vector<UsdAnimXCurve>& curves,
    VtValue* value, double time, size_t n=1)
{
    if(curves.size()!=n)return false;
    VtArray<SdfTimeCode> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate<SdfTimeCode>(&curves[i], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateMatrix2dArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateMatrix3dArray(const std::vector<UsdAnimXCurve>& curves,
    VtValue* value, double time, size_t n=1)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateMatrix4dArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    /*
    if(curves.size()!=1)return false;
    GfMatrix2d v;
    _Interpolate4<GfMatrix2d>(&curves[0], &v, time);
    *value = VtValue(v);
    return true;
    */
    return false;
}

static
bool 
UsdAnimXInterpolateVector2dArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 2*n)return false;
    VtArray<GfVec2d> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate2<GfVec2d>(&curves[i*2], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector2fArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 2*n)return false;
    VtArray<GfVec2f> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate2<GfVec2f>(&curves[i*2], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector2hArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 2*n)return false;
    VtArray<GfVec2h> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate2<GfVec2h>(&curves[i*2], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector3dArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 3*n)return false;
    VtArray<GfVec3d> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate3<GfVec3d>(&curves[i*3], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector3fArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 3*n)return false;
    VtArray<GfVec3f> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate3<GfVec3f>(&curves[i*3], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector3hArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 3*n)return false;
    VtArray<GfVec3h> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate3<GfVec3h>(&curves[i*3], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector4dArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 4*n)return false;
    VtArray<GfVec4d> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate4<GfVec4d>(&curves[i*4], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector4fArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 4*n)return false;
    VtArray<GfVec4f> array(n);
    for(size_t i=0;i<n;++i) {
      _Interpolate4<GfVec4f>(&curves[i*4], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateVector4hArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 4*n)return false;
    VtArray<GfVec4h> array(n);
    for(size_t i=0;i<n;++i) {
        _Interpolate4<GfVec4h>(&curves[i*4], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateQuatdArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 4*n)return false;
    VtArray<GfQuatd> array(n);
    for(size_t i=0;i<n;++i) {
        _InterpolateQuat<GfQuatd>(&curves[i*4], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateQuatfArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 4*n)return false;
    VtArray<GfQuatf> array(n);
    for(size_t i=0;i<n;++i) {
        _InterpolateQuat<GfQuatf>(&curves[i*4], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

static
bool 
UsdAnimXInterpolateQuathArray(const std::vector<UsdAnimXCurve>& curves, 
    VtValue* value, double time, size_t n=1)
{
    if(curves.size() != 4*n)return false;
    VtArray<GfQuath> array(n);
    for(size_t i=0;i<n;++i) {
        _InterpolateQuat<GfQuath>(&curves[i*4], &array[i], time);
    }
    *value = VtValue(array);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_ANIMX_INTERPOLATION_H
