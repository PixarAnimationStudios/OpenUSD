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
#include "pxr/pxr.h"
#include "pxr/usd/usd/stageLoadRules.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stl.h"

#include <boost/functional/hash.hpp>

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdStageLoadRules::AllRule);
    TF_ADD_ENUM_NAME(UsdStageLoadRules::OnlyRule);
    TF_ADD_ENUM_NAME(UsdStageLoadRules::NoneRule);
}

using _GetPath = TfGet<0>;

UsdStageLoadRules
UsdStageLoadRules::LoadNone()
{
    UsdStageLoadRules ret;
    ret._rules.emplace_back(SdfPath::AbsoluteRootPath(), NoneRule);
    return ret;
}

void
UsdStageLoadRules::LoadWithDescendants(SdfPath const &path)
{
    auto range = SdfPathFindPrefixedRange(
        _rules.begin(), _rules.end(), path, _GetPath());

    _rules.insert(_rules.erase(range.first, range.second), { path, AllRule });
}

void
UsdStageLoadRules::LoadWithoutDescendants(SdfPath const &path)
{
    auto range = SdfPathFindPrefixedRange(
        _rules.begin(), _rules.end(), path, _GetPath());

    _rules.insert(_rules.erase(range.first, range.second), { path, OnlyRule });
}

void
UsdStageLoadRules::Unload(SdfPath const &path)
{
    auto range = SdfPathFindPrefixedRange(
        _rules.begin(), _rules.end(), path, _GetPath());

    _rules.emplace(_rules.erase(range.first, range.second), path, NoneRule);
}

void
UsdStageLoadRules::LoadAndUnload(const SdfPathSet &loadSet,
                                 const SdfPathSet &unloadSet,
                                 UsdLoadPolicy policy)
{
    // XXX Could potentially be faster...
    for (SdfPath const &path: unloadSet) {
        Unload(path);
    }
    for (SdfPath const &path: loadSet) {
        if (policy == UsdLoadWithDescendants) {
            LoadWithDescendants(path);
        }
        else if (policy == UsdLoadWithoutDescendants) {
            LoadWithoutDescendants(path);
        }
    }
}

void
UsdStageLoadRules::AddRule(SdfPath const &path, Rule rule)
{
    auto iter = _LowerBound(path);
    if (iter != _rules.end() && iter->first == path) {
        iter->second = rule;
    }
    else {
        _rules.emplace(iter, path, rule);
    }
}

void
UsdStageLoadRules::SetRules(std::vector<std::pair<SdfPath, Rule>> const &rules)
{
    _rules = rules;
}

void
UsdStageLoadRules::Minimize()
{
    if (_rules.empty()) {
        return;
    }
    
    // If there's an 'AllRule' for '/', remove it -- the implicit rule for '/'
    // with no entry present is already 'AllRule'.
    if (_rules.front().second == AllRule &&
        _rules.front().first == SdfPath::AbsoluteRootPath()) {
        _rules.erase(_rules.begin());
    }

    if (_rules.size() <= 1) {
        return;
    }

    // Walk forward, keeping a stack of "parents" (really nearest ancestors).
    // Any entry whose next nearest ancestral entry has the same rule can be
    // removed as redundant.
    std::vector<size_t> parentIdxStack;
    SdfPath curPrefix = _rules.front().first;
    for (size_t i = 0; i != _rules.size(); ++i) {
        auto const &cur = _rules[i];

        // Pop ancestral rules off the stack until we find the one for cur, or
        // until there are no more ancestral rules.
        while (!parentIdxStack.empty() &&
               !cur.first.HasPrefix(_rules[parentIdxStack.back()].first)) {
            parentIdxStack.pop_back();
        }

        // Parent rule is implicitly 'AllRule' if there is no parent.
        Rule parentRule = parentIdxStack.empty() ?
            AllRule : _rules[parentIdxStack.back()].second;
        if (cur.second == parentRule) {
            // Remove this rule.
            _rules.erase(_rules.begin() + i);
            --i;
        }
        else {
            // This rule is kept and becomes the next parent.
            parentIdxStack.push_back(i);
        }
    }
}

bool
UsdStageLoadRules::IsLoaded(SdfPath const &path) const
{
    return GetEffectiveRuleForPath(path) != NoneRule;
}

bool
UsdStageLoadRules::IsLoadedWithAllDescendants(SdfPath const &path) const
{
    if (_rules.empty()) {
        // LoadAll case.
        return true;
    }

    // Find the longest prefix of \p path.  It must be an AllRule.
    auto prefixIter = SdfPathFindLongestPrefix(
        _rules.begin(), _rules.end(), path, _GetPath());

    // There must either be no prefix, or a prefix that's AllRule.
    if (prefixIter != _rules.end() && prefixIter->second != AllRule) {
        return false;
    }

    // Find the range of paths prefixed by the given path.  There must either be
    // none, or all of them must be AllRules.
    auto range = SdfPathFindPrefixedRange(
        _rules.begin(), _rules.end(), path, _GetPath());

    for (auto iter = range.first; iter != range.second; ++iter) {
        if (iter->second != AllRule) {
            return false;
        }
    }

    // This path and all descendants are considered loaded.
    return true;
}

