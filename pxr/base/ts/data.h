//
// Copyright 2023 Pixar
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

#ifndef PXR_BASE_TS_DATA_H
#define PXR_BASE_TS_DATA_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/ts/evalCache.h"
#include "pxr/base/ts/mathUtils.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/vt/value.h"

#include <string>
#include <math.h>

PXR_NAMESPACE_OPEN_SCOPE

class Ts_PolymorphicDataHolder;

/// \class Ts_Data 
/// \brief Holds the data for an TsKeyFrame.
///
/// \c Ts_Data is an interface for holding \c TsKeyFrame data.
///
class Ts_Data {
public:
    virtual ~Ts_Data() = default;
    virtual void CloneInto(Ts_PolymorphicDataHolder *holder) const = 0;
    
    // Create and return an EvalCache that represents the spline segment from
    // this keyframe to kf2.
    virtual std::shared_ptr<Ts_UntypedEvalCache> CreateEvalCache(
        Ts_Data const* kf2) const = 0;

    // Evaluate between this keyframe data and \p kf2 at \p time.  This is
    // useful for callers that do not otherwise want or need to create/retain an
    // eval cache.
    virtual VtValue
    EvalUncached(Ts_Data const *kf2, TsTime time) const = 0;

    // Evaluate the derivative between this keyframe data and \p kf2 at \p time.
    // This is useful for callers that do not otherwise want or need to
    // create/retain an eval cache.
    virtual VtValue
    EvalDerivativeUncached(Ts_Data const *kf2, TsTime time) const = 0;

    virtual bool operator==(const Ts_Data &) const = 0;

    // Time
    inline TsTime GetTime() const {
        return _time;
    }
    inline void SetTime(TsTime newTime) {
        _time = newTime;
    }

    // Knot type
    virtual TsKnotType GetKnotType() const = 0;
    virtual void SetKnotType( TsKnotType knotType ) = 0;
    virtual bool CanSetKnotType( TsKnotType knotType,
                                 std::string *reason ) const = 0;

    // Values
    virtual VtValue GetValue() const = 0;
    virtual void SetValue( VtValue val ) = 0;
    virtual VtValue GetValueDerivative() const = 0;
    virtual bool GetIsDualValued() const = 0;
    virtual void SetIsDualValued( bool isDual ) = 0;
    virtual VtValue GetLeftValue() const = 0;
    virtual VtValue GetLeftValueDerivative() const = 0;
    virtual void SetLeftValue( VtValue ) = 0;
    virtual VtValue GetZero() const = 0;
    virtual bool ValueCanBeInterpolated() const = 0;

    // Extrapolation.
    // Note these methods don't actually use any data from this object
    // and only depend on the spline type and the given parameters.
    //
    virtual VtValue GetSlope( const Ts_Data& ) const = 0;
    virtual VtValue Extrapolate( const VtValue& value, TsTime dt,
                                 const VtValue& slope) const = 0;

    // Tangents

    /// True if the data type supports tangents, and the knot type is one that
    /// shows tangents in the UI.  True only for Bezier.  Linear and held knots
    /// return false, even though their tangents can be set.
    virtual bool HasTangents() const = 0;

    /// If true, implies the tangents can be written. For historical reasons,
    /// linear and held knots support tangents.  This means that these types
    /// return true for ValueTypeSupportsTangents() but false for HasTangents().
    // XXX: pixar-ism?
    virtual bool ValueTypeSupportsTangents() const = 0;

    virtual VtValue GetLeftTangentSlope() const = 0;
    virtual VtValue GetRightTangentSlope() const = 0;
    virtual TsTime GetLeftTangentLength() const = 0;
    virtual TsTime GetRightTangentLength() const = 0;
    virtual void SetLeftTangentSlope( VtValue ) = 0;
    virtual void SetRightTangentSlope( VtValue ) = 0;
    virtual void SetLeftTangentLength( TsTime ) = 0;
    virtual void SetRightTangentLength( TsTime ) = 0;
    virtual bool GetTangentSymmetryBroken() const = 0;
    virtual void SetTangentSymmetryBroken( bool broken ) = 0;
    virtual void ResetTangentSymmetryBroken() = 0;

private:

    TsTime _time = 0.0;
};

// Typed keyframe data class.
template <typename T>
class Ts_TypedData : public Ts_Data {
public:
    typedef T ValueType;
    typedef Ts_TypedData<T> This;

    Ts_TypedData(const T&);
    Ts_TypedData(
        const TsTime &t,
        bool isDual,
        const T& leftValue,
        const T& rightValue,
        const T& leftTangentSlope,
        const T& rightTangentSlope);

    ~Ts_TypedData() override = default;

    void CloneInto(Ts_PolymorphicDataHolder *holder) const override;

    // Create a untyped eval cache for the segment defined by ourself and the
    // given keyframe.
    std::shared_ptr<Ts_UntypedEvalCache> CreateEvalCache(
        Ts_Data const* kf2) const override;

    // Evaluate between this keyframe data and \p kf2 at \p time.  This is
    // useful for callers that do not otherwise want or need to create/retain an
    // eval cache.
    VtValue EvalUncached(
        Ts_Data const *kf2, TsTime time) const override;

    // Evaluate the derivative between this keyframe data and \p kf2 at \p time.
    // This is useful for callers that do not otherwise want or need to
    // create/retain an eval cache.
    VtValue EvalDerivativeUncached(
        Ts_Data const *kf2, TsTime time) const override;

    // Create a typed eval cache for the segment defined by ourself and the
    // given keyframe.
    std::shared_ptr<Ts_EvalCache<T,
        TsTraits<T>::interpolatable> > CreateTypedEvalCache(
        Ts_Data const* kf2) const;

    bool operator==(const Ts_Data &) const override;

    // Knot type
    TsKnotType GetKnotType() const override;
    void SetKnotType( TsKnotType knotType ) override;
    bool CanSetKnotType(
        TsKnotType knotType, std::string *reason ) const override;

    // Values
    VtValue GetValue() const override;
    void SetValue( VtValue ) override;
    VtValue GetValueDerivative() const override;
    bool GetIsDualValued() const override;
    void SetIsDualValued( bool isDual ) override;
    VtValue GetLeftValue() const override;
    VtValue GetLeftValueDerivative() const override;
    void SetLeftValue( VtValue ) override;
    VtValue GetZero() const override;
    bool ValueCanBeInterpolated() const override;

    // Tangents
    bool HasTangents() const override;
    bool ValueTypeSupportsTangents() const override;
    VtValue GetLeftTangentSlope() const override;
    VtValue GetRightTangentSlope() const override;
    TsTime GetLeftTangentLength() const override;
    TsTime GetRightTangentLength() const override;
    void SetLeftTangentSlope( VtValue ) override;
    void SetRightTangentSlope( VtValue ) override;
    void SetLeftTangentLength( TsTime ) override;
    void SetRightTangentLength( TsTime ) override;
    bool GetTangentSymmetryBroken() const override;
    void SetTangentSymmetryBroken( bool broken ) override;
    void ResetTangentSymmetryBroken() override;

public:

    // Extrapolation methods.

    VtValue GetSlope(const Ts_Data &right) const override
    {
        if constexpr (TsTraits<T>::extrapolatable)
        {
            const TsTime dx = right.GetTime() - GetTime();
            const TsTime dxInv = 1.0 / dx;

            const T y1 = GetValue().template Get<T>();
            const T y2 = right.GetLeftValue().template Get<T>();
            const T dy = y2 - y1;

            // This is effectively dy/dx, but some types lack operator/, so
            // phrase in terms of operator*.
            const T slope = dy * dxInv;
            return VtValue(slope);
        }
        else
        {
            return VtValue(TsTraits<T>::zero);
        }
    }

    VtValue Extrapolate(
        const VtValue &value, TsTime dt, const VtValue &slope) const override
    {
        if constexpr (TsTraits<T>::extrapolatable)
        {
            const T v = value.template Get<T>();
            const T s = slope.template Get<T>();
            const T result = v + dt * s;
            return VtValue(result);
        }
        else
        {
            return value;
        }
    }

private:
    
    // Convenience accessors for the data stored inside the _Values struct.

    T const& _GetRightValue() const {
        return _values.Get()._rhv;
    }
    T const& _GetLeftValue() const {
        return _values.Get()._lhv;
    }
    T const& _GetRightTangentSlope() const {
        return _values.Get()._rightTangentSlope;
    }
    T const& _GetLeftTangentSlope() const {
        return _values.Get()._leftTangentSlope;
    }

    void _SetRightValue(T const& rhv) {
        _values.GetMutable()._rhv = rhv;
    }
    void _SetLeftValue(T const& lhv) {
        _values.GetMutable()._lhv = lhv;
    }
    void _SetRightTangentSlope(T const& rightTangentSlope) {
        _values.GetMutable()._rightTangentSlope = rightTangentSlope;
    }
    void _SetLeftTangentSlope(T const& leftTangentSlope) {
        _values.GetMutable()._leftTangentSlope = leftTangentSlope;
    }

private:
    friend class TsKeyFrame;
    friend class Ts_UntypedEvalCache;
    friend class Ts_EvalQuaternionCache<T>;
    friend class Ts_EvalCache<T, TsTraits<T>::interpolatable>;

    // A struct containing all the member variables that depend on type T.
    template <class V>
    struct _Values {

        explicit _Values(
            V const& lhv=TsTraits<T>::zero,
            V const& rhv=TsTraits<T>::zero,
            V const& leftTangentSlope=TsTraits<T>::zero,
            V const& rightTangentSlope=TsTraits<T>::zero) :
            _lhv(lhv),
            _rhv(rhv),
            _leftTangentSlope(leftTangentSlope),
            _rightTangentSlope(rightTangentSlope)
        {
        }

        // Left and right hand values.
        // Single-value knots only use _rhv; dual-value knots use both.
        V _lhv, _rhv;
        
        // Tangent slope, or derivative, in units per frame.
        V _leftTangentSlope, _rightTangentSlope;
    };

    // A wrapper for _Values with small-object optimization.  The _ValuesHolder
    // object is always the same size.  If T is sizeof(double) or smaller, the
    // _Values struct is held in member data.  If T is larger than double, the
    // struct is heap-allocated.
    class _ValuesHolder
    {
    private:
        static constexpr size_t _size = sizeof(_Values<double>);
        static constexpr bool _isSmall = (sizeof(_Values<T>) <= _size);

        // Storage implementation for small types.
        struct _LocalStorage
        {
            _LocalStorage(_Values<T> &&values)
                : _data(std::move(values)) {}

            const _Values<T>& Get() const { return _data; }
            _Values<T>& GetMutable() { return _data; }

            _Values<T> _data;
        };

        // Storage implementation for large types.
        struct _HeapStorage
        {
            _HeapStorage(_Values<T> &&values)
                : _data(new _Values<T>(std::move(values))) {}

            // Copy constructor: deep-copies data.
            _HeapStorage(const _HeapStorage &other)
                : _data(new _Values<T>(other.Get())) {}

            const _Values<T>& Get() const { return *_data; }
            _Values<T>& GetMutable() { return *_data; }

            std::unique_ptr<_Values<T>> _data;
        };

        // Select storage implementation.
        using _Storage =
            typename std::conditional<
                _isSmall, _LocalStorage, _HeapStorage>::type;

    public:
        // Construct from _Values rvalue.
        explicit _ValuesHolder(_Values<T> &&values)
            : _storage(std::move(values)) {}

        // Copy constructor.
        _ValuesHolder(const _ValuesHolder &other)
            : _storage(other._storage) {}

        // Destructor: explicitly call _Storage destructor.
        ~_ValuesHolder() { _storage.~_Storage(); }

        // Accessors.
        const _Values<T>& Get() const { return _storage.Get(); }
        _Values<T>& GetMutable() { return _storage.GetMutable(); }

    private:
        union
        {
            _Storage _storage;
            char _padding[_size];
        };
    };

