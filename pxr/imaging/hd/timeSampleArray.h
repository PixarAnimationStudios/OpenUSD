//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_TIME_SAMPLE_ARRAY_H
#define PXR_IMAGING_HD_TIME_SAMPLE_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Resample two neighboring samples.
template <typename T>
inline T HdResampleNeighbors(float alpha, const T& v0, const T& v1)
{
    return GfLerp(alpha, v0, v1);
}

/// Specialization for HdQuatf: spherical linear interpolation.
HD_API
inline GfQuatf HdResampleNeighbors(float alpha,
                                   const GfQuatf &v0,
                                   const GfQuatf &v1)
{
    return GfSlerp(double(alpha), v0, v1);
}

/// Specialization for VtArray: component-wise resampling.
template <typename T>
inline VtArray<T> HdResampleNeighbors(float alpha,
                                      const VtArray<T>& v0,
                                      const VtArray<T>& v1)
{
    VtArray<T> r(v0.size());
    for (size_t i=0; i < r.size(); ++i) {
        r[i] = HdResampleNeighbors(alpha, v0[i], v1[i]);
    }
    return r;
}

/// Specialization for VtValue: interpolate the held values.
HD_API
VtValue HdResampleNeighbors(float alpha, const VtValue& v0, const VtValue& v1);

/// Resample a function described by an ordered array of samples,
/// using a linear reconstruction filter evaluated at the given
/// parametric position u.  The function is considered constant
/// outside the supplied sample range.
template <typename T>
T HdResampleRawTimeSamples(
    float u, 
    size_t numSamples,
    const float *us, 
    const T *vs)
{
    if (numSamples == 0) {
        TF_CODING_ERROR("HdResampleRawTimeSamples: Zero samples provided");
        return T();
    }

    size_t i=0;
    for (; i < numSamples; ++i) {
        if (us[i] == u) {
            // Fast path for exact parameter match.
            return vs[i];
        }
        if (us[i] > u) {
            break;
        }
    }
    if (i == 0) {
        // u is before the first sample.
        return vs[0];
    } else if (i == numSamples) {
        // u is after the last sample.
        return vs[numSamples-1];
    } else if (us[i] == us[i-1]) {
        // Neighboring samples have identical parameter.
        // Arbitrarily choose a sample.
        TF_WARN("HdResampleRawTimeSamples: overlapping samples at %f; "
                "using first sample", us[i]);
        return vs[i-1];
    } else {
        // Linear blend of neighboring samples.
        float alpha = (u-us[i-1]) / (us[i]-us[i-1]);
        return HdResampleNeighbors(alpha, vs[i-1], vs[i]);
    }
}

/// Resample a function described by an ordered array of samples and sample
/// indices, using a linear reconstruction filter evaluated at the given
/// parametric position u.  The function is considered constant outside the 
/// supplied sample range.
template <typename T>
std::pair<T, VtIntArray> HdResampleRawTimeSamples(
    float u, 
    size_t numSamples,
    const float *us, 
    const T *vs,
    const VtIntArray *is)
{
    if (numSamples == 0) {
        TF_CODING_ERROR("HdResampleRawTimeSamples: Zero samples provided");
        return std::pair<T, VtIntArray>(T(), VtIntArray(0));
    }

    size_t i=0;
    for (; i < numSamples; ++i) {
        if (us[i] == u) {
            // Fast path for exact parameter match.
            return std::pair<T, VtIntArray>(vs[i], is[i]);
        }
        if (us[i] > u) {
            break;
        }
    }
    if (i == 0) {
        // u is before the first sample.
        return std::pair<T, VtIntArray>(vs[0], is[0]);
    } else if (i == numSamples) {
        // u is after the last sample.
        return std::pair<T, VtIntArray>(vs[numSamples-1], is[numSamples-1]);
    } else if (us[i] == us[i-1]) {
        // Neighboring samples have identical parameter.
        // Arbitrarily choose a sample.
        TF_WARN("HdResampleRawTimeSamples: overlapping samples at %f; "
                "using first sample", us[i]);
        return std::pair<T, VtIntArray>(vs[i-1], is[i-1]);
    } else {
        // Linear blend of neighboring samples for values
        // Hold earlier value for indices
        float alpha = (us[i]-u) / (us[i]-us[i-1]);
        return std::pair<T, VtIntArray>(
            HdResampleNeighbors(alpha, vs[i-1], vs[i]),
            is[i-1]);
    }
}

// Returns contributing sample times for the interval from startTime to endTime.
//
// If there is no sample at the startTime, this will include the sample times
// just before the start time if it exists. Similarly for the endTime.
//
// Return true if the value is changing on the interval from startTime to
// endTime - or equivalently if we return two times.
bool
HdGetContributingSampleTimesForInterval(
    size_t count,
    const float * sampleTimes,
    float startTime,
    float endTime,
    std::vector<float> * outSampleTimes);

/// An array of a value sampled over time, in struct-of-arrays layout.
/// This is provided as a convenience for time-sampling attributes.
/// This type has static capacity but dynamic size, providing
/// a limited ability to handle variable sampling without requiring
/// heap allocation.
template<typename TYPE, unsigned int CAPACITY>
struct HdTimeSampleArray 
{
    HdTimeSampleArray() {
        times.resize(CAPACITY);
        values.resize(CAPACITY);
        count = 0;
    }

    HdTimeSampleArray(const HdTimeSampleArray& rhs) {
        times = rhs.times;
        values = rhs.values;
        count = rhs.count;
    }

    HdTimeSampleArray& operator=(const HdTimeSampleArray& rhs) {
        times = rhs.times;
        values = rhs.values;
        count = rhs.count;
        return *this;
    }

    /// Resize the internal buffers.
    virtual void Resize(unsigned int newSize) {
        times.resize(newSize);
        values.resize(newSize);
        count = newSize;
    }

    /// Convience method for invoking HdResampleRawTimeSamples
    /// on this HdTimeSampleArray.
    TYPE Resample(float u) const {
        return HdResampleRawTimeSamples(u, count, times.data(), values.data());
    }

    /// Unbox an HdTimeSampleArray holding boxed VtValue<VtArray<T>>
    /// samples into an array holding VtArray<T> samples. If any of the values 
    /// contain the wrong type, their data is discarded. The function returns
    /// true if all samples have the correct type.
    bool UnboxFrom(HdTimeSampleArray<VtValue, CAPACITY> const& box) {
        bool ret = true;
        Resize(box.count);
        times = box.times;
        for (size_t i=0; i < box.count; ++i) {
            if (box.values[i].template IsHolding<TYPE>() &&
                box.values[i].GetArraySize() > 0) {
                values[i] = box.values[i].template Get<TYPE>();
            } else {
                values[i] = TYPE();
                ret = false;
            }
        }
        return ret;
    }

    /// See HdGetContributingSampleTimesForInterval.
    bool GetContributingSampleTimesForInterval(
        const float startTime, const float endTime,
        std::vector<float> * const outSampleTimes) const
    {
        return HdGetContributingSampleTimesForInterval(
            count, times.data(), startTime, endTime, outSampleTimes);
    }

    size_t count;
    TfSmallVector<float, CAPACITY> times;
    TfSmallVector<TYPE, CAPACITY> values;
};

/// An array of a value and its indices sampled over time, in struct-of-arrays 
/// layout.
template<typename TYPE, unsigned int CAPACITY>
struct HdIndexedTimeSampleArray : public HdTimeSampleArray<TYPE, CAPACITY>
{
    HdIndexedTimeSampleArray() : HdTimeSampleArray<TYPE, CAPACITY>() {
        indices.resize(CAPACITY);
    }

    HdIndexedTimeSampleArray(const HdIndexedTimeSampleArray& rhs) : 
        HdTimeSampleArray<TYPE, CAPACITY>(rhs) {
        indices = rhs.indices;
    }

    HdIndexedTimeSampleArray& 
    operator=(const HdIndexedTimeSampleArray& rhs) {
        this->times = rhs.times;
        this->values = rhs.values;
        this->count = rhs.count;
        indices = rhs.indices;
        return *this;
    }

    /// Resize the internal buffers.
    void Resize(unsigned int newSize) override {
        HdTimeSampleArray<TYPE, CAPACITY>::Resize(newSize);
        indices.resize(newSize);
    }

    /// Convience method for invoking HdResampleRawTimeSamples
    /// on this HdIndexedTimeSampleArray.
    std::pair<TYPE, VtIntArray> ResampleIndexed(float u) const {
        return HdResampleRawTimeSamples(u, this->count, this->times.data(), 
                                        this->values.data(), indices.data());
    }

    /// Unbox an HdIndexedTimeSampleArray holding boxed VtValue<VtArray<T>>
    /// samples into an array holding VtArray<T> samples. If any of the values 
    /// contain the wrong type, their data is discarded. The function returns
    /// true if all samples have the correct type.
    bool UnboxFrom(HdIndexedTimeSampleArray<VtValue, CAPACITY> const& box) {
        bool ret = true;
        Resize(box.count);
        this->times = box.times;
        indices = box.indices;
        for (size_t i=0; i < box.count; ++i) {
            if (box.values[i].template IsHolding<TYPE>() &&
                box.values[i].GetArraySize() > 0) {
                this->values[i] = box.values[i].template Get<TYPE>();
            } else {
                this->values[i] = TYPE();
                ret = false;
            }
        }
        return ret;
    }

    TfSmallVector<VtIntArray, CAPACITY> indices;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_TIME_SAMPLE_ARRAY_H
