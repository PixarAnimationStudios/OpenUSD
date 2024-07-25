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
        return (preTanMayaForm ? preTanWidth / 3.0 : preTanWidth);
    }

    TsTime GetMayaPreTanWidth() const
    {
        return (preTanMayaForm ? preTanWidth : preTanWidth * 3.0);
    }

    TsTime GetPostTanWidth() const
    {
        return (postTanMayaForm ? postTanWidth / 3.0 : postTanWidth);
    }

    TsTime GetMayaPostTanWidth() const
    {
        return (postTanMayaForm ? postTanWidth : postTanWidth * 3.0);
    }

    void SetPreTanWidth(const TsTime width)
    {
        preTanWidth = (preTanMayaForm ? width * 3.0 : width);
    }

    void SetPostTanWidth(const TsTime width)
    {
        postTanWidth = (postTanMayaForm ? width * 3.0 : width);
    }

public:
    // Knot time.
    TsTime time;

    // Time width of the pre-tangent.  Always non-negative.  Ignored for Hermite
    // knots.  If pre-tangent is in standard form, this is the actual width; if
    // in Maya form, this is the width multiplied by 3.
    TsTime preTanWidth;

    // Time width of the post-tangent.  Always non-negative.  Ignored for
    // Hermite knots.  If post-tangent is in standard form, this is the actual
    // width; if in Maya form, this is the width multiplied by 3.
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
    TsCurveType curveType : 2;

    // Whether this knot is dual-valued (value discontinuity at the knot).
    bool dualValued : 1;

    // Whether pre-tangent is in Maya form; if false, it is in standard form.
    bool preTanMayaForm : 1;

    // Whether post-tangent is in Maya form; if false, it is in standard form.
    bool postTanMayaForm : 1;
};


// Data for one knot in a spline.
//
// Tangents have two forms:
//
// - Standard tangents are expressed as width and slope.
//
// - Maya tangents are expressed as width and height.  All values are stored
//   multiplied by 3.  Heights are negated for pre-tangents.
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
    T GetMayaPreTanHeight() const;
    T GetPostTanSlope() const;
    T GetPostTanHeight() const;
    T GetMayaPostTanHeight() const;

public:
    // Value at this knot.
    T value;

    // If dual-valued, the pre-value at this knot.
    T preValue;

    // When pre-tangent is in standard form, preTanSlope stores the slope of the
    // pre-tangent, rise over run, value height divided by time width.  When
    // pre-tangent is in Maya form, preTanMayaHeight stores the height of the
    // pre-tangent, multplied by 3 and negated.
    union
    {
        T preTanSlope;
        T preTanMayaHeight;
    };

    // When post-tangent is in standard form, postTanSlope stores the slope of
    // the post-tangent, rise over run, value height divided by time width.
    // When post-tangent is in Maya form, postTanMayaHeight stores the height of
    // the post-tangent, multplied by 3.
    union
    {
        T postTanSlope;
        T postTanMayaHeight;
    };
};

// For double-typed values, on x86-64, this struct should fit in a cache line.
// Exceeding this size may impact performance.
static_assert(sizeof(Ts_TypedKnotData<double>) <= 64);


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
    virtual void SetMayaPreTanHeight(VtValue height) = 0;
    virtual void GetMayaPreTanHeight(VtValue *heightOut) const = 0;
    virtual void SetPostTanSlope(VtValue slope) = 0;
    virtual void GetPostTanSlope(VtValue *slopeOut) const = 0;
    virtual void SetMayaPostTanHeight(VtValue height) = 0;
    virtual void GetMayaPostTanHeight(VtValue *heightOut) const = 0;
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
    void SetMayaPreTanHeight(VtValue height) override;
    void GetMayaPreTanHeight(VtValue *heightOut) const override;
    void SetPostTanSlope(VtValue slope) override;
    void GetPostTanSlope(VtValue *slopeOut) const override;
    void SetMayaPostTanHeight(VtValue height) override;
    void GetMayaPostTanHeight(VtValue *heightOut) const override;

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
    COMP(curveType);
    COMP(preTanMayaForm);
    COMP(postTanMayaForm);

    COMP(value);
    COMP(preValue);
    COMP(preTanSlope);
    COMP(postTanSlope);

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
    if (!preTanMayaForm)
    {
        return preTanSlope;
    }
    else if (preTanWidth == 0)
    {
        return 0;
    }
    else
    {
        return -preTanMayaHeight / preTanWidth;
    }
}

template <typename T>
T Ts_TypedKnotData<T>::GetPreTanHeight() const
{
    if (preTanMayaForm)
    {
        return -preTanMayaHeight / 3.0;
    }
    else
    {
        return -preTanWidth * preTanSlope;
    }
}

template <typename T>
T Ts_TypedKnotData<T>::GetMayaPreTanHeight() const
{
    if (preTanMayaForm)
    {
        return preTanMayaHeight;
    }
    else
    {
        return preTanWidth * preTanSlope * 3.0;
    }
}

template <typename T>
T Ts_TypedKnotData<T>::GetPostTanSlope() const
{
    if (!postTanMayaForm)
    {
        return postTanSlope;
    }
    else if (postTanWidth == 0)
    {
        return 0;
    }
    else
    {
        return postTanMayaHeight / postTanWidth;
    }
}

template <typename T>
T Ts_TypedKnotData<T>::GetPostTanHeight() const
{
    if (postTanMayaForm)
    {
        return postTanMayaHeight / 3.0;
    }
    else
    {
        return postTanWidth * postTanSlope;
    }
}

template <typename T>
T Ts_TypedKnotData<T>::GetMayaPostTanHeight() const
{
    if (postTanMayaForm)
    {
        return postTanMayaHeight;
    }
    else
    {
        return postTanWidth * postTanSlope * 3.0;
    }
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
void Ts_TypedKnotDataProxy<T>::SetMayaPreTanHeight(
    const VtValue height)
{
    _data->preTanMayaHeight = height.UncheckedGet<T>();
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::GetMayaPreTanHeight(
    VtValue* const heightOut) const
{
    *heightOut = VtValue(_data->GetMayaPreTanHeight());
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
void Ts_TypedKnotDataProxy<T>::SetMayaPostTanHeight(
    const VtValue height)
{
    _data->postTanMayaHeight = height.UncheckedGet<T>();
}

template <typename T>
void Ts_TypedKnotDataProxy<T>::GetMayaPostTanHeight(
    VtValue* const heightOut) const
{
    *heightOut = VtValue(_data->GetMayaPostTanHeight());
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif
