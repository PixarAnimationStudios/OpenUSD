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

#ifndef PXR_USD_USD_STAGE_LOAD_RULES_H
#define PXR_USD_USD_STAGE_LOAD_RULES_H

/// \file usd/stageLoadRules.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/sdf/path.h"

#include <iosfwd>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdStageLoadRules
///
/// This class represents rules that govern payload inclusion on UsdStages.
///
/// Rules are represented as pairs of SdfPath and a Rule enum value, one of
/// AllRule, OnlyRule, and NoneRule.  To understand how rules apply to
/// particular paths, see UsdStageLoadRules::GetEffectiveRuleForPath().
///
/// Convenience methods for manipulating rules by typical 'Load' and 'Unload'
/// operations are provided in UsdStageLoadRules::LoadWithoutDescendants(),
/// UsdStageLoadRules::LoadWithDescendants(), UsdStageLoadRules::Unload().
///
/// For finer-grained rule crafting, see AddRule().
///
/// Remove redundant rules that do not change the effective load state with
/// UsdStageLoadRules::Minimize().
class UsdStageLoadRules
{
public:
    /// \enum Rule
    ///
    /// These values are paired with paths to govern payload inclusion on
    /// UsdStages.
    enum Rule {
        /// Include payloads on the specified prim and all descendants.
        AllRule,  
        /// Include payloads on the specified prim but no descendants.
        OnlyRule, 
        /// Exclude payloads on the specified prim and all descendants.
        NoneRule  
    };

    /// Construct rules that load all payloads.
    UsdStageLoadRules() = default;

    /// Return rules that load all payloads.  This is equivalent to
    /// default-constructed UsdStageLoadRules.
    static inline UsdStageLoadRules LoadAll() {
        return UsdStageLoadRules();
    }

    /// Return rules that load no payloads.
    USD_API
    static UsdStageLoadRules LoadNone();

    UsdStageLoadRules(UsdStageLoadRules const &) = default;
    UsdStageLoadRules(UsdStageLoadRules &&) = default;
    UsdStageLoadRules &operator=(UsdStageLoadRules const &) = default;
    UsdStageLoadRules &operator=(UsdStageLoadRules &&) = default;

    /// Add a rule indicating that \p path, all its ancestors, and all its
    /// descendants shall be loaded.
    ///
    /// Any previous rules created by calling LoadWithoutDescendants() or
    /// Unload() on this path or descendant paths are replaced by this rule.
    /// For example, calling LoadWithoutDescendants('/World/sets/kitchen')
    /// followed by LoadWithDescendants('/World/sets') will effectively remove
    /// the rule created in the first call.  See AddRule() for more direct
    /// manipulation.
    USD_API
    void LoadWithDescendants(SdfPath const &path);

    /// Add a rule indicating that \p path and all its ancestors but none of its
    /// descendants shall be loaded.
    ///
    /// Any previous rules created by calling LoadWithDescendants() or Unload()
    /// on this path or descendant paths are replaced or restricted by this
    /// rule.  For example, calling LoadWithDescendants('/World/sets') followed
    /// by LoadWithoutDescendants('/World/sets/kitchen') will cause everything
    /// under '/World/sets' to load except for those things under
    /// '/World/sets/kitchen'.  See AddRule() for more direct manipulation.
    USD_API
    void LoadWithoutDescendants(SdfPath const &path);
    
    /// Add a rule indicating that \p path and all its descendants shall be
    /// unloaded.
    ///
    /// Any previous rules created by calling LoadWithDescendants() or
    /// LoadWithoutDescendants() on this path or descendant paths are replaced
    /// or restricted by this rule.  For example, calling
    /// LoadWithDescendants('/World/sets') followed by
    /// Unload('/World/sets/kitchen') will cause everything under '/World/sets'
    /// to load, except for '/World/sets/kitchen' and everything under it.
    USD_API
    void Unload(SdfPath const &path);

    /// Add rules as if Unload() was called for each element of \p unloadSet
    /// followed by calls to either LoadWithDescendants() (if \p policy is
    /// UsdLoadPolicy::LoadWithDescendants) or LoadWithoutDescendants() (if
    /// \p policy is UsdLoadPolicy::LoadWithoutDescendants) for each element of
    /// \p loadSet.
    USD_API
    void LoadAndUnload(const SdfPathSet &loadSet,
                       const SdfPathSet &unloadSet, UsdLoadPolicy policy);

    /// Add a literal rule.  If there's already a rule for \p path, replace it.
    USD_API
    void AddRule(SdfPath const &path, Rule rule);

    /// Set literal rules, must be sorted by SdfPath::operator<.
    USD_API
    void SetRules(std::vector<std::pair<SdfPath, Rule>> const &rules);

    /// Set literal rules, must be sorted by SdfPath::operator<.
    inline void SetRules(std::vector<std::pair<SdfPath, Rule>> &&rules) {
        _rules = std::move(rules);
    }

    /// Remove any redundant rules to make the set of rules as small as possible
    /// without changing behavior.
    USD_API
    void Minimize();

    /// Return true if the given \p path is considered loaded by these rules, or
    /// false if it is considered unloaded.  This is equivalent to
    /// GetEffectiveRuleForPath(path) != NoneRule.
    USD_API
    bool IsLoaded(SdfPath const &path) const;

    /// Return true if the given \p path and all descendants are considered
    /// loaded by these rules; false otherwise.
    USD_API
    bool IsLoadedWithAllDescendants(SdfPath const &path) const;

    /// Return true if the given \p path and is considered loaded, but none of
    /// its descendants are considered loaded by these rules; false otherwise.
    USD_API
    bool IsLoadedWithNoDescendants(SdfPath const &path) const;

    /// Return the "effective" rule for the given \p path.  For example, if the
    /// closest ancestral rule of \p path is an \p AllRule, return \p AllRule.
    /// If the closest ancestral rule of \p path is for \p path itself and it is
    /// an \p OnlyRule, return \p OnlyRule.  Otherwise if there is a closest
    /// descendant rule to \p path this is an \p OnlyRule or an \p AllRule,
    /// return \p OnlyRule.  Otherwise return \p NoneRule.
    USD_API
    Rule GetEffectiveRuleForPath(SdfPath const &path) const;

    /// Return all the rules as a vector.
    inline std::vector<std::pair<SdfPath, Rule>> const &GetRules() const {
        return _rules;
    }

    /// Return true if \p other has exactly the same set of rules as this.  Note
    /// that this means rules that are functionally equivalent may compare
    /// inequal.  If this is not desired, ensure both sets of rules are reduced
    /// by Minimize() first.
    USD_API
    bool operator==(UsdStageLoadRules const &other) const;

    /// Return false if \p other has exactly the same set of rules as this.  See
    /// operator==().
    inline bool operator!=(UsdStageLoadRules const &other) const {
        return !(*this == other);
    }

    /// Swap the contents of these rules with \p other.
    inline void swap(UsdStageLoadRules &other) {
        _rules.swap(other._rules);
    }

private:
    friend USD_API std::ostream &
    operator<<(std::ostream &, std::pair<SdfPath, Rule> const &);
    
    friend USD_API std::ostream &
    operator<<(std::ostream &, UsdStageLoadRules const &);

    friend USD_API
    size_t hash_value(UsdStageLoadRules const &);

    USD_API
    std::vector<std::pair<SdfPath, Rule> >::const_iterator
    _LowerBound(SdfPath const &path) const;

    USD_API
    std::vector<std::pair<SdfPath, Rule> >::iterator
    _LowerBound(SdfPath const &path);

    std::vector<std::pair<SdfPath, Rule>> _rules;

};

/// Swap the contents of rules \p l and \p r.
inline void swap(UsdStageLoadRules &l, UsdStageLoadRules &r)
{
    l.swap(r);
}

/// Stream a text representation of a UsdStageLoadRules object.
USD_API
std::ostream &operator<<(std::ostream &, UsdStageLoadRules const &);

/// Stream a text representation of a pair of SdfPath and
/// UsdStageLoadRules::Rule.
USD_API
std::ostream &operator<<(std::ostream &,
                         std::pair<SdfPath, UsdStageLoadRules::Rule> const &);

/// Return the hash code for a UsdStageLoadRules object.
USD_API
size_t hash_value(UsdStageLoadRules const &);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_STAGE_LOAD_RULES_H
