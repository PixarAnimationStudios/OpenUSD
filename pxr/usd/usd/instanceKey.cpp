//
// Copyright 2016 Pixar
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
#include "pxr/usd/usd/instanceKey.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/pcp/primIndex.h"

#include <boost/functional/hash.hpp>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

Usd_InstanceKey::Usd_InstanceKey()
    : _hash(_ComputeHash())
{
}

static UsdStagePopulationMask
_MakeMaskRelativeTo(SdfPath const &path, UsdStagePopulationMask const &mask)
{
    SdfPath const &absRoot = SdfPath::AbsoluteRootPath();
    std::vector<SdfPath> maskPaths = mask.GetPaths();
    for (SdfPath &maskPath: maskPaths) {
        if (maskPath.HasPrefix(path)) {
            maskPath = maskPath.ReplacePrefix(path, absRoot);
        }
        else {
            maskPath = SdfPath();
        }
    }
    // Remove the empty paths to the end, and construct a mask with the
    // translated paths.
    return UsdStagePopulationMask(
        maskPaths.begin(),
        std::remove(maskPaths.begin(), maskPaths.end(), SdfPath()));
}

static UsdStageLoadRules
_MakeLoadRulesRelativeTo(SdfPath const &path, UsdStageLoadRules const &rules)
{
    UsdStageLoadRules::Rule rootRule = rules.GetEffectiveRuleForPath(path);
    std::vector<std::pair<SdfPath, UsdStageLoadRules::Rule> >
        elems = rules.GetRules();
    
    SdfPath const &absRoot = SdfPath::AbsoluteRootPath();
    for (auto &p: elems) {
        if (p.first == path) {
            p.first = absRoot;
            p.second = rootRule;
        }
        else if (p.first.HasPrefix(path)) {
            p.first = p.first.ReplacePrefix(path, absRoot);
        }
        else {
            p.first = SdfPath();
        }
    }
    elems.erase(
        std::remove_if(
            elems.begin(), elems.end(),
            [](std::pair<SdfPath, UsdStageLoadRules::Rule> const &rule) {
                return rule.first.IsEmpty();
            }),
        elems.end());

    // Ensure the first element is the root rule.
    if (elems.empty() || elems.front().first != absRoot) {
        elems.emplace(elems.begin(), absRoot, rootRule);
    }
    else {
        elems.front().second = rootRule;
    }

    UsdStageLoadRules ret;
    ret.SetRules(elems);
    ret.Minimize();
    return ret;
}

Usd_InstanceKey::Usd_InstanceKey(const PcpPrimIndex& instance,
                                 const UsdStagePopulationMask *mask,
                                 const UsdStageLoadRules &loadRules)
    : _pcpInstanceKey(instance)
{
    Usd_ComputeClipSetDefinitionsForPrimIndex(instance, &_clipDefs);

    // Make the population mask "relative" to this prim index by removing the
    // index's path prefix from all paths in the mask that it prefixes.  So for
    // example, if the mask is [/World/set/prop1, /World/set/tableGroup/table,
    // /World/set/prop2], and this prim index is /World/set/tableGroup, then we
    // want the resulting mask to be [/table].  The special cases where the mask
    // includes the whole subtree or excludes the whole subtree are easy to deal
    // with.
    if (!mask) {
        _mask = UsdStagePopulationMask::All();
    }
    else {
        _mask = _MakeMaskRelativeTo(instance.GetPath(), *mask);
    }

    // Do the same with the load rules.
    _loadRules = _MakeLoadRulesRelativeTo(instance.GetPath(), loadRules);

    // Compute and cache the hash code.
    _hash = _ComputeHash();
}

bool 
Usd_InstanceKey::operator==(const Usd_InstanceKey& rhs) const
{
    return _hash == rhs._hash &&
        _pcpInstanceKey == rhs._pcpInstanceKey &&
        _clipDefs == rhs._clipDefs &&
        _mask == rhs._mask &&
        _loadRules == rhs._loadRules;
}

size_t
Usd_InstanceKey::_ComputeHash() const
{
    size_t hash = hash_value(_pcpInstanceKey);
    for (const Usd_ClipSetDefinition& clipDefs: _clipDefs) {
        boost::hash_combine(hash, clipDefs.GetHash());
    }
    boost::hash_combine(hash, _mask);
    boost::hash_combine(hash, _loadRules);
    return hash;
}

std::ostream &
operator<<(std::ostream &os, const Usd_InstanceKey &key)
{
    os << "_pcpInstanceKey:\n" << key._pcpInstanceKey.GetString() << '\n'
       << "_mask: " << key._mask << '\n'
       << "_loadRules: " << key._loadRules << '\n'
       << "_hash: " << key._hash << '\n';
    return os;
}

PXR_NAMESPACE_CLOSE_SCOPE
