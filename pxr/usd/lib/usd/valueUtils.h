//
// Copyright 2017 Pixar
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
#ifndef USD_VALUE_UTILS_H
#define USD_VALUE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/clip.h"

#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class Usd_InterpolatorBase;

/// Returns true if \p value contains an SdfValueBlock, false otherwise.
template <class T>
inline bool
Usd_ValueContainsBlock(const T* value)
{
    return false;
}

/// \overload
template <class T>
inline bool
Usd_ValueContainsBlock(const SdfValueBlock* value)
{
    return value;
}

/// \overload
inline bool 
Usd_ValueContainsBlock(const VtValue* value) 
{
    return value && value->IsHolding<SdfValueBlock>();
}

/// \overload
inline bool
Usd_ValueContainsBlock(const SdfAbstractDataValue* value) 
{
    return value && value->isValueBlock;
}

/// \overload
inline bool
Usd_ValueContainsBlock(const SdfAbstractDataConstValue* value)
{
    const std::type_info& valueBlockTypeId(typeid(SdfValueBlock));
    return value && value->valueType == valueBlockTypeId;
}

/// If \p value contains an SdfValueBlock, clear the value and
/// return true. Otherwise return false.
template <class T>
inline bool
Usd_ClearValueIfBlocked(T* value)
{
    // We can't actually clear the value here, since there's
    // no good API for doing so. If the value is holding a
    // block, we just return true and rely on the consumer
    // to act as if the value were cleared.
    return Usd_ValueContainsBlock(value);
}

/// \overload
inline bool 
Usd_ClearValueIfBlocked(VtValue* value) 
{
    if (Usd_ValueContainsBlock(value)) {
        *value = VtValue();
        return true;
    }

    return false;
}

template <class T>
inline bool
Usd_QueryTimeSample(
    const SdfLayerRefPtr& layer, const SdfAbstractDataSpecId& specId,
    double time, Usd_InterpolatorBase* interpolator, T* result)
{
    return layer->QueryTimeSample(specId, time, result);
}

template <class T>
inline bool
Usd_QueryTimeSample(
    const Usd_ClipRefPtr& clip, const SdfAbstractDataSpecId& specId,
    double time, Usd_InterpolatorBase* interpolator, T* result)
{
    return clip->QueryTimeSample(specId, time, interpolator, result);
}

/// Merges sample times in \p additionalTimeSamples into the vector pointed to 
/// by \p timeSamples. This assumes that the values in \p timeSamples and
/// \p additionalTimeSamples are already sorted.
/// 
/// If \p tempUnionSampleTimes is not null, it is used as temporary storage in 
/// the call to std::set_union, to hold the union of the two vectors.
void
Usd_MergeTimeSamples(std::vector<double> * const timeSamples, 
                     const std::vector<double> &additionalTimeSamples,
                     std::vector<double> * tempUnionTimeSamples=nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_VALUE_UTILS_H
