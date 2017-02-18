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

#include "pxr/pxr.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

// Make the template class general so that we can use the same class
// (Gf_OstreamHelperP) on all variables and not worry about making a mistake
template <class T>
T Gf_OstreamHelperP(T v) { return v; }

inline TfStreamFloat Gf_OstreamHelperP(float v) { 
    return TfStreamFloat(v); 
}
inline TfStreamDouble Gf_OstreamHelperP(double v) { 
    return TfStreamDouble(v); 
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // GF_OSTREAM_HELPERS 
