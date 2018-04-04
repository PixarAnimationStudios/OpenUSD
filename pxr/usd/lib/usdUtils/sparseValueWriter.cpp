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

#include "pxr/usd/usdUtils/sparseValueWriter.h"

#include "pxr/base/gf/half.h"

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec4h.h"

#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

// Utilities for checking closeness of two values. In each case, _IsClose 
// returns true if the relative difference between the values is lower 
// than the fixed epsilon value. 

// XXX: We might want to expose these epsilon values via public API.

// The epsilon value used for comparing half values.
static const double _halfEpsilon = 1e-2;

// The epsilon value used for comparing float values.
static const double _floatEpsilon = 1e-6;

// The epsilon value used for comparing double values.
static const double _doubleEpsilon = 1e-12;

static bool
_IsClose(const double &a, const double &b)
{
    return GfIsClose(a, b, _doubleEpsilon);
}

static bool
_IsClose(const float &a, const float &b)
{
    return GfIsClose(a, b, _floatEpsilon);
}

static bool
_IsClose(const GfHalf &a, const GfHalf &b)
{
    return GfIsClose(a, b, _halfEpsilon);
}

template <
    typename MatrixType,
    typename std::enable_if<GfIsGfMatrix<MatrixType>::value>::type* = nullptr>
static bool
_IsClose(const MatrixType &a, const MatrixType &b)
{
    // XXX: is there a better way to tell if two matrices are within epsilon of 
    // each other?
    for (size_t i=0; i<MatrixType::numRows; ++i) {
        for (size_t j=0; j<MatrixType::numColumns; ++j) {            
            if (!_IsClose(a[i][j], b[i][j])) {
                return false;
            }
        }
    }
    return true;    
}

template <typename VectorType, 
    typename std::enable_if<GfIsGfVec<VectorType>::value>::type* = nullptr>
static bool
_IsClose(const VectorType &a, const VectorType &b)
{
    for (size_t i=0; i < VectorType::dimension; ++i) {
        if (!_IsClose(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

template <typename QuatType,
    typename std::enable_if<GfIsGfQuat<QuatType>::value>::type* = nullptr>
static bool
_IsClose(const QuatType &a, const QuatType &b)
{
    if (!_IsClose(a.GetReal(), b.GetReal())) {
        return false;
    }

    return _IsClose(a.GetImaginary(), b.GetImaginary());
}

template <typename T> 
static bool 
_IsClose(const VtArray<T> &a, const VtArray<T> &b)
{
    if (a.size() != b.size()) {
        return false;
    } 
        
    for (size_t i = 0; i < a.size(); ++i) {
        if (!_IsClose(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

#define CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, objType)                           \
    if (a.IsHolding<objType>() && b.IsHolding<objType>())                      \
        return _IsClose(a.UncheckedGet<objType>(), b.UncheckedGet<objType>());

bool 
_IsClose(const VtValue &a, const VtValue &b)
{
    // If either value is empty, return false.
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }

    // Checking for most common types early on to speed up the common cases.
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, double);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, float);

    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfMatrix4d);

    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec3d);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec4d);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec2d);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec3f);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec4f);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec2f);

    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtFloatArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtDoubleArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtMatrix4dArray);

    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec3fArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec3dArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec4fArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec4dArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec2fArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec2dArray);

    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfMatrix3d);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfMatrix2d);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtMatrix3dArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtMatrix2dArray);

    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfQuatd);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfQuatf);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtQuatdArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtQuatfArray);
    
    // Check for all the half types, which are rarely used.
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfHalf);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec2h);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec3h);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfVec4h);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtHalfArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec2hArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec3hArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtVec4hArray);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, GfQuath);
    CHECK_IS_HOLDING_AND_IS_CLOSE(a, b, VtQuathArray);

    // If we get here, it's not a floating-point type. Hence, check for equality.
    return a == b;
}

UsdUtilsSparseAttrValueWriter::UsdUtilsSparseAttrValueWriter(
    const UsdAttribute &attr, 
    const VtValue &defaultValue /* =VtValue() */) :
    _attr(attr)
{
    VtValue copyOfDefValue = defaultValue;
    _InitializeSparseAuthoring(&copyOfDefValue);
}

UsdUtilsSparseAttrValueWriter::UsdUtilsSparseAttrValueWriter(
    const UsdAttribute &attr, 
    VtValue *defaultValue) :
    _attr(attr)
{
    _InitializeSparseAuthoring(defaultValue);
}

