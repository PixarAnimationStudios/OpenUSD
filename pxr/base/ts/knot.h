//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_KNOT_H
#define PXR_BASE_TS_KNOT_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/knotData.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/type.h"

#include <string>
#include <memory>
#include <iosfwd>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE


/// A knot belonging to a TsSpline.
///
/// This class is non-templated, but can hold data for varying value types
/// (double, float, and half).  All knots in a spline must have the same value
/// type.
///
/// \sa TsTypedKnot
///
class TsKnot
{
public:
    /// \name Construction and value semantics
    ///
    /// Unlike splines, knots have a fixed value type from the time they are
    /// constructed.
    ///
    /// Knots are fairly small, so copying them is not particularly expensive.
    /// Move construction and assignment are supported, but this is only
    /// beneficial when there is custom data.
    ///
    /// @{

    /// Default constructor creates a double-typed knot.
    TS_API
    TsKnot();

    /// Creates a knot with a specified value type.
    TS_API
    TsKnot(
        TfType valueType,
        TsCurveType curveType = TsCurveTypeBezier);

    TS_API
    TsKnot(const TsKnot &other);

    TS_API
    TsKnot(TsKnot &&other);

    TS_API
    ~TsKnot();

    TS_API
    TsKnot& operator=(const TsKnot &other);

    TS_API
    TsKnot& operator=(TsKnot &&other);

    TS_API
    bool operator==(const TsKnot &other) const;

    TS_API
    bool operator!=(const TsKnot &other) const;

    /// @}
    /// \name Knot time
    /// @{

    TS_API
    bool SetTime(
        TsTime time);

    TS_API
    TsTime GetTime() const;

    /// @}
    /// \name Interpolation mode
    /// @{

    /// Sets the interpolation mode of the spline segment following this knot.
    TS_API
    bool SetNextInterpolation(TsInterpMode mode);

    TS_API
    TsInterpMode GetNextInterpolation() const;

    /// @}
    /// \name Knot value
    /// @{

    TS_API
    TfType GetValueType() const;

    template <typename T>
    bool IsHolding() const;

    TS_API
    bool SetValue(
        VtValue value);

    template <typename T>
    bool SetValue(
        const T value);

    TS_API
    bool GetValue(
        VtValue *valueOut) const;

    template <typename T>
    bool GetValue(
        T *valueOut) const;

    /// @}
    /// \name Dual values
    /// @{

    TS_API
    bool IsDualValued() const;

    TS_API
    bool SetPreValue(
        VtValue value);

    template <typename T>
    bool SetPreValue(
        const T value);

    TS_API
    bool GetPreValue(
        VtValue *valueOut) const;

    template <typename T>
    bool GetPreValue(
        T *valueOut) const;

    TS_API
    bool ClearPreValue();

    /// @}
    /// \name Curve type
    ///
    /// Each knot's curve type must match the curve type of the spline to which
    /// it belongs.  Knot objects are Bezier by default.
    ///
    /// In a Hermite spline, tangent widths are determined automatically.  They
    /// are always one-third of the width of the segment to which they belong.
    ///
    /// @{

    TS_API
    bool SetCurveType(TsCurveType curveType);

    TS_API
    TsCurveType GetCurveType() const;

    /// @}
    /// \name Pre-tangent
    ///
    /// Tangents are expressed as width and slope.
    ///
    /// It is an error to read or write widths for Hermite knots.
    ///
    /// \note Note that Maya uses tangents in a different format.
    /// \ref TsConvertFromStandardTangent is a utility function that can
    /// convert a standard width and slope to values expected by Maya.
    ///
    /// @{

    TS_API
    bool SetPreTanWidth(TsTime width);

    TS_API
    TsTime GetPreTanWidth() const;

    TS_API
    bool SetPreTanSlope(VtValue slope);

    template <typename T>
    bool SetPreTanSlope(T slope);

    TS_API
    bool GetPreTanSlope(VtValue *slopeOut) const;

    template <typename T>
    bool GetPreTanSlope(T *slopeOut) const;

    /// @}
    /// \name Post-tangent
    /// @{

    TS_API
    bool SetPostTanWidth(TsTime width);

    TS_API
    TsTime GetPostTanWidth() const;

    TS_API
    bool SetPostTanSlope(VtValue slope);

    template <typename T>
    bool SetPostTanSlope(T slope);

    TS_API
    bool GetPostTanSlope(VtValue *slopeOut) const;

    template <typename T>
    bool GetPostTanSlope(T *slopeOut) const;

    /// @}
    /// \name Custom data
    ///
    /// Knots may have custom data: an arbitrary VtDictionary of key/value
    /// pairs.
    ///
    /// Custom data does not affect evaluation.  It is only for clients' use.
    ///
    /// When knots are edited, Ts does not automatically alter any custom data.
    /// This means that, if splines are written out, edited by other clients
    /// that do not recognize a particular kind of custom data, and read back
    /// in, then that custom data may become outdated.
    ///
    /// @{

    TS_API
    bool SetCustomData(
        VtDictionary customData);

    TS_API
    VtDictionary GetCustomData() const;

    TS_API
    bool SetCustomDataByKey(
        const std::string &keyPath,
        VtValue value);

    TS_API
    VtValue GetCustomDataByKey(
        const std::string &keyPath) const;

    /// @}
    /// \name Continuity queries
    /// @{

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsC0Continuous() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsG1Continuous() const;

    /// <b>Not yet implemented.</b>
    TS_API
    bool IsC1Continuous() const;

    /// @}

protected:
    friend class TsSpline;
    friend class TsKnotMap;
    friend class TsRegressionPreventer;

    // Constructor for copying knot data from SplineData.  The data has been
    // copied for us, and we take ownership of it.
    TsKnot(
        Ts_KnotData *data,
        TfType valueType,
        VtDictionary &&customData);

    // Accessors for low-level knot data.
    Ts_KnotData* _GetData() { return _data; }
    const Ts_KnotData* _GetData() const { return _data; }

private:
    template <typename T>
    bool _CheckInParam(T value) const;

    template <typename T>
    bool _CheckOutParam(T *valueOut) const;

    bool _CheckGetWidth() const;
    bool _CheckSetWidth(TsTime width) const;
    bool _CheckInParamVt(VtValue value) const;
    bool _CheckOutParamVt(VtValue* value) const;

    template <typename T>
    Ts_TypedKnotData<T>* _TypedData() const;

    template <typename T>
    const Ts_TypedKnotData<T>* _ConstTypedData() const;

private:
    // Main knot fields.  Never null.  The data is on the heap, and we own it
    // exclusively, but we don't use unique_ptr because we need to deallocate in
    // a type-aware way; see the destructor.
    Ts_KnotData* _data;

    // Proxy object, for typed data access.  Never null.
    //
    // XXX: it would be possible to eliminate this member by encoding value type
    // in a bitfield in Ts_KnotData; there are only three possible value types,
    // and there are plenty of unused bits in the Ts_KnotData padding.  Then we
    // could create proxy objects on the fly, or make them singletons.
    //
    std::unique_ptr<Ts_KnotDataProxy> _proxy;

    // Custom data.  Optional; may be empty.
    VtDictionary _customData;
};

/// Output a text representation of a spline to a stream.
TS_API
std::ostream& operator<<(std::ostream& out, const TsKnot &knot);


/// A convenience for constructing knots with specified types.
///
/// Instead of writing:
///
/// <pre>
/// TsKnot knot1(TfType::Find<double>());
/// TsKnot knot2(TfType::Find<float>());
/// TsKnot knot3(TfType::Find<GfHalf>());
/// </pre>
///
/// One may write:
///
/// <pre>
/// TsDoubleKnot knot1;
/// TsFloatKnot knot2;
/// TsHalfKnot knot3;
/// </pre>
///
template <typename T,
          typename = std::enable_if_t<Ts_IsSupportedValueType<T>::value>>
class TsTypedKnot : public TsKnot
{
public:
    TsTypedKnot() : TsKnot(Ts_GetType<T>()) {}
};

/// \class TsDoubleKnot
/// A knot-construction convenience.  See TsTypedKnot.
using TsDoubleKnot = TsTypedKnot<double>;

/// \class TsFloatKnot
/// A knot-construction convenience.  See TsTypedKnot.
using TsFloatKnot = TsTypedKnot<float>;

/// \class TsHalfKnot
/// A knot-construction convenience.  See TsTypedKnot.
using TsHalfKnot = TsTypedKnot<GfHalf>;

// Make sure we have coverage for all allowed types.
#define _MAKE_CLAUSE(unused, tuple)                                          \
    static_assert(std::is_same_v<TF_PP_CAT(TF_PP_CAT(Ts,                     \
                    TS_SPLINE_VALUE_TYPE_NAME(tuple)), Knot),                \
                  TsTypedKnot<TS_SPLINE_VALUE_CPP_TYPE(tuple)>>,             \
                  "Incorrect type alias for TsKnot type: " #tuple);
TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES)
#undef _MAKE_CLAUSE



