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
#ifndef HD_TIME_SAMPLE_ARRAY_H
#define HD_TIME_SAMPLE_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Resample two neighboring samples.
template <typename T>
inline T HdResampleNeighbors(float alpha, const T& v0, const T& v1)
{
    return GfLerp(alpha, v0, v1);
}

/// Specialization for HdQuatf: spherical linear interpolation.
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
    for (int i=0; i < r.size(); ++i) {
        r[i] = HdResampleNeighbors(alpha, v0[i], v1[i]);
    }
    return r;
}

/// Resample a function described by an ordered array of samples,
/// using a linear reconstruction filter evaluated at the given
/// parametric position u.  The function is considered constant
/// outside the supplied sample range.
template <typename T>
T HdResampleRawTimeSamples(float u, size_t numSamples,
                           const float *us, const T *vs)
{
    if (numSamples == 0) {
        TF_CODING_ERROR("HdResample: Zero samples provided");
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
        float alpha = (us[i]-u) / (us[i]-us[i-1]);
        return HdResampleNeighbors(alpha, vs[i-1], vs[i]);
    }
}

/// An array of a value sampled over time, in struct-of-arrays layout.
/// This is provided as a convenience for time-sampling attributes.
/// This type has static capacity but dynamic size, providing
/// a limited ability to handle variable sampling without requiring
/// heap allocation.
template<typename TYPE, unsigned int CAPACITY>
struct HdTimeSampleArray {
    /// Static maximum capacity.
    static const unsigned int capacity = CAPACITY;
    /// Sample times, ordered by increasing time.
    float times[CAPACITY];
    /// Sample values, corresponding to the times[] array.
    TYPE values[CAPACITY];
    /// Count of how many samples are stored.  0 <= count <= CAPACITY.
    unsigned int count;

    /// Convience method for invoking HdResampleRawTimeSamples
    /// on this HdTimeSampleArray.
    TYPE Resample(float u) const {
        return HdResampleRawTimeSamples(u, count, times, values);
    }

    /// Unbox an HdTimeSampleArray holding boxed VtValue<VtArray<T>>
    /// samples into an aray holding VtArray<T> samples.
    ///
    /// Similar to VtValue::Get(), this will issue a coding error if the
    /// VtValue is not holding the expected type.
    ///
    /// \see VtValue::Get()
    void UnboxFrom(HdTimeSampleArray<VtValue,CAPACITY> const& box) {
        count = box.count;
        for (size_t i=0; i < box.count; ++i) {
            times[i] = box.times[i];
            if (box.values[i].GetArraySize() > 0) {
                values[i] = box.values[i].template Get<TYPE>();
            } else {
                values[i] = TYPE();
            }
        }
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_TIME_SAMPLE_ARRAY_H
