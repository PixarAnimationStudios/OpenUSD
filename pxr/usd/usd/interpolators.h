//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_INTERPOLATORS_H
#define PXR_USD_USD_INTERPOLATORS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/clipSet.h"
#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/usd/valueUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/smallVector.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;

// Helper struct to bundle a value and a time.
struct Usd_ValueTimeSample
{
    VtValue value;
    double time;

    friend std::ostream &
    operator<<(std::ostream &, Usd_ValueTimeSample const &);
};

// A helper struct for composing interpolating samples during value resolution.
// Typically we're interpolating two samples, so use local capacity in that
// case.
using Usd_InterpolationSampleSeries = TfSmallVector<Usd_ValueTimeSample, 2>;

// Linearly interpolate samples wrt time if possible and leave the result as
// samples[0].  If samples.size() is 1 or the sample values cannot be
// interpolated, just pop_back() `samples`.  The `samples` argument must not be
// nullptr.
USD_API
void
Usd_Interpolate(Usd_InterpolationSampleSeries *samples, double time);

/// \class Usd_Interpolator
///
/// Interpolator used to fetch interpolating samples (or held samples) from
/// either a clipSet or a layer.
///
class Usd_Interpolator
{
public:
    explicit Usd_Interpolator(
        UsdInterpolationType interpolationType = UsdInterpolationTypeLinear)
        : _interpolationType(interpolationType) {}

    // Fetch up to two interpolating samples from `layer` for `path` at `time`
    // with bracketing times `lower` and `upper`.
    //
    // If the `interpolationType` is Held, only fetch up to one sample.  If the
    // held or lower sample is a block, clear `out`. Otherwise if
    // `interpolationType` is Held or `lower` and `upper` are the same (within a
    // tolerance) or the sample at `upper` is a block, return the single `lower`
    // sample in `out`.
    //
    // Otherwise return two samples in `out` corresponding to `lower` and
    // `upper` to be interpolated.
    USD_API
    bool GetInterpolatingSamples(
        const SdfLayerRefPtr& layer, const SdfPath& path,
        double time, double lower, double upper,
        Usd_InterpolationSampleSeries *out) const;

    // Same as the above, except fetch samples from the `clipSet` instead.
    USD_API
    bool GetInterpolatingSamples(
        const Usd_ClipSetRefPtr& clipSet, const SdfPath& path,
        double time, double lower, double upper,
        Usd_InterpolationSampleSeries *out) const;

    UsdInterpolationType GetInterpolationType() const {
        return _interpolationType;
    }
    
private:
    UsdInterpolationType _interpolationType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INTERPOLATORS_H
