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
#ifndef GF_OSTREAM_HELPERS_H
#define GF_OSTREAM_HELPERS_H

/// \file gf/ostreamHelpers.h
/// \ingroup group_gf_DebuggingOutput
///
/// Helpers for Gf stream operators.
///
/// These functions are useful to help with writing stream operators for
/// Gf types.  Please do not include this file in any header.

#include <iostream>
#include <limits>

// Helper class to write out the correct precision
template <class T>
struct Gf_OstreamHelperValue {
    explicit Gf_OstreamHelperValue(T v) : value(v) {}
    T value;
};

// Make the template class general so that we can use the same class
// (Gf_OstreamHelperP) on all variables and not worry about making a mistake
template <class T>
T Gf_OstreamHelperP(T v) { return v; }

inline Gf_OstreamHelperValue<float> Gf_OstreamHelperP(float v) { 
    return Gf_OstreamHelperValue<float>(v); 
}
inline Gf_OstreamHelperValue<double> Gf_OstreamHelperP(double v) { 
    return Gf_OstreamHelperValue<double>(v); 
}

// Helper functions to write out floats / doubles with the correct
// precision.  Copied from Tf/StringUtils.cpp TfStringify.  See
// comment in that function regarding precision.

inline std::ostream &
operator<<(std::ostream &out, const Gf_OstreamHelperValue<float> &data)
{
    int oldPrecision = out.precision(std::numeric_limits<float>::digits10);
    out << data.value;
    out.precision(oldPrecision);
    return out;
}

inline std::ostream &
operator<<(std::ostream &out, const Gf_OstreamHelperValue<double> &data)
{
    int oldPrecision = out.precision(std::numeric_limits<double>::digits10);
    out << data.value;
    out.precision(oldPrecision);
    return out;
}

#endif /* GF_OSTREAM_HELPERS */
