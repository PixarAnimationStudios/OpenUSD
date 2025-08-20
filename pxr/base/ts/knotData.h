//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_KNOT_DATA_H
#define PXR_BASE_TS_KNOT_DATA_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/tf/type.h"

#include <memory>
#include <cstring>

PXR_NAMESPACE_OPEN_SCOPE


// Ts API objects (Spline, Knot) are non-templated, but they can represent
// different value types (double, float, half), and internally we handle those
// different value types with templates.  Some knot members (like time) are
// type-independent, while others (like value) are type-dependent.  All knots in
// a spline have the same value type.
//
// Splines can have many knots, and we try to take up as little memory as
// possible in storing them.  We also try to be as fast as possible in accessing
// them, but the combination of non-templated API classes and templated data is
// a form of type erasure that requires a compromise: we use virtual methods to
// retrieve data.
//
// We manage knot data with two class hierarchies:
//
// - The data itself is stored as a plain struct.  There are two halves: a
//   non-templated base struct that contains the type-independent members, and a
//   templated derived struct that contains the type-dependent members.  By
//   making the data a plain struct, we avoid storing a vtable pointer for every
//   instance, but in return we have no convenient way to get at the templated
//   data directly.
//
// - We access the type-dependent members using a proxy class.  There is an
//   abstract base class that declares a virtual interface, and a templated
//   derived class that implements it.  Proxy objects have a vtable pointer, and
//   contain a pointer to a templated derived data struct.


// XXX TODO
// review access patterns - do we need all?:
//   SplineData -> AsDouble -> direct access
//   SplineData virtuals
//   Ts_Get[Typed]SplineData
//   Knot -> proxy
//   Knot -> _TypedData -> direct access
//   TsDispatchToValueTypeTemplate


// Non-template base class for knot data.
//
struct Ts_KnotData
{
public:
    // Typically used by Create(), but can be invoked directly for clients that
    // don't care about the value dimension, and instantiate this struct without
    // subclassing.
    Ts_KnotData();

    // Creates an appropriately subtyped instance on the heap.
    static Ts_KnotData* Create(TfType valueType);

    // Compares two KnotData structs.  Ignores subclasses.
    bool operator==(const Ts_KnotData &other) const;

public:
    // Helpers that switch on flags.

    TsTime GetPreTanWidth() const
    {
        return preTanWidth;
    }

    TsTime GetPostTanWidth() const
    {
        return postTanWidth;
    }

    void SetPreTanWidth(const TsTime width)
    {
        preTanWidth = width;
    }

    void SetPostTanWidth(const TsTime width)
    {
        postTanWidth = width;
    }

public:
    // Knot time.
    TsTime time;

    // Time width of the pre-tangent.  Always non-negative.  Ignored for Hermite
    // knots.
    TsTime preTanWidth;

    // Time width of the post-tangent.  Always non-negative.  Ignored for
    // Hermite knots.
    TsTime postTanWidth;

    // BITFIELDS - note: for enum-typed bitfields, we declare one bit more than
    // is minimally needed to represent all declared enum values.  For example,
    // TsCurveType has only two values, so it should be representable in one
    // bit.  However, compilers are free to choose the underlying representation
    // of enums, and some platforms choose signed values, meaning that we
    // actually need one bit more, so that we can hold the sign bit.  We could
    // declare the enums with unsigned underlying types, but that runs into a
    // gcc 9.2 bug.  We can spare the extra bit; alignment means there is no
    // difference in struct size.

    // Interpolation mode for the segment following this knot.
    TsInterpMode nextInterp : 3;

    // The spline type this knot belongs to, or is intended for.
    TsCurveType curveType : 2;  // deprecated

    // Whether this knot is dual-valued (value discontinuity at the knot).
    bool dualValued : 1;

    // The pre- and post-tangent algorithms.
    TsTangentAlgorithm preTanAlgorithm : 4;
    TsTangentAlgorithm postTanAlgorithm : 4;
};


// Data for one knot in a spline.
//
// Tangents are expressed as width and slope.
//
template <typename T>
struct Ts_TypedKnotData :
    public Ts_KnotData
{
public:
    Ts_TypedKnotData();

    bool operator==(const Ts_TypedKnotData<T> &other) const;

public:
    // Helpers that switch on flags.
    T GetPreValue() const;
    T GetPreTanSlope() const;
    T GetPreTanHeight() const;
    T GetPostTanSlope() const;
    T GetPostTanHeight() const;


    // Compute algorithmic tangents for this Ts_KnotData based on the provided
    // prev and next Ts_KnotData. See TsKnot::UpdateTangents for documentation.
    TS_API
    bool UpdateTangents(const Ts_TypedKnotData<T>* prevData,
                        const Ts_TypedKnotData<T>* nextData,
                        const TsCurveType curveType);

protected:
    TS_API
    bool _UpdateTangent(const Ts_TypedKnotData<T>* prevData,
                        const Ts_TypedKnotData<T>* nextData,
                        TsCurveType curveType,
                        bool updatePre);

    TS_API
    bool _UpdateTangentAutoEase(const Ts_TypedKnotData<T>* prevData,
                                const Ts_TypedKnotData<T>* nextData,
                                bool updatePre);

public:
    // Value at this knot.
    T value;

    // If dual-valued, the pre-value at this knot.
    T preValue;

    // preTanSlope stores the slope of the pre-tangent, rise over run, value
    // height divided by time width.
    T preTanSlope;

    // postTanSlope stores the slope of the post-tangent, rise over run, value
    // height divided by time width.
    T postTanSlope;
};

// Virtual interface to TypedKnotData.
//
// VtValue parameters are not type-checked.  They are blindly cast.  Callers
// must verify types.
//
class Ts_KnotDataProxy
{
public:
    // Creates an appropriately subtyped instance.
    static std::unique_ptr<Ts_KnotDataProxy>
    Create(Ts_KnotData *data, TfType valueType);

    virtual ~Ts_KnotDataProxy();

    virtual Ts_KnotData* CloneData() const = 0;
    virtual void DeleteData() = 0;

    virtual TfType GetValueType() const = 0;
    virtual bool IsDataEqualTo(const Ts_KnotData &other) const = 0;

    virtual void SetValue(VtValue value) = 0;
    virtual void GetValue(VtValue *valueOut) const = 0;
    virtual void SetPreValue(VtValue value) = 0;
    virtual void GetPreValue(VtValue *valueOut) const = 0;

    virtual void SetPreTanSlope(VtValue slope) = 0;
    virtual void GetPreTanSlope(VtValue *slopeOut) const = 0;
    virtual void SetPostTanSlope(VtValue slope) = 0;
    virtual void GetPostTanSlope(VtValue *slopeOut) const = 0;

    virtual bool UpdateTangents(const Ts_KnotDataProxy* prevProxy,
                                const Ts_KnotDataProxy* nextProxy,
                                const TsCurveType curveType) = 0;
};


// A means of accessing TypedKnotData.
//
template <typename T>
class Ts_TypedKnotDataProxy final :
    public Ts_KnotDataProxy
{
public:
    explicit Ts_TypedKnotDataProxy(Ts_TypedKnotData<T> *data);

    Ts_KnotData* CloneData() const override;
    void DeleteData() override;

    TfType GetValueType() const override;
    bool IsDataEqualTo(const Ts_KnotData &other) const override;

    void SetValue(VtValue value) override;
    void GetValue(VtValue *valueOut) const override;
    void SetPreValue(VtValue value) override;
    void GetPreValue(VtValue *valueOut) const override;

    void SetPreTanSlope(VtValue slope) override;
    void GetPreTanSlope(VtValue *slopeOut) const override;
    void SetPostTanSlope(VtValue slope) override;
    void GetPostTanSlope(VtValue *slopeOut) const override;

    bool UpdateTangents(const Ts_KnotDataProxy* prevProxy,
                        const Ts_KnotDataProxy* nextProxy,
                        const TsCurveType curveType) override;

private:
    Ts_TypedKnotData<T> *_data;
};


////////////////////////////////////////////////////////////////////////////////
// TEMPLATE IMPLEMENTATIONS

template <typename T>
Ts_TypedKnotData<T>::Ts_TypedKnotData()
    : Ts_KnotData(),
      value(T()),
      preValue(T()),
      preTanSlope(T()),
      postTanSlope(T())
{
}

#define COMP(member)                                      \
    if (member != other.member)                           \
    {                                                     \
        return false;                                     \
    }

template <typename T>
bool Ts_TypedKnotData<T>::operator==(
    const Ts_TypedKnotData<T> &other) const
{
    COMP(time);
    COMP(preTanWidth);
    COMP(postTanWidth);
    COMP(dualValued);
    COMP(nextInterp);
    // CurveType for knots has been deprecated and we no longer consider its
    // value when testing for equality.
    // COMP(curveType);

    COMP(value);
    COMP(preValue);
    COMP(preTanSlope);
    COMP(postTanSlope);

    COMP(preTanAlgorithm);
    COMP(postTanAlgorithm);

    return true;
}

#undef COMP

template <typename T>
T Ts_TypedKnotData<T>::GetPreValue() const
{
    return (dualValued ? preValue : value);
}

template <typename T>
T Ts_TypedKnotData<T>::GetPreTanSlope() const
{
    return preTanSlope;
}

template <typename T>
T Ts_TypedKnotData<T>::GetPreTanHeight() const
{
    return -preTanWidth * preTanSlope;
}

template <typename T>
T Ts_TypedKnotData<T>::GetPostTanSlope() const
{
    return postTanSlope;
}

template <typename T>
T Ts_TypedKnotData<T>::GetPostTanHeight() const
{
    return postTanWidth * postTanSlope;
}

////////////////////////////////////////////////////////////////////////////////
// TypedKnotDataProxy

template <typename T>
Ts_TypedKnotDataProxy<T>::Ts_TypedKnotDataProxy(
    Ts_TypedKnotData<T> *data)
    : _data(data)
{
}

template <typename T>
Ts_KnotData* Ts_TypedKnotDataProxy<T>::CloneData() const
{
    return new Ts_TypedKnotData<T>(*_data);
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::DeleteData()
{
    delete _data;
}

template <typename T>
TfType Ts_TypedKnotDataProxy<T>::GetValueType() const
{
    return Ts_GetType<T>();
}

template <typename T>
bool Ts_TypedKnotDataProxy<T>::IsDataEqualTo(const Ts_KnotData &other) const
{
    // Force-downcast to our value type.  Callers must verify types match.
    const Ts_TypedKnotData<T> *typedOther =
        static_cast<const Ts_TypedKnotData<T>*>(&other);

    return *_data == *typedOther;
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::SetValue(
    const VtValue value)
{
    _data->value = value.UncheckedGet<T>();
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::GetValue(
    VtValue* const valueOut) const
{
    *valueOut = VtValue(_data->value);
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::SetPreValue(
    const VtValue value)
{
    _data->preValue = value.UncheckedGet<T>();
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::GetPreValue(
    VtValue* const valueOut) const
{
    *valueOut = VtValue(_data->preValue);
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::SetPreTanSlope(
    const VtValue slope)
{
    _data->preTanSlope = slope.UncheckedGet<T>();
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::GetPreTanSlope(
    VtValue* const slopeOut) const
{
    *slopeOut = VtValue(_data->GetPreTanSlope());
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::SetPostTanSlope(
    const VtValue slope)
{
    _data->postTanSlope = slope.UncheckedGet<T>();
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::GetPostTanSlope(
    VtValue* const slopeOut) const
{
    *slopeOut = VtValue(_data->GetPostTanSlope());
}

template <typename T>
bool Ts_TypedKnotDataProxy<T>::UpdateTangents(
    const Ts_KnotDataProxy* prevProxy,
    const Ts_KnotDataProxy* nextProxy,
    const TsCurveType curveType)
{
    const Ts_TypedKnotDataProxy<T>* prevTypedProxy =
        dynamic_cast<const Ts_TypedKnotDataProxy<T>*>(prevProxy);
    const Ts_TypedKnotData<T>* prevData = (prevTypedProxy
                                           ? prevTypedProxy->_data
                                           : nullptr);
    const Ts_TypedKnotDataProxy<T>* nextTypedProxy =
        dynamic_cast<const Ts_TypedKnotDataProxy<T>*>(nextProxy);
    const Ts_TypedKnotData<T>* nextData = (nextTypedProxy
                                           ? nextTypedProxy->_data
                                           : nullptr);

    return _data->UpdateTangents(prevData, nextData, curveType);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif
