//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"
#include "pxr/imaging/hd/timeSampleArray.h"

#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/sdf/types.h"

#include <boost/preprocessor/seq/for_each.hpp>

PXR_NAMESPACE_OPEN_SCOPE

VtValue
HdResampleNeighbors(float alpha, const VtValue &v0, const VtValue &v1)
{
    const TfType t0 = v0.GetType();
    if (!t0) {
        TF_RUNTIME_ERROR("Unknown sample value type '%s'",
                         v0.GetTypeName().c_str());
        return v0;
    }

    const TfType t1 = v1.GetType();
    if (t0 != t1) {
        TF_RUNTIME_ERROR("Mismatched sample value types '%s' and '%s'",
                         v0.GetTypeName().c_str(), v1.GetTypeName().c_str());
        return v0;
    }

    // After verifying that the values have matching types, return the result
    // of HdResampleNeighbors() for the enclosed values.

#define _HANDLE_TYPE(r, unused, type)                                   \
    {                                                                   \
        static const TfType valueType = TfType::Find<type>();           \
        if (t0 == valueType) {                                          \
            const type &val0 = v0.Get<type>();                          \
            const type &val1 = v1.Get<type>();                          \
            return VtValue(HdResampleNeighbors(alpha, val0, val1));     \
        }                                                               \
    }

    BOOST_PP_SEQ_FOR_EACH(_HANDLE_TYPE, ~, USD_LINEAR_INTERPOLATION_TYPES)
#undef _HANDLE_TYPE

    // If the value can't be interpolated, just hold the preceding time
    // sample's value.
    return (alpha < 1.f) ? v0 : v1;
}

PXR_NAMESPACE_CLOSE_SCOPE
