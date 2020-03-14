//
// Copyright 2016 Pixar
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
#ifndef PXR_USD_USD_INTERPOLATORS_H
#define PXR_USD_USD_INTERPOLATORS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/valueUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/gf/math.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;

/// \class Usd_InterpolatorBase
///
/// Base class for objects implementing interpolation for attribute
/// values. This is invoked during value resolution for times that do not have
/// authored time samples.
///
class Usd_InterpolatorBase
{
public:
    virtual bool Interpolate(
        const SdfLayerRefPtr& layer, const SdfPath& path,
        double time, double lower, double upper) = 0;
    virtual bool Interpolate(
        const Usd_ClipRefPtr& clip, const SdfPath& path,
        double time, double lower, double upper) = 0;
};

/// \class Usd_NullInterpolator
///
/// Null interpolator object for use in cases where interpolation is
/// not expected.
///
class Usd_NullInterpolator
    : public Usd_InterpolatorBase
{
public:
    virtual bool Interpolate(
        const SdfLayerRefPtr& layer, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return false;
    }

    virtual bool Interpolate(
        const Usd_ClipRefPtr& clip, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return false;
    }
};

/// \class Usd_UntypedInterpolator
///
/// Interpolator used for type-erased value access.
///
/// The type-erased value API does not provide information about the
/// expected value type, so this interpolator needs to do more costly
/// type lookups to dispatch to the appropriate interpolator.
///
class Usd_UntypedInterpolator
    : public Usd_InterpolatorBase
{
public:
    Usd_UntypedInterpolator(const UsdAttribute& attr, VtValue* result)
        : _attr(attr)
        , _result(result)
    {
    }

    virtual bool Interpolate(
        const SdfLayerRefPtr& layer, const SdfPath& path,
        double time, double lower, double upper) final;

    virtual bool Interpolate(
        const Usd_ClipRefPtr& clip, const SdfPath& path,
        double time, double lower, double upper) final;

private:
    template <class Src>
    bool _Interpolate(
        const Src& src, const SdfPath& path,
        double time, double lower, double upper);

private:
    const UsdAttribute& _attr;
    VtValue* _result;
};

/// \class Usd_HeldInterpolator
///
/// Object implementing "held" interpolation for attribute values.
///
/// With "held" interpolation, authored time sample values are held constant
/// across time until the next authored time sample. In other words, the
/// attribute value for a time with no samples authored is the nearest
/// preceding value.
///
template <class T>
class Usd_HeldInterpolator 
    : public Usd_InterpolatorBase
{
public:
    Usd_HeldInterpolator(T* result)
        : _result(result)
    {
    }

    virtual bool Interpolate(
        const SdfLayerRefPtr& layer, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return layer->QueryTimeSample(path, lower, _result);
    }

    virtual bool Interpolate(
        const Usd_ClipRefPtr& clip, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return clip->QueryTimeSample(path, lower, this, _result);
    }

private:
    T* _result;
};

template <class T>
inline T
Usd_Lerp(double alpha, const T &lower, const T &upper)
{
    return GfLerp(alpha, lower, upper);
}

inline GfQuath
Usd_Lerp(double alpha, const GfQuath &lower, const GfQuath &upper)
{
    return GfSlerp(alpha, lower, upper);
}

inline GfQuatf
Usd_Lerp(double alpha, const GfQuatf &lower, const GfQuatf &upper)
{
    return GfSlerp(alpha, lower, upper);
}

inline GfQuatd
Usd_Lerp(double alpha, const GfQuatd &lower, const GfQuatd &upper)
{
    return GfSlerp(alpha, lower, upper);
}