void 
UsdUtilsSparseAttrValueWriter::_InitializeSparseAuthoring(
    VtValue *defaultValue)
{
    VtValue existingDefault;
    if (_attr.Get(&existingDefault, UsdTimeCode::Default())) {
        if (!defaultValue->IsEmpty() &&
            (existingDefault.IsEmpty() || 
                (!existingDefault.IsEmpty() && 
                !_IsClose(existingDefault, *defaultValue)))) {
            _attr.Set(*defaultValue);
        }
    } else if (!defaultValue->IsEmpty()) {
        _attr.Set(*defaultValue);
    }

    // If a non-empty default value was provided, then set _prevValue to it,
    // if not set it to the existing default value. This initializes the 
    // run-length encoding / sparse authoring scheme.
    if (!defaultValue->IsEmpty()) {
        _prevValue.Swap(*defaultValue);
    } else {
        _prevValue.Swap(existingDefault);
    }
}

bool
UsdUtilsSparseAttrValueWriter::SetTimeSample(
    const VtValue &value, 
    const UsdTimeCode time) 
{
    if (time.IsDefault()) {
        TF_CODING_ERROR("UsdUtilsSparseAttrValueWriter::SetTimeSample should "
            "not be called with time=Default.");
        return false;
    }

    if (_prevTime > time) {
        TF_CODING_ERROR("Time-samples should be set in sequentially "
                        "increasing order of time. Current time ( %s ) is "
                        "earlier than previous time ( %s )", 
                        TfStringify(time).c_str(),
                        TfStringify(_prevTime).c_str());
    }

    bool success = true;
    if (!_IsClose(_prevValue, value)) {
        if (!_didWritePrevValue) {
            // Write the extra timeSample to end the previous run.
            success = _attr.Set(_prevValue, _prevTime) && success;
        }
        success = _attr.Set(value, time) && success;

        _prevValue = value;

        // Setting _didWritePrevValue to true indicates the start of a new
        // run of similar values.
        _didWritePrevValue = true;
    } else {
        _didWritePrevValue = false;
    }

    // Update the time at which the next sample must be authored, 
    // if different from the current sample.
    _prevTime = time;

    return success;
}

bool
UsdUtilsSparseAttrValueWriter::SetTimeSample(
    VtValue *value, 
    const UsdTimeCode time) 
{
    if (time.IsDefault()) {
        TF_CODING_ERROR("UsdUtilsSparseAttrValueWriter::SetTimeSample should "
            "not be called with time=Default.");
        return false;
    }

    if (_prevTime > time) {
        TF_CODING_ERROR("Time-samples should be set in sequentially "
                        "increasing order of time. Current time ( %s ) is "
                        "earlier than previous time ( %s )", 
                        TfStringify(time).c_str(),
                        TfStringify(_prevTime).c_str());
    }

    bool success = true;
    if (!_IsClose(_prevValue, *value)) {
        if (!_didWritePrevValue) {
            // Write the extra timeSample to end the previous run.
            success = _attr.Set(_prevValue, _prevTime) && success;
        }
        success = _attr.Set(*value, time) && success;

        _prevValue.Swap(*value);

        // Setting _didWritePrevValue to true indicates the start of a new
        // run of similar values.
        _didWritePrevValue = true;
    } else {
        _didWritePrevValue = false;
    }

    // Update the time at which the next sample must be authored, 
    // if different from the current sample.
    _prevTime = time;

    return success;
}

bool
UsdUtilsSparseValueWriter::SetAttribute(
    const UsdAttribute &attr, 
    const VtValue &value, 
    const UsdTimeCode time /*=UsdTimeCode::Default()*/)
{
    return _SetAttributeImpl(attr, value, time);
}

bool 
UsdUtilsSparseValueWriter::SetAttribute(
    const UsdAttribute &attr, 
    VtValue *value, 
    const UsdTimeCode time /*=UsdTimeCode::Default()*/)
{
    return _SetAttributeImpl(attr, value, time);
}

template <typename T>
bool 
UsdUtilsSparseValueWriter::_SetAttributeImpl(
    const UsdAttribute &attr,
    T &value,
    const UsdTimeCode time)
{
    auto it = _attrValueWriterMap.find(attr);
    if (it == _attrValueWriterMap.end()) {
        if (time.IsDefault()) {
            _attrValueWriterMap.emplace(attr, 
                UsdUtilsSparseAttrValueWriter(attr, value));
            return true;
        } else {
            auto iterAndDidInsert = _attrValueWriterMap.emplace(attr, 
                    UsdUtilsSparseAttrValueWriter(attr));
            UsdUtilsSparseAttrValueWriter &vw = 
                    iterAndDidInsert.first->second;
            return vw.SetTimeSample(value, time);
        }
    } else {
        return it->second.SetTimeSample(value, time);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