    // Sanity check: every instantiation of _ValuesHolder is the same size.
    static_assert(
        sizeof(_ValuesHolder) == sizeof(_Values<double>),
        "_ValuesHolder does not have expected type-independent size");

private:
    _ValuesHolder _values;

    // Tangent length, in frames.
    TsTime _leftTangentLength, _rightTangentLength;

    TsKnotType _knotType;
    bool _isDual;
    bool _tangentSymmetryBroken;
};

// A wrapper for Ts_TypedData<T> for arbitrary T, exposed as a pointer to the
// non-templated base class Ts_Data, but allocated in member data rather than
// on the heap.
class Ts_PolymorphicDataHolder
{
public:
    // Wrapper for held-knot-at-time-zero constructor.
    template <typename T>
    void New(const T &val)
    {
        new (&_storage) Ts_TypedData<T>(val);
    }

    // Wrapper for general constructor.
    template <typename T>
    void New(
        const TsTime &t,
        bool isDual,
        const T &leftValue,
        const T &rightValue,
        const T &leftTangentSlope,
        const T &rightTangentSlope)
    {
        new (&_storage) Ts_TypedData<T>(
            t, isDual, leftValue, rightValue,
            leftTangentSlope, rightTangentSlope);
    }

    // Copy constructor.
    template <typename T>
    void New(const Ts_TypedData<T> &other)
    {
        new (&_storage) Ts_TypedData<T>(other);
    }

    // Explicit destructor.  Clients call this method from their destructors,
    // and prior to calling New to replace an existing knot.
    void Destroy()
    {
        reinterpret_cast<Ts_Data*>(&_storage)->~Ts_Data();
    }

    // Const accessor.
    const Ts_Data* Get() const
    {
        return reinterpret_cast<const Ts_Data*>(&_storage);
    }

    // Non-const accessor.
    Ts_Data* GetMutable()
    {
        return reinterpret_cast<Ts_Data*>(&_storage);
    }

private:
    // Our buffer is sized for Ts_TypedData<T>.  This is always the same size
    // regardless of T; see Ts_TypedData::_ValuesHolder.
    using _Storage =
        typename std::aligned_storage<
            sizeof(Ts_TypedData<double>), sizeof(void*)>::type;

private:
    _Storage _storage;
};

////////////////////////////////////////////////////////////////////////
// Ts_TypedData

template <typename T>
Ts_TypedData<T>::Ts_TypedData(const T& value) :
    _values(_Values<T>(value,value)),
    _leftTangentLength(0.0),
    _rightTangentLength(0.0),
    _knotType(TsKnotHeld),
    _isDual(false),
    _tangentSymmetryBroken(false)
{
}

template <typename T>
Ts_TypedData<T>::Ts_TypedData(
    const TsTime &t,
    bool isDual,
    const T& leftValue,
    const T& rightValue,
    const T& leftTangentSlope,
    const T& rightTangentSlope) :
    _values(_Values<T>(leftValue,rightValue,
                leftTangentSlope,rightTangentSlope)),
    _leftTangentLength(0.0),
    _rightTangentLength(0.0),
    _knotType(TsKnotHeld),
    _isDual(isDual),
    _tangentSymmetryBroken(false)
{
    SetTime(t);
}

template <typename T>
void
Ts_TypedData<T>::CloneInto(Ts_PolymorphicDataHolder *holder) const
{
    holder->New(*this);
}

template <typename T>
std::shared_ptr<Ts_UntypedEvalCache>
Ts_TypedData<T>::CreateEvalCache(Ts_Data const* kf2) const
{
    // Cast kf2 to the correct typed data.  This is a private class, and we
    // assume kf2 is from the same spline, so it will have the same value type.
    Ts_TypedData<T> const* typedKf2 =
        static_cast<Ts_TypedData<T> const*>(kf2);

    // Construct and return a new EvalCache of the appropriate type.
    return std::make_shared<
        Ts_EvalCache<T, TsTraits<T>::interpolatable>>(this, typedKf2);
}

template <typename T>
std::shared_ptr<Ts_EvalCache<T, TsTraits<T>::interpolatable> >
Ts_TypedData<T>::CreateTypedEvalCache(Ts_Data const* kf2) const
{
    Ts_TypedData<T> const* typedKf2 =
        static_cast<Ts_TypedData<T> const*>(kf2);

    return std::shared_ptr<Ts_EvalCache<T, TsTraits<T>::interpolatable> >(
        new Ts_EvalCache<T, TsTraits<T>::interpolatable>(this, typedKf2));
}

template <typename T>
VtValue
Ts_TypedData<T>
::EvalUncached(Ts_Data const *kf2, TsTime time) const
{
    // Cast kf2 to the correct typed data.  This is a private class, and we
    // assume kf2 is from the same spline, so it will have the same value type.
    Ts_TypedData<T> const* typedKf2 =
        static_cast<Ts_TypedData<T> const*>(kf2);

    return Ts_EvalCache<T, TsTraits<T>::interpolatable>(this, typedKf2)
        .Eval(time);
}

template <typename T>
VtValue
Ts_TypedData<T>
::EvalDerivativeUncached(Ts_Data const *kf2, TsTime time) const
{
    // Cast kf2 to the correct typed data.  This is a private class, and we
    // assume kf2 is from the same spline, so it will have the same value type.
    Ts_TypedData<T> const* typedKf2 =
        static_cast<Ts_TypedData<T> const*>(kf2);

    return Ts_EvalCache<T, TsTraits<T>::interpolatable>(this, typedKf2)
        .EvalDerivative(time);
}

template <typename T>
bool
Ts_TypedData<T>::operator==(const Ts_Data &rhs) const
{
    if (!TsTraits<T>::supportsTangents) {
        return
            GetKnotType() == rhs.GetKnotType() &&
            GetTime() == rhs.GetTime() &&
            GetValue() == rhs.GetValue() &&
            GetIsDualValued() == rhs.GetIsDualValued() &&
            (!GetIsDualValued() || (GetLeftValue() == rhs.GetLeftValue()));
    }

    return
        GetTime() == rhs.GetTime() &&
        GetValue() == rhs.GetValue() &&
        GetKnotType() == rhs.GetKnotType() &&
        GetIsDualValued() == rhs.GetIsDualValued() &&
        (!GetIsDualValued() || (GetLeftValue() == rhs.GetLeftValue())) &&
        GetLeftTangentLength() == rhs.GetLeftTangentLength() &&
        GetRightTangentLength() == rhs.GetRightTangentLength() &&
        GetLeftTangentSlope() == rhs.GetLeftTangentSlope() &&
        GetRightTangentSlope() == rhs.GetRightTangentSlope() &&
        GetTangentSymmetryBroken() == rhs.GetTangentSymmetryBroken();
}

template <typename T>
TsKnotType
Ts_TypedData<T>::GetKnotType() const
{
    return _knotType;
}

template <typename T>
void
Ts_TypedData<T>::SetKnotType( TsKnotType knotType )
{
    std::string reason;

    if (!CanSetKnotType(knotType, &reason)) {
        TF_CODING_ERROR(reason);
        return;
    }

    _knotType = knotType;
}

template <typename T>
bool
Ts_TypedData<T>::CanSetKnotType( TsKnotType knotType,
                                   std::string *reason ) const
{
    // Non-interpolatable values can only have held key frames.
    if (!ValueCanBeInterpolated() && knotType != TsKnotHeld) {
        if (reason) {
            *reason = "Value cannot be interpolated; only 'held' " \
                "key frames are allowed.";
        }
        return false;
    }

    // Only value types that support tangents can have bezier key frames.
    if (!TsTraits<T>::supportsTangents && knotType == TsKnotBezier) {
        if (reason) {
            *reason = TfStringPrintf(
                    "Cannot set keyframe type %s; values of type '%s' "
                    "do not support tangents.", 
                    TfEnum::GetDisplayName(knotType).c_str(),
                    ArchGetDemangled(typeid(ValueType)).c_str());
        }
        return false;
    }

    return true;
}

template <typename T>
VtValue
Ts_TypedData<T>::GetValue() const
{
    return VtValue(_GetRightValue());
}

template <typename T>
VtValue
Ts_TypedData<T>::GetValueDerivative() const
{
    if (TsTraits<T>::supportsTangents) {
        return GetRightTangentSlope();
    } else {
        return VtValue(TsTraits<T>::zero);
    }
}

template <typename T>
void
Ts_TypedData<T>::SetValue( VtValue val )
{
    VtValue v = val.Cast<T>();
    if (!v.IsEmpty()) {
        _SetRightValue(v.Get<T>());
        if (!ValueCanBeInterpolated())
            SetKnotType(TsKnotHeld);
    } else {
        TF_CODING_ERROR("cannot convert type '%s' to '%s' to assign "
                        "to keyframe", val.GetTypeName().c_str(),
                        ArchGetDemangled(typeid(ValueType)).c_str());
    }
}

template <typename T>
bool
Ts_TypedData<T>::GetIsDualValued() const
{
    return _isDual;
}

template <typename T>
void
Ts_TypedData<T>::SetIsDualValued( bool isDual )
{
    if (isDual && !TsTraits<T>::interpolatable) {
        TF_CODING_ERROR("keyframes of type '%s' cannot be dual-valued",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return;
    }

    _isDual = isDual;

    if (_isDual) {
        // The data stored for the left value was meaningless.
        // Mirror the right-side value to the left.
        SetLeftValue(GetValue());
    }
}

template <typename T>
VtValue 
Ts_TypedData<T>::GetLeftValue() const
{
    return VtValue(_isDual ? _GetLeftValue() : _GetRightValue());
}

template <typename T>
VtValue 
Ts_TypedData<T>::GetLeftValueDerivative() const
{
    if (TsTraits<T>::supportsTangents) {
        return GetLeftTangentSlope();
    } else {
        return VtValue(TsTraits<T>::zero);
    }
}

template <typename T>
void 
Ts_TypedData<T>::SetLeftValue( VtValue val )
{
    if (!TsTraits<T>::interpolatable) {
        TF_CODING_ERROR("keyframes of type '%s' cannot be dual-valued",
                        ArchGetDemangled(typeid(ValueType)).c_str() );
        return;
    }
    if (!GetIsDualValued()) {
        TF_CODING_ERROR("keyframe is not dual-valued; cannot set left value");
        return;
    }

    VtValue v = val.Cast<T>();
    if (!v.IsEmpty()) {
        _SetLeftValue(v.Get<T>());
        if (!ValueCanBeInterpolated())
            SetKnotType(TsKnotHeld);
    } else {
        TF_CODING_ERROR("cannot convert type '%s' to '%s' to assign to "
                        "keyframe", val.GetTypeName().c_str(),
                        ArchGetDemangled(typeid(ValueType)).c_str());
    }
}

template <typename T>
VtValue
Ts_TypedData<T>::GetZero() const
{
    return VtValue(TsTraits<T>::zero);
}

template <typename T>
bool
Ts_TypedData<T>::ValueCanBeInterpolated() const
{
    return TsTraits<T>::interpolatable;
}

template <typename T>
bool
Ts_TypedData<T>::HasTangents() const
{
    return TsTraits<T>::supportsTangents && _knotType == TsKnotBezier;
}

template <typename T>
bool
Ts_TypedData<T>::ValueTypeSupportsTangents() const
{
    // Oddly, linear and held knots have settable tangents.  Animators use
    // this when switching Beziers to Held and then back again.
    return TsTraits<T>::supportsTangents;
}

template <typename T>
VtValue
Ts_TypedData<T>::GetLeftTangentSlope() const
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return VtValue();
    }

    return VtValue(_GetLeftTangentSlope());
}

template <typename T>
VtValue
Ts_TypedData<T>::GetRightTangentSlope() const
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str() );
        return VtValue();
    }

    return VtValue(_GetRightTangentSlope());
}

template <typename T>
TsTime
Ts_TypedData<T>::GetLeftTangentLength() const
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return 0;
    }

    return _leftTangentLength;
}

template <typename T>
TsTime
Ts_TypedData<T>::GetRightTangentLength() const
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return 0;
    }

    return _rightTangentLength;
}

template <typename T>
void
Ts_TypedData<T>::SetLeftTangentSlope( VtValue val )
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return;
    }

    VtValue v = val.Cast<T>();
    if (!v.IsEmpty()) {
        _SetLeftTangentSlope(val.Get<T>());
    } else {
        TF_CODING_ERROR("cannot convert type '%s' to '%s' to assign to "
                        "keyframe", val.GetTypeName().c_str(),
                        ArchGetDemangled(typeid(ValueType)).c_str());
    }
}

template <typename T>
void
Ts_TypedData<T>::SetRightTangentSlope( VtValue val )
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return;
    }

    VtValue v = val.Cast<T>();
    if (!v.IsEmpty()) {
        _SetRightTangentSlope(val.Get<T>());
    } else {
        TF_CODING_ERROR("cannot convert type '%s' to '%s' to assign to keyframe"
                        , val.GetTypeName().c_str(),
                        ArchGetDemangled(typeid(ValueType)).c_str());
    }
}

#define TS_LENGTH_EPSILON 1e-6

template <typename T>
void
Ts_TypedData<T>::SetLeftTangentLength( TsTime newLen )
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR( "keyframes of type '%s' do not have tangents",
                         ArchGetDemangled(typeid(ValueType)).c_str());
        return;
    }
    if (std::isnan(newLen)) {
        TF_CODING_ERROR("Cannot set tangent length to NaN; ignoring");
        return;
    }
    if (std::isinf(newLen)) {
        TF_CODING_ERROR("Cannot set tangent length to inf; ignoring");
        return;
    }
    if (newLen < 0.0) {
        if (-newLen < TS_LENGTH_EPSILON) {
            newLen = 0.0;
        } else {
            TF_CODING_ERROR(
                "Cannot set tangent length to negative value; ignoring");
            return;
        }
    }

    _leftTangentLength = newLen;
}

template <typename T>
void
Ts_TypedData<T>::SetRightTangentLength( TsTime newLen )
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return;
    }
    if (std::isnan(newLen)) {
        TF_CODING_ERROR("Cannot set tangent length to NaN; ignoring");
        return;
    }
    if (std::isinf(newLen)) {
        TF_CODING_ERROR("Cannot set tangent length to inf; ignoring");
        return;
    }
    if (newLen < 0.0) {
        if (-newLen < TS_LENGTH_EPSILON) {
            newLen = 0.0;
        } else {
            TF_CODING_ERROR(
                "Cannot set tangent length to negative value; ignoring");
            return;
        }
    }

    _rightTangentLength = newLen;
}

template <typename T>
bool
Ts_TypedData<T>::GetTangentSymmetryBroken() const
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return false;
    }

    return _tangentSymmetryBroken;
}

template <typename T>
void
Ts_TypedData<T>::SetTangentSymmetryBroken( bool broken )
{
    if (!TsTraits<T>::supportsTangents) {
        TF_CODING_ERROR("keyframes of type '%s' do not have tangents",
                        ArchGetDemangled(typeid(ValueType)).c_str());
        return;
    }

    if (_tangentSymmetryBroken != broken) {
        _tangentSymmetryBroken = broken;
        if (!_tangentSymmetryBroken) {
            _SetLeftTangentSlope(_GetRightTangentSlope());
        }
    }
}

template <typename T>
void
Ts_TypedData<T>::ResetTangentSymmetryBroken()
{
    // do nothing -- no tangents
}

// Declare specializations for float and double.
// Definitions are in Data.cpp.
template <>
TS_API void
Ts_TypedData<float>::ResetTangentSymmetryBroken();

template <>
TS_API void
Ts_TypedData<double>::ResetTangentSymmetryBroken();

template <>
TS_API bool
Ts_TypedData<float>::ValueCanBeInterpolated() const;

template <>
TS_API bool
Ts_TypedData<double>::ValueCanBeInterpolated() const;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
