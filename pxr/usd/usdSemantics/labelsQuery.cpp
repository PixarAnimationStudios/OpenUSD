//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSemantics/labelsQuery.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

UsdSemanticsLabelsQuery::UsdSemanticsLabelsQuery(const TfToken& taxonomy,
                                                 UsdTimeCode timeCode) :
    _taxonomy(taxonomy), _time(timeCode) {
    if (taxonomy.IsEmpty()) {
        TF_CODING_ERROR("UsdSemanticsLabelsQuery created with empty taxonomy.");
    }
}

UsdSemanticsLabelsQuery::UsdSemanticsLabelsQuery(const TfToken& taxonomy,
                                                 const GfInterval& interval) :
    _taxonomy(taxonomy), _time(interval) {

    if (taxonomy.IsEmpty()) {
        TF_CODING_ERROR("UsdSemanticsLabelsQuery created with empty taxonomy.");
    }

    if (interval.IsEmpty()) {
        TF_CODING_ERROR("UsdSemanticsLabelsQuery created with empty interval.");
        _time = UsdTimeCode::Default();
    }
}

bool
UsdSemanticsLabelsQuery::_PopulateLabels(const UsdPrim& prim) const {
    if (prim.IsPseudoRoot()) {
        return false;
    }

    if (!prim.HasAPI<UsdSemanticsLabelsAPI>(_taxonomy)) {
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
            TF_WARN("Labels attribute undefined at %s",
                             UsdDescribe(schema.GetPrim()).c_str());
            return {};
        }
        if constexpr (std::is_same_v<TimeType, UsdTimeCode>) {
            VtTokenArray labelsAtTime;
            if (!labelsAttr.Get(&labelsAtTime, queryTime)) {
                TF_WARN(
                    "Failed to read tokens from %s",
                    UsdDescribe(labelsAttr).c_str());
            }
            return {std::cbegin(labelsAtTime), std::cend(labelsAtTime)};
        }
        if constexpr (std::is_same_v<TimeType, GfInterval>) {
            TF_VERIFY(!queryTime.IsEmpty());
            std::vector<double> times;
            if (!labelsAttr.GetTimeSamplesInInterval(
                queryTime, &times)) {
                TF_WARN("Failed to retrieve time samples at %s",
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

            _UnorderedTokenSet uniqueLabels;
            for (const auto timeInInterval : times) {
                VtTokenArray labelsAtTime;
                if (!labelsAttr.Get(&labelsAtTime, timeInInterval)) {
                    TF_WARN("Failed to read value at %s",
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
    const auto [iter, emplaced] = 
        _cachedLabels.emplace(prim.GetPath(), _UnorderedTokenSet {});
    if (emplaced) {
        iter->second = std::move(labels);
    }
    return true;
}

bool
UsdSemanticsLabelsQuery::_PopulateInheritedLabels(const UsdPrim& prim) const {
    const auto stage = prim.GetStage();
    bool hasInheritedLabel = false;
    for (const auto& path : prim.GetPath().GetAncestorsRange()) {
        // Note that _PopulateLabels must run every ancestor to update the
        // cache. Attempting to collapse this expression using |= or the
        // STL may result in some population being skipped and incorrect
        // results.
        if (_PopulateLabels(stage->GetPrimAtPath(path))) {
            hasInheritedLabel = true;
        }
    }
    return hasInheritedLabel;
}

VtTokenArray
UsdSemanticsLabelsQuery::ComputeUniqueDirectLabels(const UsdPrim& prim) const
{
    // If the prim was not labeled, we can early exit without
    // locking.
    if (!_PopulateLabels(prim)) {
        return {};
    }

    std::shared_lock lock{_cachedLabelsMutex};
    const auto it = std::as_const(_cachedLabels).find(prim.GetPath());
    if (it == std::cend(_cachedLabels)) {
        return {};
    }
    VtTokenArray result{std::cbegin(it->second), std::cend(it->second)};
    std::sort(result.begin(), result.end());
    return result;
}

VtTokenArray
UsdSemanticsLabelsQuery::ComputeUniqueInheritedLabels(const UsdPrim& prim) const
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
                uniqueElements.insert(
                    std::cbegin(it->second), std::cend(it->second));
            }
        }
    }
    VtTokenArray result{std::cbegin(uniqueElements), std::cend(uniqueElements)};
    std::sort(result.begin(), result.end());
    return result;
}

bool
UsdSemanticsLabelsQuery::HasDirectLabel(const UsdPrim& prim, 
                                        const TfToken& label) const
{
    // If the prim was not labeled, we can early exit without
    // locking.
    if (!_PopulateLabels(prim)) {
        return false;
    }

    std::shared_lock lock{_cachedLabelsMutex};
    const auto it = _cachedLabels.find(prim.GetPath());
    return (it != std::cend(_cachedLabels)) &&
           (it->second.count(label));
}
    
bool
UsdSemanticsLabelsQuery::HasInheritedLabel(const UsdPrim& prim,
                                           const TfToken& label) const
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
                    (it->second.count(label)));
    });
}

PXR_NAMESPACE_CLOSE_SCOPE