////////////////////////////////////////////////////////////////////////////////
// TEMPLATE HELPERS

template <typename T>
bool TsKnot::_CheckInParam(const T value) const
{
    if constexpr (!Ts_IsSupportedValueType<T>::value)
    {
        static_assert(Ts_IsSupportedValueType<T>::value,
            "Cannot pass non-floating-point type as T-typed knot parameter");
        return false;
    }
    else
    {
        if (GetValueType() != Ts_GetType<T>())
        {
            TF_CODING_ERROR(
                "Cannot set '%s' value into knot of type '%s'",
                Ts_GetType<T>().GetTypeName().c_str(),
                GetValueType().GetTypeName().c_str());
            return false;
        }

        if (!Ts_IsFinite(value))
        {
            TF_CODING_ERROR("Set values must be finite.");
            return false;
        }

        return true;
    }
}

template <typename T>
bool TsKnot::_CheckOutParam(T *valueOut) const
{
    if constexpr (!Ts_IsSupportedValueType<T>::value)
    {
        static_assert(Ts_IsSupportedValueType<T>::value,
            "Cannot pass non-floating-point type as T-typed knot parameter");
        return false;
    }
    else
    {
        if (!valueOut)
        {
            TF_CODING_ERROR("Null pointer");
            return false;
        }

        if (GetValueType() != Ts_GetType<T>())
        {
            TF_CODING_ERROR(
                "Cannot read from knot of type '%s' into '%s'",
                GetValueType().GetTypeName().c_str(),
                Ts_GetType<T>().GetTypeName().c_str());
            return false;
        }

        return true;
    }
}

template <typename T>
Ts_TypedKnotData<T>*
TsKnot::_TypedData() const
{
    return static_cast<Ts_TypedKnotData<T>*>(_data);
}

template <typename T>
const Ts_TypedKnotData<T>*
TsKnot::_ConstTypedData() const
{
    return static_cast<const Ts_TypedKnotData<T>*>(_data);
}

////////////////////////////////////////////////////////////////////////////////
// TEMPLATE IMPLEMENTATIONS

template <typename T>
bool TsKnot::IsHolding() const
{
    return GetValueType() == Ts_GetType<T>();
}

template <typename T>
bool TsKnot::SetValue(const T value)
{
    if (!_CheckInParam(value))
    {
        return false;
    }

    _TypedData<T>()->value = value;
    return true;
}

template <typename T>
bool TsKnot::GetValue(T *valueOut) const
{
    if (!_CheckOutParam(valueOut))
    {
        return false;
    }

    *valueOut = _ConstTypedData<T>()->value;
    return true;
}

template <typename T>
bool TsKnot::SetPreValue(const T value)
{
    if (!_CheckInParam(value))
    {
        return false;
    }

    _data->dualValued = true;
    _TypedData<T>()->preValue = value;
    return true;
}

template <typename T>
bool TsKnot::GetPreValue(T* const valueOut) const
{
    if (!_CheckOutParam(valueOut))
    {
        return false;
    }

    if (_data->dualValued)
    {
        *valueOut = _ConstTypedData<T>()->preValue;
    }
    else
    {
        *valueOut = _ConstTypedData<T>()->value;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Pre-Tangent

template <typename T>
bool TsKnot::SetPreTanSlope(const T slope)
{
    if (!_CheckInParam(slope))
    {
        return false;
    }

    _TypedData<T>()->preTanSlope = slope;
    return true;
}

template <typename T>
bool TsKnot::GetPreTanSlope(T* const slopeOut) const
{
    if (!_CheckOutParam(slopeOut))
    {
        return false;
    }

    *slopeOut = _ConstTypedData<T>()->GetPreTanSlope();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Post-Tangent

template <typename T>
bool TsKnot::SetPostTanSlope(const T slope)
{
    if (!_CheckInParam(slope))
    {
        return false;
    }

    _TypedData<T>()->postTanSlope = slope;
    return true;
}

template <typename T>
bool TsKnot::GetPostTanSlope(T* const slopeOut) const
{
    if (!_CheckOutParam(slopeOut))
    {
        return false;
    }

    *slopeOut = _ConstTypedData<T>()->GetPostTanSlope();
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif
