//
// Copyright 2019 Pixar
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
#ifndef PXR_USD_USD_COLLECTION_MEMBERSHIP_QUERY_H
#define PXR_USD_USD_COLLECTION_MEMBERSHIP_QUERY_H

/// \file usd/collectionMembershipQuery.h

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/primFlags.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// -------------------------------------------------------------------------- //
// UsdCollectionMembershipQuery                                               //
// -------------------------------------------------------------------------- //
/// \class UsdCollectionMembershipQuery
///
/// \brief Represents a flattened view of a collection.  For more information
/// about collections, please see UsdCollectionAPI as a way to encode and
/// retrieve a collection from scene description.  A
/// UsdCollectionMembershipQuery object can be used to answer queries about
/// membership of paths in the collection efficiently.
class UsdCollectionMembershipQuery
{
public:
    /// Holds an unordered map describing membership of paths in this collection
    /// and the associated expansionRule for how the paths are to be expanded.
    /// Valid expansionRules are UsdTokens->explicitOnly,
    /// UsdTokens->expandPrims, and UsdTokens->expandPrimsAndProperties.  For
    /// more information on the expansion rules, please see the expansionRule
    /// attribute on UsdCollectionAPI.
    /// If a collection includes another collection, the included collection's
    /// PathExpansionRuleMap is merged into this one. If a path is excluded,
    /// its expansion rule is set to UsdTokens->exclude.
    using PathExpansionRuleMap = std::unordered_map<SdfPath,
          TfToken, SdfPath::Hash>;

    /// Default Constructor, creates an empty UsdCollectionMembershipQuery
    /// object
    UsdCollectionMembershipQuery() = default;

    /// Constructor that takes a path expansion rule map.  The map is scanned
    /// for 'excludes' when the UsdCollectionMembershipQuery object is
    /// constructed.
    UsdCollectionMembershipQuery(
        const PathExpansionRuleMap& pathExpansionRuleMap,
        const SdfPathSet& includedCollections);

    /// Constructor that takes a path expansion rule map as an rvalue reference
    UsdCollectionMembershipQuery(
        PathExpansionRuleMap&& pathExpansionRuleMap,
        SdfPathSet&& includedCollections);

    /// \overload
    /// Returns whether the given path is included in the collection from
    /// which this UsdCollectionMembershipQuery object was computed. This is the
    /// API that clients should use for determining if a given object is a
    /// member of the collection. To enumerate all the members of a collection,
    /// use \ref UsdComputeIncludedObjectsFromCollection or
    /// \ref UsdComputeIncludedPathsFromCollection.
    ///
    /// If \p expansionRule is not nullptr, it is set to the expansion-
    /// rule value that caused the path to be included in or excluded from
    /// the collection. If \p path is not included in the collection,
    /// \p expansionRule is set to UsdTokens->exclude.
    ///
    /// It is useful to specify this parameter and use this overload of
    /// IsPathIncluded(), when you're interested in traversing a subtree
    /// and want to know whether the root of the subtree is included in a
    /// collection. For evaluating membership of descendants of the root,
    /// please use the other overload of IsPathIncluded(), that takes both
    /// a path and the parent expansionRule.
    ///
    /// The python version of this method only returns the boolean result.
    /// It does not return \p expansionRule.
    USD_API
    bool IsPathIncluded(const SdfPath &path,
                        TfToken *expansionRule=nullptr) const;

    /// \overload
    /// Returns whether the given path, \p path is included in the
    /// collection from which this UsdCollectionMembershipQuery object was
    /// computed, given the parent-path's inherited expansion rule,
    /// \p parentExpansionRule.
    ///
    /// If \p expansionRule is not nullptr, it is set to the expansion-
    /// rule value that caused the path to be included in or excluded from
    /// the collection. If \p path is not included in the collection,
    /// \p expansionRule is set to UsdTokens->exclude.
    ///
    /// The python version of this method only returns the boolean result.
    /// It does not return \p expansionRule.
    USD_API
    bool IsPathIncluded(const SdfPath &path,
                        const TfToken &parentExpansionRule,
                        TfToken *expansionRule=nullptr) const;

    /// Returns true if the collection excludes one or more paths below an
    /// included path.
    bool HasExcludes() const {
        return _hasExcludes;
    }

    /// Equality operator
    bool operator==(UsdCollectionMembershipQuery const& rhs) const {
        return _hasExcludes == rhs._hasExcludes &&
            _pathExpansionRuleMap == rhs._pathExpansionRuleMap &&
            _includedCollections == rhs._includedCollections;
    }

    /// Inequality operator
    bool operator!=(UsdCollectionMembershipQuery const& rhs) const {
        return !(*this == rhs);
    }

    /// Hash functor
    struct Hash {
        USD_API
        size_t operator()(UsdCollectionMembershipQuery const& query) const;
    };

    /// Hash function
    inline size_t GetHash() const {
        return Hash()(*this);
    }

    /// Returns a raw map of the paths included or excluded in the
    /// collection along with the expansion rules for the included
    /// paths.
    const PathExpansionRuleMap& GetAsPathExpansionRuleMap() const {
        return _pathExpansionRuleMap;
    }

    /// Returns a set of paths for all collections that were included in the
    /// collection from which this UsdCollectionMembershipQuery object was
    /// computed. This set is recursive, so collections that were included
    /// by other collections will be part of this set. The collection from
    /// which this UsdCollectionMembershipQuery object was computed is *not*
    /// part of this set.
    const SdfPathSet& GetIncludedCollections() const {
        return _includedCollections;
    }

private:
    PathExpansionRuleMap _pathExpansionRuleMap;

    SdfPathSet _includedCollections;

    // A cached flag indicating whether _pathExpansionRuleMap contains
    // any exclude rules.
    bool _hasExcludes=false;
};

/// Returns all the usd objects that satisfy the predicate, \p pred in the
/// collection represented by the UsdCollectionMembershipQuery object, \p
/// query.
///
/// The results depends on the load state of the UsdStage, \p stage.
USD_API
std::set<UsdObject> UsdComputeIncludedObjectsFromCollection(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred=UsdPrimDefaultPredicate);

/// Returns all the paths that satisfy the predicate, \p pred in the
/// collection represented by the UsdCollectionMembershipQuery object, \p
/// query.
///
/// The result depends on the load state of the UsdStage, \p stage.
USD_API
SdfPathSet UsdComputeIncludedPathsFromCollection(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred=UsdPrimDefaultPredicate);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
