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
#ifndef PXR_USD_USD_VALUE_UTILS_H
#define PXR_USD_USD_VALUE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"

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
    return value && value->valueType == typeid(SdfValueBlock);
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

/// Helper function for setting a value into an SdfAbstractDataValue
/// for generic programming.
template <class T>
inline bool
Usd_SetValue(SdfAbstractDataValue *dv, T const &val)
{ 
    return dv->StoreValue(val); 
}

/// \overload
/// Helper function for setting a value into a VtValue
/// for generic programming.
template <class T>
inline bool
Usd_SetValue(VtValue *value, T const &val)
{ 
    *value = val; 
    return true;
}

/// \overload
/// Helper function for setting a value into a T* from a VtValue
/// for generic programming.
template <class T,
          typename = std::enable_if_t<
              !std::is_same<T, SdfAbstractDataValue>::value &&
              !std::is_same<T, VtValue>::value>>
inline bool
Usd_SetValue(T* value, VtValue const &val)
{
    if (val.IsHolding<T>()) {
        *value = val.UncheckedGet<T>();
        return true;
    }
    return false;
}

enum class Usd_DefaultValueResult 
{
    None = 0,
    Found,
    Blocked,
};

template <class T, class Source>
Usd_DefaultValueResult 
Usd_HasDefault(const Source& source, const SdfPath& specPath, T* value)
{

    if (!value) {
        // Caller is not interested in the value, so avoid fetching it.
        std::type_info const &ti =
            source->GetFieldTypeid(specPath, SdfFieldKeys->Default);
        if (ti == typeid(void)) {
            return Usd_DefaultValueResult::None;
        }
        else if (ti == typeid(SdfValueBlock)) {
            return Usd_DefaultValueResult::Blocked;
        }
        else {
            return Usd_DefaultValueResult::Found;
        }
    }
    else {
        // Caller requests the value.
        if (source->HasField(specPath, SdfFieldKeys->Default, value)) {
            if (Usd_ClearValueIfBlocked(value)) {
                return Usd_DefaultValueResult::Blocked;
            }
            return Usd_DefaultValueResult::Found;
        }
        // fall-through
    }
    return Usd_DefaultValueResult::None;
}

template <class T>
inline bool
Usd_QueryTimeSample(
    const SdfLayerRefPtr& layer, const SdfPath& path,
    double time, Usd_InterpolatorBase* interpolator, T* result)
{
    return layer->QueryTimeSample(path, time, result);
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

// Helper that implements the various options for adding items to lists
// enumerated by UsdListPosition.
//
// If the list op is in explicit mode, the item will be inserted into the
// explicit list regardless of the list specified in the position enum.
//
// If the item already exists in the list, but not in the requested
// position, it will be moved to the requested position.
template <class PROXY>
void
Usd_InsertListItem(PROXY proxy, const typename PROXY::value_type &item,
                   UsdListPosition position)
{
    typename PROXY::ListProxy list(/* unused */ SdfListOpTypeExplicit);
    bool atFront = false;
    switch (position) {
    case UsdListPositionBackOfPrependList:
        list = proxy.GetPrependedItems();
        atFront = false;
        break;
    case UsdListPositionFrontOfPrependList:
        list = proxy.GetPrependedItems();
        atFront = true;
        break;
    case UsdListPositionBackOfAppendList:
        list = proxy.GetAppendedItems();
        atFront = false;
        break;
    case UsdListPositionFrontOfAppendList:
        list = proxy.GetAppendedItems();
        atFront = true;
        break;
    }

    // This function previously used SdfListEditorProxy::Add, which would
    // update the explicit list if the list op was in explicit mode. Clients
    // currently expect this behavior, so we need to maintain it regardless
    // of the list specified in the postiion enum.
    if (proxy.IsExplicit()) {
        list = proxy.GetExplicitItems();
    }

    if (list.empty()) {
        list.Insert(-1, item);
    } else {
        const size_t pos = list.Find(item);
        if (pos != size_t(-1)) {
            const size_t targetPos = atFront ? 0 : list.size()-1;
            if (pos == targetPos) {
                // Item already exists in the right position.
                return;
            }
            list.Erase(pos);
        }
        list.Insert(atFront ? 0 : -1, item);
    }
}

/// Resolves all the individual values in the given dictionary using the given
/// resolve function.
/// Fn type is equivalent to:
///     void resolveFunc(VtValue *)
template <typename Fn>
void
Usd_ResolveValuesInDictionary(VtDictionary *dict, const Fn &resolveFunc)
{
    for (auto& entry : *dict) {
        VtValue& v = entry.second;
        if (v.IsHolding<VtDictionary>()) {
            VtDictionary resolvedDict;
            v.UncheckedSwap(resolvedDict);
            Usd_ResolveValuesInDictionary(&resolvedDict, resolveFunc);
            v.UncheckedSwap(resolvedDict);
        }
        else {
            resolveFunc(&v);
        }
    }
}

/// Apply the given layer \p offset to the given \p value if the value holds
/// a type a that can be offset it time. Each supported type haa an overload 
/// of this function defined.
void
Usd_ApplyLayerOffsetToValue(VtValue *value, const SdfLayerOffset &offset);

/// \overload
inline void
Usd_ApplyLayerOffsetToValue(SdfTimeCode *value, const SdfLayerOffset &offset)
{
    *value = offset * (*value);
}

/// \overload
inline void
Usd_ApplyLayerOffsetToValue(VtArray<SdfTimeCode> *value, 
                            const SdfLayerOffset &offset)
{
    for (SdfTimeCode &timeCode : *value) {
        timeCode = offset * timeCode;
    }
}

/// \overload
inline void
Usd_ApplyLayerOffsetToValue(SdfTimeSampleMap *value, 
                            const SdfLayerOffset &offset)
{
    // Swap the original map so we can write new values back into the original
    // value.
    SdfTimeSampleMap origValue;
    std::swap(origValue, *value);
    for (const auto& sample : origValue) {
        // Each time sample key must be mapped by the layer offset.
        VtValue &newSample = (*value)[offset * sample.first];
        newSample = std::move(sample.second);
        // The value may also have be mapped if it is time mappable.
        Usd_ApplyLayerOffsetToValue(&newSample, offset);
    }
}

/// \overload
inline void
Usd_ApplyLayerOffsetToValue(VtDictionary *value, const SdfLayerOffset &offset)
{
    Usd_ResolveValuesInDictionary(value, 
        [&offset](VtValue *v) 
        {
             Usd_ApplyLayerOffsetToValue(v, offset);
        });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_VALUE_UTILS_H