bool
UsdStageLoadRules::IsLoadedWithNoDescendants(SdfPath const &path) const
{
    if (_rules.empty()) {
        // LoadAll case.
        return false;
    }

    // Look for \p path in _rules.  It must be present and must be an OnlyRule.
    auto compareFn = [](std::pair<SdfPath, Rule> const &entry,
                        SdfPath const &p) {
        return entry.first < p;
    };
    auto iter =
        std::lower_bound(_rules.begin(), _rules.end(), path, compareFn);
    
    if (iter == _rules.end() ||
        iter->first != path ||
        iter->second != OnlyRule) {
        return false;
    }

    // Skip the entry for this path, and scan forward to the next non-NoneRule.
    // If it has this path as prefix, return false, otherwise return true.
    for (++iter; iter != _rules.end(); ++iter) {
        if (iter->second != NoneRule) {
            return !iter->first.HasPrefix(path);
        }
    }

    // Encountered only NoneRules: this path is loaded with no descendants.
    return true;
}

UsdStageLoadRules::Rule
UsdStageLoadRules::GetEffectiveRuleForPath(SdfPath const &path) const
{
    if (_rules.empty()) {
        // LoadAll case.
        return AllRule;
    }

    // Find the longest prefix of \p path.  If it is an AllRule, or it is an
    // OnlyRule and its path is the same as this path, then this path is
    // included.
    auto prefixIter = SdfPathFindLongestPrefix(
        _rules.begin(), _rules.end(), path, _GetPath());

    // If no prefix present, this path is included.
    if (prefixIter == _rules.end()) {
        return AllRule;
    }
    
    // If the prefix path's rule is AllRule, this path is included.
    if (prefixIter->second == AllRule) {
        return AllRule;
    }

    // If the prefix *is* this path and it's OnlyRule, we have the answer in
    // hand.
    if (prefixIter->first == path && prefixIter->second == OnlyRule) {
        return OnlyRule;
    }

    // Otherwise find all the "direct child"-type paths of \p path.  That is,
    // subsequent paths that are prefixed _by_ \p path, but skipping deeper such
    // paths.  For example, if path is /Foo/Bar, then we want to consider rules
    // for /Foo/Bar/Baz and /Foo/Bar/Qux, but not /Foo/Bar/Baz/Child.  If one of
    // these exists and it is an AllRule or OnlyRule, then this path is included
    // as 'OnlyRule', since it's part of the ancestor chain.  Otherwise this
    // path is excluded.

    ++prefixIter;
    auto range = SdfPathFindPrefixedRange(
        prefixIter, _rules.end(), path, _GetPath());

    // If there are no such paths, this path is a NoneRule.
    if (range.first == range.second) {
        return NoneRule;
    }

    auto iter = range.first;
    while (iter != range.second) {
        if (iter->second == OnlyRule || iter->second == AllRule) {
            return OnlyRule;
        }
        // Skip anything prefixed by iter->first.
        auto next = iter + 1;
        while (next != range.second && next->first.HasPrefix(iter->first)) {
            ++next;
        }
        iter = next;
    }
    return NoneRule;
}

bool
UsdStageLoadRules::operator==(UsdStageLoadRules const &other) const {
    return _rules == other._rules;
}

std::vector<std::pair<SdfPath, UsdStageLoadRules::Rule> >::const_iterator
UsdStageLoadRules::_LowerBound(SdfPath const &path) const
{
    return std::lower_bound(
        _rules.begin(), _rules.end(), path,
        [](std::pair<SdfPath, Rule> const &elem, SdfPath const &path) {
            return elem.first < path;
        });
}

std::vector<std::pair<SdfPath, UsdStageLoadRules::Rule> >::iterator
UsdStageLoadRules::_LowerBound(SdfPath const &path)
{
    return std::lower_bound(
        _rules.begin(), _rules.end(), path,
        [](std::pair<SdfPath, Rule> const &elem, SdfPath const &path) {
            return elem.first < path;
        });
}

std::ostream &
operator<<(std::ostream &os,
           std::pair<SdfPath, UsdStageLoadRules::Rule> const &p) {
    return os << "(<" << p.first << ">, " <<
        (p.second == UsdStageLoadRules::AllRule ? "AllRule" :
         p.second == UsdStageLoadRules::OnlyRule ? "OnlyRule" :
         p.second == UsdStageLoadRules::NoneRule ? "NoneRule" :
         "<invalid value>") << ")";
}

std::ostream &
operator<<(std::ostream &os, UsdStageLoadRules const &rules)
{
    return os << "UsdStageLoadRules(" << rules._rules << ")";
}

size_t
hash_value(UsdStageLoadRules const &rules)
{
    boost::hash<std::vector<std::pair<SdfPath, UsdStageLoadRules::Rule> > > h;
    return h(rules._rules);
}

PXR_NAMESPACE_CLOSE_SCOPE

