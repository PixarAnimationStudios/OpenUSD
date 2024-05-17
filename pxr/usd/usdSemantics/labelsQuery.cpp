//
// Copyright 2024 Pixar
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

#include "pxr/usd/usdSemantics/labelsQuery.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

UsdSemanticsLabelsQuery::UsdSemanticsLabelsQuery(const TfToken& taxonomy,
                                                 UsdTimeCode timeCode) :
    _taxonomy(taxonomy), _time(timeCode) {
    TF_VERIFY(!taxonomy.IsEmpty());
}

UsdSemanticsLabelsQuery::UsdSemanticsLabelsQuery(const TfToken& taxonomy,
                                                 const GfInterval& interval) :
    _taxonomy(taxonomy), _time(interval) {
    TF_VERIFY(!taxonomy.IsEmpty());
    if (!TF_VERIFY(!interval.IsEmpty())) {
        _time = UsdTimeCode::Default();
    }
}

bool
UsdSemanticsLabelsQuery::_PopulateLabels(const UsdPrim& prim) {
    if (prim.IsPseudoRoot()) {
        return false;
    }
    UsdSemanticsLabelsAPI schema(prim, _taxonomy);
    if (!schema) {
        return false;
    }

    auto doubleCheckedLockCondition = [&prim, this]() {
        return _cachedLabels.count(prim.GetPath()) > 0;
    };
    {
        std::shared_lock lock{_cachedLabelsMutex};
        if (doubleCheckedLockCondition()) {
            return true;
        }
    }

    // Avoid locking while we compute
    auto labels = std::visit([&schema](auto&& queryTime) -> _UnorderedTokenSet{
        using TimeType = std::decay_t<decltype(queryTime)>;
        const auto labelsAttr = schema.GetLabelsAttr();
        if (!labelsAttr) {
            TF_RUNTIME_ERROR("Labels attribute undefined at %s",
                             UsdDescribe(schema.GetPrim()).c_str());
            return {};
        }
        if constexpr (std::is_same_v<TimeType, UsdTimeCode>) {
            VtTokenArray labelsAtTime;
            if (!labelsAttr.Get(&labelsAtTime, queryTime)) {
                TF_RUNTIME_ERROR(
                    "Failed to read tokens from %s",
                    UsdDescribe(labelsAttr).c_str());
            }
            return {std::cbegin(labelsAtTime), std::cend(labelsAtTime)};
        }
        if constexpr (std::is_same_v<TimeType, GfInterval>) {
            TF_DEV_AXIOM(!queryTime.IsEmpty());
            std::vector<double> times;
            if (!labelsAttr.GetTimeSamplesInInterval(
                queryTime, &times)) {
                TF_RUNTIME_ERROR("Failed to retrieve time samples at %s",
                                 UsdDescribe(labelsAttr).c_str());
                return {};
            }
            if (!TF_VERIFY(std::all_of(std::cbegin(times), std::cend(times),
                           [](const auto t) {
                                return t >= UsdTimeCode::EarliestTime(); }))
                           ) {
                return {};
            }
            // Ensure that the queryTime.GetMin() is considered if finite.
            // If it's not, add the earliest time sample.
            const double earliest =
                queryTime.IsMinFinite() ? queryTime.GetMin() :
                UsdTimeCode::EarliestTime().GetValue();
            if (times.empty() || (times.front() != earliest)) {
                times.push_back(earliest);
            }

            // Times is guaranteed to have interval.GetMin() or
            // UsdTimeCode::EarliestTime.  If no time samples are authored,
            // querying at this time sample will return default or fallback
            // values. No special handling required.
            TF_DEV_AXIOM(!times.empty());

            _UnorderedTokenSet uniqueLabels;
            for (const auto timeInInterval : times) {
                VtTokenArray labelsAtTime;
                if (!labelsAttr.Get(&labelsAtTime, timeInInterval)) {
                    TF_RUNTIME_ERROR("Failed to read value at %s",
                                     UsdDescribe(labelsAttr).c_str());
                    return {};
                }
                uniqueLabels.insert(
                    std::cbegin(labelsAtTime), std::cend(labelsAtTime));
            }
            return uniqueLabels;
        }
    }, _time);

    std::unique_lock lock{_cachedLabelsMutex};
    // If another thread has already computed the cached labels, discard the
    // results.
    if (!doubleCheckedLockCondition()) {
        _cachedLabels.emplace(prim.GetPath(), std::move(labels));
    }
    return true;
}

bool
UsdSemanticsLabelsQuery::_PopulateInheritedLabels(const UsdPrim& prim) {
    const auto stage = prim.GetStage();
    bool hasInheritedLabel = false;
    for (const auto& path : prim.GetPath().GetAncestorsRange()) {
        // Note that _PopulateLabels must run every ancestor to update the
        // cache. Attempting to collapse this expression using |= or the
        // STL may result in some population being skipped and incorrect
        // results.
        if (_PopulateLabels(stage->GetPrimAtPath(path)))
            hasInheritedLabel = true;
    }
    return hasInheritedLabel;
}

VtTokenArray
UsdSemanticsLabelsQuery::ComputeUniqueDirectLabels(const UsdPrim& prim)
{
    // If the prim was not labeled, we can early exit without
    // locking.
    if (!_PopulateLabels(prim)) {
        return {};
    }

    std::shared_lock lock{_cachedLabelsMutex};
    const auto it = _cachedLabels.find(prim.GetPath());
    if (it == std::cend(_cachedLabels)) {
        return {};
    }
    VtTokenArray result{std::cbegin(it->second), std::cend(it->second)};
    std::sort(result.begin(), result.end());
    return result;
}

VtTokenArray
UsdSemanticsLabelsQuery::ComputeUniqueInheritedLabels(const UsdPrim& prim)
{
    // If no ancestors were labeled, we can early exit without
    // locking.
    if (!_PopulateInheritedLabels(prim)) {
        return {};
    }

    _UnorderedTokenSet uniqueElements;
    {
        std::shared_lock lock{_cachedLabelsMutex};
        for (const auto& path : prim.GetPath().GetAncestorsRange()) {
            const auto it = _cachedLabels.find(path);
            if (it != std::cend(_cachedLabels)) {
                uniqueElements.insert(std::cbegin(it->second), std::cend(it->second));
            }
        }
    }
    VtTokenArray result{std::cbegin(uniqueElements), std::cend(uniqueElements)};
    std::sort(result.begin(), result.end());
    return result;
}

bool
UsdSemanticsLabelsQuery::HasDirectLabel(const UsdPrim& prim, const TfToken& label)
{
    // If the prim was not labeled, we can early exit without
    // locking.
    if (!_PopulateLabels(prim)) {
        return false;
    }

    std::shared_lock lock{_cachedLabelsMutex};
    const auto it = _cachedLabels.find(prim.GetPath());
    return (it != std::cend(_cachedLabels)) &&
           (it->second.count(label) > 0);
}
    
bool
UsdSemanticsLabelsQuery::HasInheritedLabel(const UsdPrim& prim,
                                           const TfToken& label)
{
    // If no ancestors or this were labeled, we can early exit without
    // locking.
    if (!_PopulateInheritedLabels(prim)) {
        return false;
    }

    std::shared_lock lock{_cachedLabelsMutex};
    auto range = prim.GetPath().GetAncestorsRange();
    return std::any_of(
        std::cbegin(range), std::cend(range),
        [this, &label](const auto& path) {
            const auto it = _cachedLabels.find(path);
            return ((it != std::cend(_cachedLabels)) &&
                    (it->second.count(label) > 0));
    });
}

PXR_NAMESPACE_CLOSE_SCOPE