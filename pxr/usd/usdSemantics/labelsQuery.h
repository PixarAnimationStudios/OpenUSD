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

#ifndef PXR_USDSEMANTICS_LABELSQUERY_H
#define PXR_USDSEMANTICS_LABELSQUERY_H

#include "pxr/pxr.h"

#include "pxr/usd/usdSemantics/labelsAPI.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/token.h"

#include <shared_mutex>
#include <set>
#include <unordered_map>
#include <variant>


PXR_NAMESPACE_OPEN_SCOPE

/// The `UsdSemanticsLabelsQuery` can be used to query a prim's
/// labels for a specified taxonomy and time from the `UsdSemanticsLabelsAPI`.
/// Time may be an individual time code or an interval.
///
/// This utility requires that all prims are on the same stage.
///
/// The query caches certain reads and computations and should be discarded
/// when the state of the stage changes. Queries are thread safe.
class UsdSemanticsLabelsQuery
{
public:
    UsdSemanticsLabelsQuery(const UsdSemanticsLabelsQuery&) = delete;
    UsdSemanticsLabelsQuery& operator=(
        const UsdSemanticsLabelsQuery&) = delete;

    /// Constructs a query for a `taxonomy` at a single `timeCode`.
    /// 
    /// Requires that the `taxonomy` must not be empty.
    USDSEMANTICS_API UsdSemanticsLabelsQuery(const TfToken& taxonomy,
                                             UsdTimeCode timeCode);

    /// Construct a query for a `taxonomy` over an `interval`.
    ///
    /// Requires that neither the `interval` nor `taxonomy` to be empty.
    ///
    /// \warning Finite minimum values of the interval will return the same
    /// result regardless of closed or open state due to held interpolation
    /// semantics and Zeno's paradox.
    USDSEMANTICS_API UsdSemanticsLabelsQuery(const TfToken& taxonomy,
                                             const GfInterval& interval);

    /// Compute the union of values for `semantics:labels:<taxonomy>` in the
    /// specified interval directly applied to this prim.
    ///
    /// The returned order is the determnistic ordering of `TfToken`.
    ///
    /// If no time samples are authored, the default and fallback values will
    /// be queried.
    ///
    /// \sa UsdSemanticsLabelsQuery::ComputeUniqueInheritedLabels
    USDSEMANTICS_API
    VtTokenArray ComputeUniqueDirectLabels(const UsdPrim& prim);

    /// Compute the union of values for `semantics:labels:<taxonomy>` in the
    /// specified interval including any labels inherited from ancestors.
    ///
    /// The returned order is the determnistic ordering of `TfToken`.
    ///
    /// If no time samples are authored, the default and fallback values of
    /// the prim and every ancestor will be queried.
    ///
    /// \sa UsdSemanticsLabelsQuery::ComputeUniqueDirectLabels
    USDSEMANTICS_API
    VtTokenArray ComputeUniqueInheritedLabels(const UsdPrim& prim);

    /// Return true if a label has been specified directly on this prim for
    /// this query's taxonomy and time
    ///
    /// \sa UsdSemanticsLabelsQuery::HasInheritedLabel
    USDSEMANTICS_API
    bool HasDirectLabel(const UsdPrim& prim, const TfToken& label);

    /// Return true if a label has been specified for a prim or its
    /// ancestors for this query's taxonomy and time
    ///
    /// \sa UsdSemanticsLabelsQuery::HasDirectLabel
    USDSEMANTICS_API
    bool HasInheritedLabel(const UsdPrim& prim, const TfToken& label);

    const std::variant<GfInterval, UsdTimeCode>& GetTime() const { 
        return _time; }
    const TfToken& GetTaxonomy() const { return _taxonomy; }

private:
    // Return true if the prim has an entry in the cache
    bool _PopulateLabels(const UsdPrim& prim);

    // Return true if any of the prim's ancestors have an entry in the cache
    bool _PopulateInheritedLabels(const UsdPrim& prim);

    TfToken _taxonomy;
    std::variant<GfInterval, UsdTimeCode> _time;

    std::shared_mutex _cachedLabelsMutex;
    using _UnorderedTokenSet = std::unordered_set<TfToken, TfHash>;
    std::unordered_map<SdfPath, _UnorderedTokenSet, TfHash> _cachedLabels;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif