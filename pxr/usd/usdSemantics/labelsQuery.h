//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

    /// Computes the values for `semantics:labels:<taxonomy>` directly applied 
    /// to this prim. If this query's specified time is a time code, returns the
    /// values at that time, otherwise, computes the union of values
    /// for the interval.
    ///
    /// The results are sorted as if by std::sort
    ///
    /// If no time samples are authored, the default and fallback values will
    /// be queried.
    ///
    /// \sa UsdSemanticsLabelsQuery::ComputeUniqueInheritedLabels
    USDSEMANTICS_API
    VtTokenArray ComputeUniqueDirectLabels(const UsdPrim& prim) const;

    /// Computes the values for `semantics:labels:<taxonomy>` including any 
    /// labels inherited from ancestors. If this query's specified time is a 
    /// time code, returns the values at that time, otherwise, computes the 
    /// union of values for the interval.
    ///
    /// The results are sorted as if by std::sort
    ///
    /// If no time samples are authored, the default and fallback values of
    /// the prim and every ancestor will be queried.
    ///
    /// \sa UsdSemanticsLabelsQuery::ComputeUniqueDirectLabels
    USDSEMANTICS_API
    VtTokenArray ComputeUniqueInheritedLabels(const UsdPrim& prim) const;

    /// Return true if a label has been specified directly on this prim for
    /// this query's taxonomy and time
    ///
    /// \sa UsdSemanticsLabelsQuery::HasInheritedLabel
    USDSEMANTICS_API
    bool HasDirectLabel(const UsdPrim& prim, const TfToken& label) const;

    /// Return true if a label has been specified for a prim or its
    /// ancestors for this query's taxonomy and time
    ///
    /// \sa UsdSemanticsLabelsQuery::HasDirectLabel
    USDSEMANTICS_API
    bool HasInheritedLabel(const UsdPrim& prim, const TfToken& label) const;

    /// Returns the time used by this query when computing a prim's labels.
    const std::variant<GfInterval, UsdTimeCode>& GetTime() const & { 
        return _time; }
    
    /// Returns the time used by this query when computing a prim's labels.
    std::variant<GfInterval, UsdTimeCode> GetTime() && { 
        return std::move(_time); }

    /// Returns the taxonomy used by this query when computing a prim's labels.
    const TfToken& GetTaxonomy() const & { return _taxonomy; }

    /// Returns the taxonomy used by this query when computing a prim's labels.
    TfToken GetTaxonomy() && { return std::move(_taxonomy); }

private:
    // Return true if the prim has an entry in the cache
    bool _PopulateLabels(const UsdPrim& prim) const;

    // Return true if any of the prim's ancestors have an entry in the cache
    bool _PopulateInheritedLabels(const UsdPrim& prim) const;

    TfToken _taxonomy;
    std::variant<GfInterval, UsdTimeCode> _time;

    mutable std::shared_mutex _cachedLabelsMutex;
    using _UnorderedTokenSet = std::unordered_set<TfToken, TfHash>;
    mutable std::unordered_map<SdfPath, _UnorderedTokenSet, TfHash> _cachedLabels;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif