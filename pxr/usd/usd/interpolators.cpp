//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/interpolators.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/visitValue.h"

#include "pxr/base/tf/ostreamMethods.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

std::ostream &operator<<(std::ostream &stream, Usd_ValueTimeSample const &vts)
{
    return stream << '(' << vts.time << ": " << vts.value << ')';
}

template <class T>
static T
Usd_Lerp(double alpha, const T &lower, const T &upper)
{
    return GfLerp(alpha, lower, upper);
}

static inline GfQuath
Usd_Lerp(double alpha, const GfQuath &lower, const GfQuath &upper)
{
    return GfSlerp(alpha, lower, upper);
}

static inline GfQuatf
Usd_Lerp(double alpha, const GfQuatf &lower, const GfQuatf &upper)
{
    return GfSlerp(alpha, lower, upper);
}

static inline GfQuatd
Usd_Lerp(double alpha, const GfQuatd &lower, const GfQuatd &upper)
{
    return GfSlerp(alpha, lower, upper);
}

namespace {

struct _LerpVisitor
{
    VtValue &lowerVal;
    VtValue &upperVal;
    double alpha;
    
    // Mutable references to allow stealing. XXX WBN for VtVisitValue to support
    // mutable values.
    _LerpVisitor(VtValue &lower, VtValue &upper, double alpha)
        : lowerVal(lower)
        , upperVal(upper)
        , alpha(alpha) {}
    
    template <class T>
    void operator()(T const &lower) {
        if constexpr (UsdLinearInterpolationTraits<T>::isSupported) {
            lowerVal = Usd_Lerp(alpha, lower, upperVal.Get<T>());
        }
        // else leave lowerVal alone.
    }

    template <class T>
    void operator()(VtArray<T> const &lowerArray) {
        if constexpr (UsdLinearInterpolationTraits<VtArray<T>>::isSupported) {
            // Fall back to held interpolation if sizes don't match. We don't
            // consider this an error because that would be too
            // restrictive. Consumers will be responsible for implementing their
            // own interpolation in cases where this occurs (e.g. meshes with
            // varying topology)
            VtArray<T> upperArray = upperVal.Remove<VtArray<T>>();
            if (lowerArray.size() != upperArray.size()) {
                return;
            }

            if (alpha == 1.0) {
                lowerVal.Swap(upperArray);
                return;
            }

            VtArray<T> result = lowerVal.UncheckedRemove<VtArray<T>>();

            T *resultData = result.data(); // possibly incur COW detach.
            T const *upperData = upperArray.cdata();

            for (size_t i = 0, end = result.size(); i != end; ++i) {
                resultData[i] = Usd_Lerp(alpha, resultData[i], upperData[i]);
            }
            lowerVal.Swap(result);
        }
        // else leave lowerVal alone.
    }
    
    void operator()(VtValue const &lower) {
        // SdfTimeCode isn't a VtValue-known value type, so handle it specially.
        if (lower.IsHolding<SdfTimeCode>()) {
            (*this)(lower.UncheckedGet<SdfTimeCode>());
        }
        else if (lower.IsHolding<VtArray<SdfTimeCode>>()) {
            (*this)(lower.UncheckedGet<VtArray<SdfTimeCode>>());
        }
    }
};

} // anon

// `samples` must not be nullptr.
void
Usd_Interpolate(Usd_InterpolationSampleSeries *samples, double time)
{
    Usd_InterpolationSampleSeries &samps = *samples;
    if (samps.size() <= 1) {
        return;
    }
    if (samps[0].time >= samps[1].time ||
        samps[0].value.GetTypeid() != samps[1].value.GetTypeid()) {
        samps.pop_back();
        return;
    }

    const double alpha =
        (time - samps[0].time) / (samps[1].time - samps[0].time);

    if (alpha == 0.0) {
        samps.pop_back();
        return;
    }

    // It may be tempting to do this, but it's not correct since for values that
    // don't interpolate, alpha = 1.0 should still return the lower sample.  So
    // this optimization is implemented for interpolating types in the
    // _LerpVisitor instead.
    // if (alpha == 1.0) {
    //     samps[0] = std::move(samps[1]);
    //     return;
    // }
    
    VtVisitValue(samps[0].value,
                 _LerpVisitor { samps[0].value, samps[1].value, alpha });
}

template <class LayerOrClipSet>
static bool _GetInterpolatingSamplesImpl(
    LayerOrClipSet const &layerOrClipSet,
    const SdfPath& path,
    double time, double lower, double upper,
    Usd_Interpolator const &interpolator,
    Usd_InterpolationSampleSeries *out)
{
    const bool held =
        interpolator.GetInterpolationType() == UsdInterpolationTypeHeld;
    if (held || GfIsClose(lower, upper, /* epsilon = */ 1e-6)) {
        // In case of held interpolation, lower sample's value will be held and
        // hence directly returned.
        out->resize(1);
        if (ARCH_UNLIKELY(!Usd_QueryTimeSample(
                              layerOrClipSet, path, lower, interpolator,
                              &out->back().value))) {
            out->clear();
            return false;
        }
        out->front().time = lower;
        return true;
    }
    else {
        out->resize(2);
        // In the presence of a value block we use held interpolation. 
        // We know that a failed call to QueryTimeSample is a block
        // because the provided time samples should all have valid values.
        // So this call fails because our <T> is not an SdfValueBlock,
        // which is the type of the contained value.
        if (!Usd_QueryTimeSample(layerOrClipSet, path, lower,
                                 interpolator, &out->front().value)) {
            // lower sample is a block, so we can't interpolate with the upper
            // sample, we have no interpolating samples in this case.
            out->clear();
            return false;
        } 
        if (!Usd_QueryTimeSample(layerOrClipSet, path, upper,
                                 interpolator, &out->back().value)) {
            // upper sample is a block, so we can't interpolate with the lower
            // sample, we fallback to held and just return the lower sample.
            out->pop_back();
            out->front().time = lower;
            return true;
        }
    }
    out->front().time = lower;
    out->back().time = upper;
    return true;
}

bool
Usd_Interpolator::GetInterpolatingSamples(
    const SdfLayerRefPtr& layer, const SdfPath& path,
    double time, double lower, double upper,
    Usd_InterpolationSampleSeries *out) const
{
    return _GetInterpolatingSamplesImpl(
        layer, path, time, lower, upper, *this, out);
}

bool
Usd_Interpolator::GetInterpolatingSamples(
    const Usd_ClipSetRefPtr& clipSet, const SdfPath& path,
    double time, double lower, double upper,
    Usd_InterpolationSampleSeries *out) const
{
    return _GetInterpolatingSamplesImpl(
        clipSet, path, time, lower, upper, *this, out);
}

PXR_NAMESPACE_CLOSE_SCOPE