/// \class Usd_LinearInterpolator
///
/// Object implementing linear interpolation for attribute values.
///
/// With linear interpolation, the attribute value for a time with no samples
/// will be linearly interpolated from the previous and next time samples.
///
template <class T>
class Usd_LinearInterpolator
    : public Usd_InterpolatorBase
{
public:
    Usd_LinearInterpolator(T* result)
        : _result(result)
    {
    }

    virtual bool Interpolate(
        const SdfLayerRefPtr& layer, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return _Interpolate(layer, path, time, lower, upper);
    }

    virtual bool Interpolate(
        const Usd_ClipRefPtr& clip, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return _Interpolate(clip, path, time, lower, upper);
    }

private:
    template <class Src>
    bool _Interpolate(
        const Src& src, const SdfPath& path,
        double time, double lower, double upper)
    {
        T lowerValue, upperValue;

        // In the presence of a value block we use held interpolation. 
        // We know that a failed call to QueryTimeSample is a block
        // because the provided time samples should all have valid values.
        // So this call fails because our <T> is not an SdfValueBlock,
        // which is the type of the contained value.
        Usd_LinearInterpolator<T> lowerInterpolator(&lowerValue);
        Usd_LinearInterpolator<T> upperInterpolator(&upperValue);

        if (!Usd_QueryTimeSample(
                src, path, lower, &lowerInterpolator, &lowerValue)) {
            return false;
        } 
        else if (!Usd_QueryTimeSample(
                src, path, upper, &upperInterpolator, &upperValue)) {
            upperValue = lowerValue; 
        }

        const double parametricTime = (time - lower) / (upper - lower);
        *_result = Usd_Lerp(parametricTime, lowerValue, upperValue);
        return true;
    }

private:
    T* _result;
};

// Specialization to linearly interpolate each element for
// array types.
template <class T>
class Usd_LinearInterpolator<VtArray<T> >
    : public Usd_InterpolatorBase
{
public:
    Usd_LinearInterpolator(VtArray<T>* result)
        : _result(result)
    {
    }

    virtual bool Interpolate(
        const SdfLayerRefPtr& layer, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return _Interpolate(layer, path, time, lower, upper);
    }

    virtual bool Interpolate(
        const Usd_ClipRefPtr& clip, const SdfPath& path,
        double time, double lower, double upper) final
    {
        return _Interpolate(clip, path, time, lower, upper);
    }

private:
    template <class Src>
    bool _Interpolate(
        const Src& src, const SdfPath& path,
        double time, double lower, double upper)
    {
        VtArray<T> lowerValue, upperValue;

        // In the presence of a value block we use held interpolation.
        // We know that a failed call to QueryTimeSample is a block
        // because the provided time samples should all have valid values.
        // So this call fails because our <T> is not an SdfValueBlock,
        // which is the type of the contained value.
        Usd_LinearInterpolator<VtArray<T> > lowerInterpolator(&lowerValue);
        Usd_LinearInterpolator<VtArray<T> > upperInterpolator(&upperValue);
        
        if (!Usd_QueryTimeSample(
                src, path, lower, &lowerInterpolator, &lowerValue)) {
            return false;
        } 
        else if (!Usd_QueryTimeSample(
                src, path, upper, &upperInterpolator, &upperValue)) {
            upperValue = lowerValue;
        }

        _result->swap(lowerValue);

        // Fall back to held interpolation (_result is set to lowerValue above)
        // if sizes don't match. We don't consider this an error because
        // that would be too restrictive. Consumers will be responsible for
        // implementing their own interpolation in cases where this occurs
        // (e.g. meshes with varying topology)
        if (_result->size() != upperValue.size()) {
            return true;
        }

        const double parametricTime = (time - lower) / (upper - lower);
        if (parametricTime == 0.0) {
            // do nothing.
        }
        else if (parametricTime == 1.0) {
            // just swap the upper value in.
            _result->swap(upperValue);
        }
        else {
            // must actually calculate interpolated values.
            T *rptr = _result->data();
            for (size_t i = 0, j = _result->size(); i != j; ++i) {
                rptr[i] = Usd_Lerp(parametricTime, rptr[i], upperValue[i]);
            }
        }

        return true;
    }        

private:
    VtArray<T>* _result;
};

/// If \p lower == \p upper, sets \p result to the time sample at 
/// that time in the given \p src clip or layer. Otherwise,
/// interpolates the value at the given \p time between \p lower 
/// and \p upper using the given \p interpolator.
template <class Src, class T>
inline bool
Usd_GetOrInterpolateValue(
    const Src& src, const SdfPath& path,
    double time, double lower, double upper,
    Usd_InterpolatorBase* interpolator, T* result)
{
    if (GfIsClose(lower, upper, /* epsilon = */ 1e-6)) {
        bool queryResult = Usd_QueryTimeSample(
            src, path, lower, interpolator, result);
        return queryResult && (!Usd_ClearValueIfBlocked(result));
    }

    return interpolator->Interpolate(src, path, time, lower, upper);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INTERPOLATORS_H
