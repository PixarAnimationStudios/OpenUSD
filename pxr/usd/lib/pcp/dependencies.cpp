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
/// \file Dependencies.cpp


#include "pxr/usd/pcp/dependencies.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/propertyIndex.h"
#include "pxr/usd/sdf/pathTable.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/work/arenaDispatcher.h"
#include <boost/functional/hash.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <algorithm>

template <class T>
struct Pcp_DependencyMultisetGetKey
{
    static const T& GetKey(const T& obj) { return obj; }
};

// A type for holding an unordered multiset of dependencies.  We don't
// need many operations on this type and we expect the number of items in
// it to be small.
// XXX: If this becomes a performance bottleneck this type could
//      internally switch to a different data structure when the
//      number of elements gets too big.
template <typename T, 
          typename Key = T,
          typename KeyFn = Pcp_DependencyMultisetGetKey<T> >
class Pcp_DependencyMultiset {
private:
    typedef std::vector<T> _Data;

public:
    typedef Key key_type;
    typedef T   value_type;
    typedef typename _Data::const_iterator iterator;
    typedef typename _Data::const_iterator const_iterator;

    Pcp_DependencyMultiset() { }
    ~Pcp_DependencyMultiset() { }

    // Insert another entry for \p value.  Order is not maintained.
    void Insert(const value_type& value)
    {
        _data.push_back(value);
    }

    // Insert another entry for \p value.  Order is not maintained.
    template <typename U>
    void Insert(const U& collection)
    {
        _data.insert(_data.end(), collection.begin(), collection.end());
    }

    // Erase one entry for \p key.
    void EraseOne(const key_type& key)
    {
        TRACE_FUNCTION();

        typename _Data::iterator j = _data.end();
        typename _Data::iterator i = _data.begin();
        while (i != j) {
            if (KeyFn::GetKey(*i) == key) {
                // Copy last element over element to remove.
                *i = *--j;

                // Remove last element.
                _data.erase(j);
                break;
            }
            ++i;
        }
    }

    void clear()
    {
        _data.clear();
    }

    void reset()
    {
        TfReset(_data);
    }

    const_iterator begin() const
    {
        return _data.begin();
    }

    const_iterator end() const
    {
        return _data.end();
    }

    bool empty() const
    {
        return _data.empty();
    }

protected:
    _Data &_GetData() 
    {
        return _data;
    }

private:
    _Data _data;
};

// Specifies a path and whether it is ancestral or direct.
struct Pcp_DependencyPathType {
    SdfPath path;
    PcpDependencyType dependencyType;
};

struct Pcp_DependencyPathTypeKeyFn
{
    static const SdfPath& GetKey(const Pcp_DependencyPathType& obj)
    { return obj.path; }
};

typedef Pcp_DependencyMultiset<
    /* T = */     Pcp_DependencyPathType, 
    /* Key = */   SdfPath,
    /* KeyFn = */ Pcp_DependencyPathTypeKeyFn> Pcp_DependencyPathTypeMultiset;

// Defines a Pcp_DependencyMultiset of nodes where the key for each
// node is the root node's path.
struct Pcp_NodeKeyFn
{
    static const SdfPath& GetKey(const PcpNodeRef& obj)
    { return obj.GetRootNode().GetPath(); }
};

typedef Pcp_DependencyMultiset<
    /* T = */     PcpNodeRef,
    /* Key = */   SdfPath,
    /* KeyFn = */ Pcp_NodeKeyFn> Pcp_NodeMultiset;

class Pcp_LayerStackRefCountMap {
private:
    typedef boost::unordered_map<PcpLayerStackRefPtr, size_t> _Data;
    _Data _data;

public:
    typedef _Data::value_type value_type;
    typedef _Data::const_iterator iterator;
    typedef _Data::const_iterator const_iterator;

    Pcp_LayerStackRefCountMap() { }
    ~Pcp_LayerStackRefCountMap() { }

    bool Has(const PcpLayerStackRefPtr& layerStack) const
    {
        return TfMapLookupPtr(_data, layerStack) != NULL;
    }

    void Insert(const PcpLayerStackRefPtr& layerStack) 
    { ++(_data[layerStack]); }
    
    bool EraseOne(const PcpLayerStackRefPtr& layerStack)
    {
        _Data::iterator it = _data.find(layerStack);
        if (TF_VERIFY(it != _data.end())) {
            if (TF_VERIFY(it->second > 0)) {
                --it->second;
            }

            if (it->second == 0) {
                _data.erase(it);
            }
            return true;
        }
        return false;
    }

    void clear()
    {
        _data.clear();
    }

    void reset()
    {
        TfReset(_data);
    }

    const_iterator begin() const
    {
        return _data.begin();
    }

    const_iterator end() const
    {
        return _data.end();
    }

    bool empty() const
    {
        return _data.empty();
    }
};

struct Pcp_DependenciesData {

    ~Pcp_DependenciesData() {
        // Tear down in parallel, since these can get big.
        WorkArenaDispatcher wd;
        wd.Run([this]() { TfReset(sdToPcp); });
        wd.Run([this]() { TfReset(pcpToSd); });
        wd.Run([this]() { TfReset(sitesUsingSite); });
        wd.Run([this]() { TfReset(sitesUsedBySite); });
        wd.Run([this]() { TfReset(spookySdToPcp); });
        wd.Run([this]() { TfReset(spookyPcpToSd); });
        wd.Run([this]() { TfReset(spookySitesUsingSite); });
        wd.Run([this]() { TfReset(spookySitesUsedBySite); });
        wd.Run([this]() { layerStacks.reset(); });
        wd.Run([this]() { sdfSiteNeedsFlush.reset(); });
        wd.Run([this]() { pcpSiteNeedsFlush.reset(); });
        wd.Run([this]() { sdfSpookySiteNeedsFlush.reset(); });
        wd.Run([this]() { pcpSpookySiteNeedsFlush.reset(); });
        wd.Run([this]() { retainedLayerStacks.reset(); });
        wd.Wait();
    }        

    // A map of layers to path tables of multisets of nodes. These multisets
    // are keyed by Pcp site paths, so ultimately what we have here is a map of
    // Sd layer -> Sd path -> Pcp site path -> PcpNode.
    //
    // This is used to maintain information about what Pcp sites depend on a
    // given Sd site, and what node in that Pcp site introduced that dependency.
    typedef SdfPathTable<Pcp_NodeMultiset> PathToNodeMultiset;
    typedef boost::unordered_map<SdfLayerHandle, PathToNodeMultiset> SdToPcp;

    // A path table of multisets of SdfSites.
    typedef Pcp_DependencyMultiset<SdfSite> SdfSiteMultiset;
    typedef SdfPathTable<SdfSiteMultiset> PcpToSd;

    // A multiset of layer stacks.
    typedef Pcp_DependencyMultiset<PcpLayerStackRefPtr> LayerStackMultiset;

    // Table of sites using a site.
    typedef SdfPathTable<Pcp_DependencyPathTypeMultiset> PathToPathTypeMultiset;
    typedef boost::unordered_map<PcpLayerStackPtr,
                                 PathToPathTypeMultiset> SitesUsingSite;

    typedef std::pair<PcpLayerStackPtr, SdfPath> PcpStackSite;
    typedef Pcp_DependencyMultiset<PcpStackSite> PcpSiteMultiset;
    typedef SdfPathTable<PcpSiteMultiset> SitesUsedBySite;

    // Handy types.
    typedef std::pair<PcpToSd::iterator,
                      PcpToSd::iterator> PcpToSdRange;
    typedef std::pair<SitesUsedBySite::iterator,
                      SitesUsedBySite::iterator> SitesUsedBySiteRange;

    // A bidirectional map of SdfSite <-> PcpSite path.
    SdToPcp sdToPcp;
    PcpToSd pcpToSd;

    // A bidirectional map of PcpSite dependencies.
    SitesUsingSite sitesUsingSite;
    SitesUsedBySite sitesUsedBySite;

    // A bidirectional map of SdfSite <-> PcpSite spooky dependencies.
    SdToPcp spookySdToPcp;
    PcpToSd spookyPcpToSd;

    // A bidirectional map of PcpSite spooky dependencies.
    SitesUsingSite spookySitesUsingSite;
    SitesUsedBySite spookySitesUsedBySite;

    // Mapping of all PcpLayerStacks with dependencies registered on them
    // to the number of dependencies.  This also retains the layer stacks
    // to ensure each layer stack's lifetime while a dependency is using it.
    Pcp_LayerStackRefCountMap layerStacks;

    // Sd sites that have had Pcp sites removed.  Some of these sites may
    // be empty, allowing us to remove entries from the maps, but we defer
    // doing that to Flush() to allow the client to time the cleanup.
    SdfSiteMultiset sdfSiteNeedsFlush;
    PcpSiteMultiset pcpSiteNeedsFlush;

    // Same as above except for spooky sites.
    SdfSiteMultiset sdfSpookySiteNeedsFlush;
    PcpSiteMultiset pcpSpookySiteNeedsFlush;

    // Layer stacks on Pcp sites that have been removed.  Some of these
    // layer stacks might be released except for being in this multiset.
    // Holding them here allows the client to time their release with a
    // call to Flush().
    LayerStackMultiset retainedLayerStacks;
};

Pcp_Dependencies::Pcp_Dependencies() :
    _data(new Pcp_DependenciesData)
{
    // Do nothing
}

Pcp_Dependencies::Pcp_Dependencies(const Pcp_Dependencies& other) :
    _data(new Pcp_DependenciesData(*other._data))
{
    // Do nothing
}

Pcp_Dependencies::~Pcp_Dependencies()
{
    // Do nothing
}

static
std::string
_FormatSite(const SdfSite& sdfSite)
{
    return TfStringPrintf("    @%s@<%s>", 
                          sdfSite.layer->GetIdentifier().c_str(),
                          sdfSite.path.GetText());
}

static
std::string
_FormatSite(const Pcp_DependenciesData::PcpStackSite& pcpStackSite)
{
    return TfStringPrintf("    @%s@<%s>", 
                          pcpStackSite.first->GetIdentifier().rootLayer
                              ? pcpStackSite.first->GetIdentifier().rootLayer->
                                  GetIdentifier().c_str()
                              : "<nil>",
                          pcpStackSite.second.GetText());
}

template <class FwdIter>
static
std::string
_FormatSites(FwdIter first, FwdIter last)
{
    typedef typename FwdIter::value_type T;
    std::vector<std::string> specStrings(std::distance(first, last));
    std::transform(first, last, specStrings.begin(),
                   (std::string (*)(const T&))_FormatSite);
    std::sort(specStrings.begin(), specStrings.end());
    return TfStringJoin(specStrings, "\n");
}

inline
void
Pcp_Dependencies::_Add(
    const SdfPath& pcpSitePath,
    SdfSite sdfSite, const PcpNodeRef& node,
    bool spooky)
{
    TfAutoMallocTag2 tag("Pcp", "Pcp_Dependencies:Add SdfSite");

    // Save the dependency.
    Pcp_DependenciesData::SdToPcp& sdToPcp = 
        (spooky ? _data->spookySdToPcp : _data->sdToPcp);
    Pcp_DependenciesData::PcpToSd& pcpToSd = 
        (spooky ? _data->spookyPcpToSd : _data->pcpToSd);

    sdToPcp[sdfSite.layer][sdfSite.path].Insert(node);
    pcpToSd[pcpSitePath].Insert(sdfSite);
}

void
Pcp_Dependencies::Add(
    const SdfPath& pcpSitePath,
    const SdfSiteVector& sdfSites,
    const PcpNodeRefVector& nodes)
{
    static const bool spooky = true;

    TF_VERIFY(sdfSites.size() == nodes.size());
    if (sdfSites.empty()) {
        return;
    }

    TfAutoMallocTag2 tag("Pcp", "Pcp_Dependencies");

    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::Add: Adding spec dependencies for path <%s>:\n%s\n",
        pcpSitePath.GetText(),
        _FormatSites(sdfSites.begin(), sdfSites.end()).c_str());

    for (size_t i = 0; i < sdfSites.size(); ++i) {
        _Add(pcpSitePath, sdfSites[i], nodes[i], not spooky);
    }
}

static 
std::string
_FormatDependencies(const PcpPrimIndexDependencies& deps)
{
    std::vector<std::string> depStrings;
    TF_FOR_ALL(it, deps.sites) {
        const PcpLayerStackSite site(it->first.first, it->first.second);

        depStrings.push_back(
            TfStringPrintf("    %s (%s)", 
                TfStringify(site).c_str(), 
                TfEnum::GetDisplayName(it->second).c_str()));
    }

    std::sort(depStrings.begin(), depStrings.end());
    return TfStringJoin(depStrings, "\n");
}

void
Pcp_Dependencies::Add(
    const SdfPath& pcpSitePath,
    const PcpPrimIndexDependencies& dependencies)
{
    if (dependencies.sites.empty()) {
        return;
    }

    TfAutoMallocTag2 tag("Pcp", "Pcp_Dependencies");
    TfAutoMallocTag2 tag2("Pcp", "Pcp_Dependencies:Add PcpSite");

    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::Add: Adding site dependencies for path <%s>:\n%s\n",
        pcpSitePath.GetText(),
        _FormatDependencies(dependencies).c_str());

    for (const auto& dep : dependencies.sites) {
        const PcpPrimIndexDependencies::Site& site = dep.first;

        // Retain the layer stack.
        _data->layerStacks.Insert(site.first);

        // Create a struct to insert into the sitesUsingSite table.
        Pcp_DependencyPathType curPath;
        curPath.path = pcpSitePath;
        curPath.dependencyType = dep.second;

        _data->sitesUsingSite[site.first][site.second].Insert(curPath);
        _data->sitesUsedBySite[pcpSitePath].
            Insert(Pcp_DependenciesData::PcpStackSite(site.first,site.second));
    }
}

void
Pcp_Dependencies::AddSpookySitesUsedByPrim(
    const SdfPath& pcpSitePath,
    const SdfSiteVector& spookySites,
    const PcpNodeRefVector& spookyNodes)
{
    static const bool spooky = true;

    TF_VERIFY(spookySites.size() == spookyNodes.size());
    if (spookySites.empty()) {
        return;
    }

    TfAutoMallocTag2 tag("Pcp", "Pcp_Dependencies - spooky");

    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::Add: Adding spooky spec dependencies for path "
        "<%s>:\n%s\n",
        pcpSitePath.GetText(),
        _FormatSites(spookySites.begin(), spookySites.end()).c_str());

    for (size_t i = 0; i < spookySites.size(); ++i) {
        _Add(pcpSitePath, spookySites[i], spookyNodes[i], spooky);
    }
}

void
Pcp_Dependencies::AddSpookySitesUsedByPrim(
    const SdfPath& pcpSitePath,
    const PcpPrimIndexDependencies& spookyDependencies)
{
    if (spookyDependencies.sites.empty()) {
        return;
    }

    TfAutoMallocTag2 tag("Pcp", "Pcp_Dependencies - spooky");

    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::Add: Adding spooky site dependencies for path "
        "<%s>:\n%s\n",
        pcpSitePath.GetText(),
        _FormatDependencies(spookyDependencies).c_str());

    for (const auto& dep : spookyDependencies.sites) {
        const PcpPrimIndexDependencies::Site& site = dep.first;

        // Spooky dependencies do not retain the layer stack.
        // This is because spooky dependencies are the result of a
        // relocation applied across a different kind of arc, and
        // the other arc will already depend on the layer stack.
        //
        // XXX It might be nice to verify that there is a dependency
        // registered on that layerStack here, but would be under
        // a different path, so we do not have an easy way to look
        // for an entry without scanning the whole table.

        // Create a struct to insert into the sitesUsingSite table.
        Pcp_DependencyPathType curPath;
        curPath.path = pcpSitePath;
        curPath.dependencyType = dep.second;

        _data->spookySitesUsingSite[site.first][site.second].Insert(curPath);
        _data->spookySitesUsedBySite[pcpSitePath].
            Insert(Pcp_DependenciesData::PcpStackSite(site.first,site.second));
    }
}

void
Pcp_Dependencies::Remove(
    const SdfPath& pcpSitePath,
    PcpLifeboat* lifeboat,
    bool specsAtPathOnly)
{
    // This is for debugging output only.
    std::vector<std::string> debugSites;

    // Remove spec dependencies.
    {
        // Find all Pcp sites to remove.
        Pcp_DependenciesData::PcpToSd::iterator i =
            _data->pcpToSd.find(pcpSitePath);
        Pcp_DependenciesData::PcpToSdRange range(i, i);
        if (specsAtPathOnly and i != _data->pcpToSd.end()) {
            // Only removing spec dependencies at pcpSitePath.
            ++range.second;
        }
        else {
            // Remove all dependencies at and below pcpSitePath.
            range.second = range.first.GetNextSubtree();
        }

        // Discard sdToPcp entries, i.e. the entries in the reverse mapping of
        // the pcpToSd/sdToPcp bidirectional map.
        for (auto valueIter = range.first; valueIter != range.second; ++valueIter) {
            const auto& value = *valueIter;
            const Pcp_DependenciesData::SdfSiteMultiset& entry = value.second;
            if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                debugSites.push_back(
                    TfStringPrintf("  <%s> (spec):", value.first.GetText()));
                debugSites.push_back(
                    _FormatSites(entry.begin(), entry.end()));
            }

            // Remove the reverse mapping.
            for (const auto& sdfSite : entry) {
                _data->sdToPcp[sdfSite.layer][sdfSite.path].EraseOne(value.first);

                // Only mark this Sd site as needing to be flushed if we're
                // removing all dependencies at and below pcpSitePath. This is
                // because Flush() assumes that it can remove all dependencies 
                // at and below each site it processes.
                if (not specsAtPathOnly) {
                    _data->sdfSiteNeedsFlush.Insert(sdfSite);
                }
            }
        }

        // Discard pcpToSd sites.
        if (range.first != range.second) {
            if (specsAtPathOnly) {
                TF_VERIFY(std::distance(range.first, range.second) == 1);
                range.first->second.clear();
            }
            else {
                _data->pcpToSd.erase(range.first);
            }
        }
    }

    if (specsAtPathOnly) {
        TF_DEBUG(PCP_DEPENDENCIES).Msg(
            "Pcp_Dependencies::Remove: Removed dependencies for path <%s>:\n"
            "%s\n",
            pcpSitePath.GetText(),
            TfStringJoin(debugSites, "\n").c_str());
        return;
    }

    // Remove spooky spec dependencies.
    // We do not do this if specsAtPathOnly is true because that is used
    // to rebuild the spec stack after an insignificant change, which
    // does not affect spooky dependencies.  Also, rebuilding the spec stack
    // does not rebuild the spooky spec dependencies, so we need to
    // preserve them here.
    //
    // TODO: Share this code with the above, by refactoring out a
    // separate bimap.
    {
        // Find all spooky Pcp sites to remove.
        Pcp_DependenciesData::PcpToSd::iterator i =
            _data->spookyPcpToSd.find(pcpSitePath);
        Pcp_DependenciesData::PcpToSdRange range(i, i);
        if (specsAtPathOnly and i != _data->spookyPcpToSd.end()) {
            // Only removing spec dependencies at pcpSitePath.
            ++range.second;
        }
        else {
            // Remove all dependencies at and below pcpSitePath.
            range.second = range.first.GetNextSubtree();
        }

        // Discard spookySdToPcp entries, i.e. the entries in the
        // reverse mapping of the spookyPcpToSd/spookySdToPcp
        // bidirectional map.
        for (auto valueIter = range.first; valueIter != range.second; ++valueIter) {
            const auto& value = *valueIter;
            const Pcp_DependenciesData::SdfSiteMultiset& entry = value.second;
            if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                debugSites.push_back(
                    TfStringPrintf("  <%s> (spooky spec):",
                                   value.first.GetText()));
                debugSites.push_back(
                    _FormatSites(entry.begin(), entry.end()));
            }

            // Remove the reverse mapping.
            for (const auto& sdfSite : entry) {
                _data->spookySdToPcp[sdfSite.layer][sdfSite.path].
                    EraseOne(value.first);
                _data->sdfSpookySiteNeedsFlush.Insert(sdfSite);
            }
        }

        // Discard spookyPcpToSd sites.
        if (range.first != range.second) {
            if (specsAtPathOnly) {
                TF_VERIFY(std::distance(range.first, range.second) == 1);
                range.first->second.clear();
            }
            else {
                _data->spookyPcpToSd.erase(range.first);
            }
        }
    }

    // Remove site dependencies.
    {
        // Find all sites used by site to remove.
        Pcp_DependenciesData::SitesUsedBySiteRange siteRange =
            _data->sitesUsedBySite.FindSubtreeRange(pcpSitePath);

        // Discard sites using site entries, i.e. the entries in the reverse
        // mapping of the PcpSite dependencies bidirectional map.
        for (auto valueIter = siteRange.first; valueIter != siteRange.second;
             ++valueIter) 
        {
            const auto& value = *valueIter;
            if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                debugSites.push_back(
                    TfStringPrintf("  <%s> (site):",
                                   value.first.GetText()));
                debugSites.push_back(
                    _FormatSites(value.second.begin(), value.second.end()));
            }

            // Remove the reverse mapping.
            for (const auto& pcpSite : value.second) {
                _data->sitesUsingSite[pcpSite.first][pcpSite.second].
                                                        EraseOne(value.first);
                _data->pcpSiteNeedsFlush.Insert(pcpSite);

                // Hold onto layer stacks and remove from count.
                PcpLayerStackRefPtr layerStack = pcpSite.first;
                if (lifeboat) {
                    lifeboat->Retain(layerStack);
                }
                _data->retainedLayerStacks.Insert(layerStack);
                TF_VERIFY(_data->layerStacks.EraseOne(layerStack));
            }
        }

        // Discard sites used by site entries.
        if (siteRange.first != siteRange.second) {
            _data->sitesUsedBySite.erase(siteRange.first);
        }
    }

    // Remove spooky site dependencies.
    {
        // Find all spooky sites used by site to remove.
        Pcp_DependenciesData::SitesUsedBySiteRange siteRange =
            _data->spookySitesUsedBySite.FindSubtreeRange(pcpSitePath);

        // Discard sites using site entries, i.e. the entries in the reverse
        // mapping of the PcpSite dependencies bidirectional map.
        for (auto valueIter = siteRange.first; valueIter != siteRange.second;
             ++valueIter) {
            const auto& value = *valueIter;

            if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                debugSites.push_back(
                    TfStringPrintf("  <%s> (spooky site):",
                                   value.first.GetText()));
                debugSites.push_back(
                    _FormatSites(value.second.begin(), value.second.end()));
            }

            // Remove the reverse mapping.
            for (const auto& pcpSite : value.second) {
                _data->spookySitesUsingSite[pcpSite.first][pcpSite.second].
                                                        EraseOne(value.first);
                _data->pcpSpookySiteNeedsFlush.Insert(pcpSite);

                // Note: We do not retain layer stacks for spooky dependencies,
                // so we we do not need to drop a _data->layerStacks entry here.
            }
        }

        // Discard sites used by site entries.
        if (siteRange.first != siteRange.second) {
            _data->spookySitesUsedBySite.erase(siteRange.first);
        }
    }

    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::Remove: Removed dependencies for path <%s>:\n"
        "%s\n",
        pcpSitePath.GetText(),
        TfStringJoin(debugSites, "\n").c_str());
}

void
Pcp_Dependencies::RemoveAll(PcpLifeboat* lifeboat)
{
    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::RemoveAll: Clearing all dependencies\n");

    // Collect all of the used layer stacks.
    std::set<PcpLayerStackRefPtr> layerStacks;
    for (const auto& v : _data->layerStacks) {
        layerStacks.insert(v.first);
    }

    // Put layer stacks in the lifeboat.
    if (lifeboat) {
        for (const auto& layerStack : layerStacks) {
            lifeboat->Retain(layerStack);
        }
    }

    // Put layer stacks in _data->retainedLayerStacks as well.  If there's
    // no lifeboat this means the client can still time the release of
    // the layer stacks using Flush().
    _data->retainedLayerStacks.Insert(layerStacks);

    // Clear all dependencies.
    _data->sdToPcp.clear();
    _data->pcpToSd.clear();
    _data->sitesUsingSite.clear();
    _data->sitesUsedBySite.clear();
    _data->spookySdToPcp.clear();
    _data->spookyPcpToSd.clear();
    _data->spookySitesUsingSite.clear();
    _data->spookySitesUsedBySite.clear();
    _data->layerStacks.clear();
    _data->sdfSiteNeedsFlush.clear();
    _data->pcpSiteNeedsFlush.clear();
    _data->sdfSpookySiteNeedsFlush.clear();
    _data->pcpSpookySiteNeedsFlush.clear();

    // Put back dependencies for the layer stacks in retainedLayerStacks.
    // If we don't then Flush() will get upset when calling EraseOne()
    // for each layer stack in retainedLayerStacks since there won't be
    // an expected entry.
    for (const auto& layerStack : _data->retainedLayerStacks) {
        _data->layerStacks.Insert(layerStack);
    }
}

void
Pcp_Dependencies::Flush()
{
    typedef Pcp_DependenciesData::PcpStackSite PcpStackSite;

    // These are for debugging output only.
    std::vector<std::string> debugSites;
    bool anyFound;

    // Copy multisets to sets -- we only want each site once.  Note
    // that we may have sites that are descendants of other sites.
    // That's okay;  see the comment in the loop below.
    std::set<SdfSite> sdfSites(_data->sdfSiteNeedsFlush.begin(),
                               _data->sdfSiteNeedsFlush.end());
    std::set<PcpStackSite> pcpSites(_data->pcpSiteNeedsFlush.begin(),
                                    _data->pcpSiteNeedsFlush.end());
    std::set<SdfSite> sdfSpookySites(_data->sdfSpookySiteNeedsFlush.begin(),
                                     _data->sdfSpookySiteNeedsFlush.end());
    std::set<PcpStackSite> pcpSpookySites(_data->pcpSpookySiteNeedsFlush.begin(),
                                          _data->pcpSpookySiteNeedsFlush.end());

    // Done with the flush sets.
    _data->sdfSiteNeedsFlush.clear();
    _data->pcpSiteNeedsFlush.clear();
    _data->sdfSpookySiteNeedsFlush.clear();
    _data->pcpSpookySiteNeedsFlush.clear();

    // Go through Sd sites discarding unused sites and layers.
    anyFound = false;
    for (const auto& sdfSite : sdfSites) {
        // Find the layer.
        Pcp_DependenciesData::SdToPcp::iterator i =
            _data->sdToPcp.find(sdfSite.layer);

        if (i != _data->sdToPcp.end()) {
            Pcp_DependenciesData::PathToNodeMultiset& pathToNodes = i->second;
            // We can only remove entire subtrees from an SdfPathTable.  While
            // this seems to mean we have to check every path in the subtree
            // to see if any have dependencies, it does not.  It doesn't
            // because for a descendant to exist all of its ancestors must
            // also exist.  If the ancestor doesn't exist then none of its
            // descendants exist either.  If the ancestor exists then we
            // don't care if descendants don't exist (beyond the fact that
            // they can use memory unnecessarily).
            //
            // We iterate through all the sites that need flushing.  Some of
            // these may be descendants of others and that's okay.  We'll
            // always check the most ancestral path first (due to the sorting
            // of sites).  If it's empty we'll remove the entire subtree and
            // we'll simply not find the descendants later.  If the ancestor
            // isn't empty then we'll find the descendant, which itself might
            // be empty and we'll remove it.
            Pcp_DependenciesData::PathToNodeMultiset::iterator j =
                pathToNodes.find(sdfSite.path);
            if (j != pathToNodes.end() and j->second.empty()) {
                if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                    if (not anyFound) {
                        debugSites.push_back("  Sdf to Pcp");
                        anyFound = true;
                    }
                    debugSites.push_back(_FormatSite(sdfSite) + " (spec)");
                }
                pathToNodes.erase(j);

                // If path table is empty we can remove the layer.
                if (pathToNodes.empty()) {
                    if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                        debugSites.push_back(
                            TfStringPrintf("    <%s> (spec layer)",
                                sdfSite.layer->GetIdentifier().c_str()));
                    }
                    _data->sdToPcp.erase(i);
                }
            }
        }
    }

    // Flush spooky spec dependencies.
    anyFound = false;
    for (const auto& sdfSite : sdfSpookySites) {
        // Find the layer.
        const Pcp_DependenciesData::SdToPcp::iterator j =
            _data->spookySdToPcp.find(sdfSite.layer);
        if (j != _data->spookySdToPcp.end()) {
            Pcp_DependenciesData::PathToNodeMultiset& pathToNodes = j->second;
            Pcp_DependenciesData::PathToNodeMultiset::iterator k =
                pathToNodes.find(sdfSite.path);
            if (k != pathToNodes.end() and k->second.empty()) {
                if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                    if (not anyFound) {
                        debugSites.push_back("  Sdf to Pcp (spooky)");
                        anyFound = true;
                    }
                    debugSites.push_back(_FormatSite(sdfSite) + " (spooky spec)");
                }
                pathToNodes.erase(k);

                // If path table is empty we can remove the layer.
                if (pathToNodes.empty()) {
                    if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                        debugSites.push_back(
                            TfStringPrintf("    <%s> (spooky spec layer)",
                                sdfSite.layer->GetIdentifier().c_str()));
                    }
                    _data->spookySdToPcp.erase(j);
                }
            }
        }
    }

    // Go through pcpSites discarding unused sites.  This is the same
    // algorithm as above.
    anyFound = false;
    for (const auto& pcpSite : pcpSites) {
        // Find the layer stack.
        Pcp_DependenciesData::SitesUsingSite::iterator i =
            _data->sitesUsingSite.find(pcpSite.first);
        if (i != _data->sitesUsingSite.end()) {
            // Find the site.
            Pcp_DependenciesData::PathToPathTypeMultiset& pathToPaths =
                i->second;
            Pcp_DependenciesData::PathToPathTypeMultiset::iterator j =
                pathToPaths.find(pcpSite.second);
            if (j != pathToPaths.end() and j->second.empty()) {
                // Remove the entry.  Remove the layer stack if it has no
                // entries anymore.
                if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                    if (not anyFound) {
                        debugSites.push_back("  Sites using site");
                        anyFound = true;
                    }
                    debugSites.push_back(_FormatSite(pcpSite) + " (site)");
                }
                pathToPaths.erase(j);
                if (pathToPaths.empty()) {
                    if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                        debugSites.push_back(
                            TfStringPrintf("    <%s> (site layer stack)",
                                pcpSite.first->GetIdentifier().rootLayer->
                                    GetIdentifier().c_str()));
                    }
                    _data->sitesUsingSite.erase(i);
                }
            }
        }
    }

    // Flush spooky site dependencies.
    anyFound = false;
    for (const auto& pcpSite : pcpSpookySites) {
        const Pcp_DependenciesData::SitesUsingSite::iterator j =
            _data->spookySitesUsingSite.find(pcpSite.first);
        if (j != _data->spookySitesUsingSite.end()) {
            // Find the site.
            Pcp_DependenciesData::PathToPathTypeMultiset& pathToPaths =
                j->second;
            Pcp_DependenciesData::PathToPathTypeMultiset::iterator k =
                pathToPaths.find(pcpSite.second);
            if (k != pathToPaths.end() and k->second.empty()) {
                // Remove the entry.  Remove the layer stack if it has no
                // entries anymore.
                if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                    if (not anyFound) {
                        debugSites.push_back("  Sites using site (spooky)");
                        anyFound = true;
                    }
                    debugSites.push_back(_FormatSite(pcpSite) + " (spooky site)");
                }
                pathToPaths.erase(k);
                if (pathToPaths.empty()) {
                    if (TfDebug::IsEnabled(PCP_DEPENDENCIES)) {
                        debugSites.push_back(
                            TfStringPrintf("    <%s> (spooky site layer stack)",
                                pcpSite.first->GetIdentifier().rootLayer->
                                    GetIdentifier().c_str()));
                    }
                    _data->spookySitesUsingSite.erase(j);
                }
            }
        }
    }

    TF_DEBUG(PCP_DEPENDENCIES).Msg(
        "Pcp_Dependencies::Flush:\n%s\n",
        TfStringJoin(debugSites, "\n").c_str());

    // Release layer stacks.
    _data->retainedLayerStacks.clear();
}


//
// Forward declarations for local helpers.
//
static void
Pcp_GetSdToPcpDependencies(
    const Pcp_DependenciesData::SdToPcp &sdToPcp,
    const SdfSite& sdfSite,
    SdfPathVector* pcpSitePaths,
    PcpNodeRefVector* sourceNodes);
static void
Pcp_GetSdToPcpDependenciesRecursive(
    const Pcp_DependenciesData::SdToPcp &sdToPcp,
    const SdfSite& sdfSite,
    bool primsOnly,
    SdfPathVector* pcpSitePaths,
    PcpNodeRefVector* sourceNodes);

typedef boost::unordered_set<SdfPath> _PathSet;
typedef boost::unordered_map<SdfPath, SdfPath> _PathMap;
static void
Pcp_Get(
    const Pcp_DependenciesData::SitesUsingSite &sitesUsingSite,
    _PathSet* result,
    const PcpLayerStackPtr& layerStack,
    const SdfPath& path,
    unsigned int dependencyType);
static void
Pcp_GetRecursive(
    const Pcp_DependenciesData::SitesUsingSite &sitesUsingSite,
    _PathSet* result,
    const PcpLayerStackPtr& layerStack,
    const SdfPath& path,
    unsigned int dependencyType);


SdfPathVector
Pcp_Dependencies::Get(
    const SdfLayerHandle& layer,
    const SdfPath& path,
    bool recursive,
    bool primsOnly,
    bool spooky,
    PcpNodeRefVector* sourceNodes) const
{
    const Pcp_DependenciesData::SdToPcp &sdToPcp =
        spooky ? _data->spookySdToPcp : _data->sdToPcp;

    // Find every Pcp site path used by the site.
    SdfPathVector result;
    if (sourceNodes) {
        sourceNodes->clear();
    }

    if (recursive) {
        TF_VERIFY(not spooky,
                  "Spooky dependencies cannot be queried recursively.");

        Pcp_GetSdToPcpDependenciesRecursive(
            sdToPcp, SdfSite(layer, path), primsOnly, &result, sourceNodes);
    }
    else {
        TF_VERIFY(primsOnly or not spooky,
                  "Can only ask for spooky dependencies on prims.");

        Pcp_GetSdToPcpDependencies(
            sdToPcp, SdfSite(layer, path), &result, sourceNodes);
    }

    return result;
}

void
Pcp_GetSdToPcpDependencies(
    const Pcp_DependenciesData::SdToPcp &sdToPcp,
    const SdfSite& sdfSite,
    SdfPathVector* pcpSitePaths,
    PcpNodeRefVector* sourceNodes)
{
    Pcp_DependenciesData::SdToPcp::const_iterator i =
        sdToPcp.find(sdfSite.layer);
    if (i != sdToPcp.end()) {
        Pcp_DependenciesData::PathToNodeMultiset::const_iterator j =
            i->second.find(sdfSite.path);
        if (j != i->second.end()) {
            TF_FOR_ALL(k, j->second) {
                pcpSitePaths->push_back(k->GetRootNode().GetPath());
                if (sourceNodes) {
                    sourceNodes->push_back(*k);
                }
            }
        }
    }
}

void
Pcp_GetSdToPcpDependenciesRecursive(
    const Pcp_DependenciesData::SdToPcp &sdToPcp,
    const SdfSite& sdfSite,
    bool primsOnly,
    SdfPathVector* pcpSitePaths,
    PcpNodeRefVector* sourceNodes)
{
    Pcp_DependenciesData::SdToPcp::const_iterator i =
        sdToPcp.find(sdfSite.layer);
    if (i != sdToPcp.end()) {
        typedef Pcp_DependenciesData::PathToNodeMultiset::value_type ValueType;
        auto range = i->second.FindSubtreeRange(sdfSite.path);
        for (auto valueIter = range.first; valueIter != range.second; ++valueIter) {
            const auto& value = *valueIter;
            if (not primsOnly or 
                value.first.IsPrimOrPrimVariantSelectionPath()) {
                TF_FOR_ALL(j, value.second) {
                    pcpSitePaths->push_back(j->GetRootNode().GetPath());
                    if (sourceNodes) {
                        sourceNodes->push_back(*j);
                    }
                }
            }
        }
    }
}

SdfPathVector
Pcp_Dependencies::Get(
    const PcpLayerStackPtr& layerStack,
    const SdfPath& path,
    unsigned int dependencyType,
    bool recursive,
    bool spooky) const
{
    _PathSet result;

    const Pcp_DependenciesData::SitesUsingSite &sitesUsingSite =
        spooky ? _data->spookySitesUsingSite : _data->sitesUsingSite;

    // Find every Pcp site path using the (layerStack, path) site.
    if (recursive) {
        Pcp_GetRecursive(sitesUsingSite, &result, layerStack, path,
                         dependencyType);
    }
    else {
        Pcp_Get(sitesUsingSite, &result, layerStack, path, dependencyType);
    }

    return SdfPathVector(result.begin(), result.end());
}

static void
Pcp_GetWithType(
    _PathSet *result,
    const Pcp_DependencyPathTypeMultiset& paths,
    unsigned int dependencyType)
{
    for (const auto& curPath : paths) {
        // Only include paths that match the specified type.
        if (dependencyType & curPath.dependencyType) {
            result->insert(curPath.path);
        }
    }
}

static void
Pcp_GetAncestral(
    _PathSet *result,
    const Pcp_DependenciesData::PathToPathTypeMultiset& pathsUsingSite,
    const SdfPath& originalPath)
{
    // Walk up the tree collecting ancestral dependencies.  We don't care
    // about the dependencyType because we only get here if ancestral was
    // requested and we want any kind of dependency on ancestors.
    // XXX: This is probably failing to account for relocations in
    //      the path translation.
    SdfPath path = originalPath.GetParentPath();
    while (path.IsPrimOrPrimVariantSelectionPath()) {
        Pcp_DependenciesData::PathToPathTypeMultiset::const_iterator j =
            pathsUsingSite.find(path);
        if (j != pathsUsingSite.end()) {
            for (const auto& curPath : j->second) {
                result->insert(originalPath
                    .ReplacePrefix(path, curPath.path)
                    .StripAllVariantSelections());
            }
        }
        path = path.GetParentPath();
    }
}

void
Pcp_Get(
    const Pcp_DependenciesData::SitesUsingSite &sitesUsingSite,
    _PathSet* result,
    const PcpLayerStackPtr& layerStack,
    const SdfPath& path,
    unsigned int dependencyType)
{
    Pcp_DependenciesData::SitesUsingSite::const_iterator i =
        sitesUsingSite.find(layerStack);
    if (i != sitesUsingSite.end()) {
        Pcp_DependenciesData::PathToPathTypeMultiset::const_iterator j =
            i->second.find(path);
        if (j != i->second.end()) {
            Pcp_GetWithType(result, j->second, dependencyType);
        }

        // If we're requesting ancestral dependencies then check ancestors
        // for any dependencies we don't have.
        // XXX: There seems to be some inconsistency whether we store
        //      ancestral dependencies or now.  We should either have
        //      them all or have none of them.
        if (dependencyType & PcpAncestral) {
            Pcp_GetAncestral(result, i->second, path);
        }
    }
}

void
Pcp_GetRecursive(
    const Pcp_DependenciesData::SitesUsingSite &sitesUsingSite,
    _PathSet* result,
    const PcpLayerStackPtr& layerStack,
    const SdfPath& path,
    unsigned int dependencyType)
{
    Pcp_DependenciesData::SitesUsingSite::const_iterator i =
        sitesUsingSite.find(layerStack);
    if (i != sitesUsingSite.end()) {
        Pcp_DependenciesData::PathToPathTypeMultiset::const_iterator j =
            i->second.find(path);
        if (j != i->second.end()) {
            // Get dependencies for the whole subtree.
            Pcp_DependenciesData::PathToPathTypeMultiset::const_iterator k =
                j.GetNextSubtree();
            for (; j != k; ++j) {
                Pcp_GetWithType(result, j->second, dependencyType);
            }
        }

        // If we're requesting ancestral dependencies then check ancestors
        // for any dependencies we don't have.
        // XXX: There seems to be some inconsistency whether we store
        //      ancestral dependencies or now.  We should either have
        //      them all or have none of them.
        if (dependencyType & PcpAncestral) {
            Pcp_GetAncestral(result, i->second, path);
        }
    }
}

SdfLayerHandleSet
Pcp_Dependencies::GetLayersUsedByPrim(const SdfPath& path,
                                      bool recursive) const
{
    typedef Pcp_DependenciesData::SitesUsedBySite::value_type ValueType;

    // Get layer stacks.
    std::set<PcpLayerStackPtr> layerStacks;
    if (recursive) {
        auto range = _data->sitesUsedBySite.FindSubtreeRange(path);
        for (auto valueIter = range.first; valueIter != range.second; ++valueIter) {
            const auto& value = *valueIter;
            for (const auto& site : value.second) {
                layerStacks.insert(site.first);
            }
        }
    }
    else {
        Pcp_DependenciesData::SitesUsedBySite::const_iterator i =
            _data->sitesUsedBySite.find(path);
        if (i != _data->sitesUsedBySite.end()) {
            for (const auto& site : i->second) {
                layerStacks.insert(site.first);
            }
        }
    }

    // Get layers.
    SdfLayerHandleSet layers;
    for (const auto& layerStack : layerStacks) {
        const SdfLayerRefPtrVector& localLayers = layerStack->GetLayers();
        layers.insert(localLayers.begin(), localLayers.end());
    }

    return layers;
}

SdfLayerHandleSet 
Pcp_Dependencies::GetUsedLayers() const
{
    SdfLayerHandleSet reachedLayers;

    for (const auto& v : _data->layerStacks) {
        const SdfLayerRefPtrVector& layers = v.first->GetLayers();
        reachedLayers.insert(layers.begin(), layers.end());
    }

    return reachedLayers;
}

SdfLayerHandleSet 
Pcp_Dependencies::GetUsedRootLayers() const
{
    SdfLayerHandleSet reachedRootLayers;

    for (const auto& v : _data->layerStacks) {
        const PcpLayerStackPtr& layerStack = v.first;
        reachedRootLayers.insert(layerStack->GetIdentifier().rootLayer);
    }

    return reachedRootLayers;
}

bool 
Pcp_Dependencies::UsesLayerStack(const PcpLayerStackPtr& layerStack) const
{
    return _data->layerStacks.Has(layerStack);
}

//
// Debugging
//

template <class T>
struct Pcp_DependenciesTupleTraits {
};

template <>
struct Pcp_DependenciesTupleTraits<SdfLayerHandle> {
    static const char* GetIdentifier(const SdfLayerHandle& a)
    {
        return a->GetIdentifier().c_str();
    }

    static const char* GetForwardLabel()
    {
        return "Sdf to Pcp";
    }

    static const char* GetReverseLabel()
    {
        return "Pcp to Sdf";
    }
};

template <>
struct Pcp_DependenciesTupleTraits<PcpLayerStackPtr> {
    static const char* GetIdentifier(const PcpLayerStackPtr& a)
    {
        return a->GetIdentifier().rootLayer->GetIdentifier().c_str();
    }

    static const char* GetForwardLabel()
    {
        return "Sites using site";
    }

    static const char* GetReverseLabel()
    {
        return "Sites used by site";
    }
};

template <class T>
struct Pcp_DependenciesTuple {
    T a; SdfPath b; SdfPath c;

    Pcp_DependenciesTuple() { }
    Pcp_DependenciesTuple(T a_, SdfPath b_, SdfPath c_) :
        a(a_), b(b_), c(c_) { }

    bool operator<(const Pcp_DependenciesTuple<T>& x) const
    {
        if (a < x.a) return true;
        if (x.a < a) return false;
        if (b < x.b) return true;
        if (x.b < b) return false;
        if (c < x.c) return true;
        if (x.c < c) return false;
        return false;
    }

    struct ReverseLess {
        bool operator()(const Pcp_DependenciesTuple<T>& x,
                        const Pcp_DependenciesTuple<T>& y) const
        {
            if (x.c < y.c) return true;
            if (y.c < x.c) return false;
            if (x.b < y.b) return true;
            if (y.b < x.b) return false;
            if (x.a < y.a) return true;
            if (y.a < x.a) return false;
            return false;
        }
    };

    const char* GetIdentifier() const
    {
        return Pcp_DependenciesTupleTraits<T>::GetIdentifier(a);
    }

    static const char* GetForwardLabel()
    {
        return Pcp_DependenciesTupleTraits<T>::GetForwardLabel();
    }

    static const char* GetReverseLabel()
    {
        return Pcp_DependenciesTupleTraits<T>::GetReverseLabel();
    }
};
typedef Pcp_DependenciesTuple<SdfLayerHandle> Pcp_DependenciesSpecTuple;
typedef Pcp_DependenciesTuple<PcpLayerStackPtr> Pcp_DependenciesSiteTuple;

template <class T>
static void
_DumpDependencies(
    std::ostream& s,
    const char* prefix,
    const std::multiset<T>& fwd,
    const std::multiset<T>& tmpRev)
{
    T prev;

    // Print forward dependencies.
    if (not fwd.empty()) {
        s << prefix << T::GetForwardLabel() << ":" << std::endl;
        for (const auto& x : fwd) {
            if (prev.a != x.a) {
                prev.a = x.a;
                s << "  @" << x.GetIdentifier() << "@:" << std::endl;
            }
            if (prev.b != x.b) {
                prev.b = x.b;
                s << "    <" << x.b.GetText() << ">:" << std::endl;
            }
            s << "      <" << x.c.GetText() << ">" << std::endl;
        }
    }

    // Get reverse in reverse sorted order
    std::multiset<T, typename T::ReverseLess> rev(tmpRev.begin(), tmpRev.end());

    // Print reverse dependencies.
    if (not rev.empty()) {
        prev = T();
        s << prefix << T::GetReverseLabel() << ":" << std::endl;
        for (const auto& x : rev) {
            if (prev.c != x.c) {
                prev.c = x.c;
                s << "  <" << x.c.GetText() << ">:" << std::endl;
            }
            if (prev.b != x.b) {
                prev.b = x.b;
                s << "    <" << x.b.GetText() << ">" << std::endl;
            }
            s << "      @" << x.GetIdentifier() << "@" << std::endl;
        }
    }
}

template <class T>
static void
_DiscardMatches(std::multiset<T>* fwd, std::multiset<T>* rev)
{
    typedef typename std::multiset<T>::iterator iterator;

    for (iterator i = rev->begin(); i != rev->end(); ) {
        iterator j = fwd->find(*i);
        if (j != fwd->end()) {
            fwd->erase(j);
            rev->erase(i++);
        }
        else {
            ++i;
        }
    }
}

static
void
_GetDependencies(
    const Pcp_DependenciesData::SdToPcp& fwdInput,
    const Pcp_DependenciesData::PcpToSd& revInput,
    std::multiset<Pcp_DependenciesSpecTuple>* fwd,
    std::multiset<Pcp_DependenciesSpecTuple>* rev)
{
    typedef Pcp_DependenciesData DD;

    fwd->clear();
    rev->clear();

    for (const auto& x : fwdInput) {
        for (const auto& y : x.second) {
            for (const auto& z : y.second) {
                fwd->insert(Pcp_DependenciesSpecTuple(
                        x.first, y.first, z.GetRootNode().GetPath()));
            }
        }
    }

    for (const auto& x : revInput) {
        for (const auto& y : x.second) {
            rev->insert(Pcp_DependenciesSpecTuple(y.layer, y.path, x.first));
        }
    }
}

static
void
_GetDependencies(
    const Pcp_DependenciesData::SitesUsingSite& fwdInput,
    const Pcp_DependenciesData::SitesUsedBySite& revInput,
    std::multiset<Pcp_DependenciesSiteTuple>* fwd,
    std::multiset<Pcp_DependenciesSiteTuple>* rev)
{
    typedef Pcp_DependenciesData DD;

    fwd->clear();
    rev->clear();

    for (const auto& x : fwdInput) {
        for (const auto& y : x.second) {
            for (const auto& z : y.second) {
                fwd->insert(Pcp_DependenciesSiteTuple(x.first, y.first,z.path));
            }
        }
    }

    for (const auto& x : revInput) {
        for (const auto& y : x.second) {
            rev->insert(Pcp_DependenciesSiteTuple(y.first, y.second, x.first));
        }
    }
}

void
Pcp_Dependencies::DumpDependencies(std::ostream& s) const
{
    {
        std::multiset<Pcp_DependenciesSpecTuple> fwd, rev;

        _GetDependencies(_data->sdToPcp, _data->pcpToSd, &fwd, &rev);
        _DumpDependencies(s, "", fwd, rev);

        _GetDependencies(_data->spookySdToPcp, _data->spookyPcpToSd,
                         &fwd, &rev);
        _DumpDependencies(s, "spooky", fwd, rev);
    }

    {
        std::multiset<Pcp_DependenciesSiteTuple> fwd, rev;
        _GetDependencies(_data->sitesUsingSite, _data->sitesUsedBySite,
                         &fwd, &rev);
        _DumpDependencies(s, "", fwd, rev);

        _GetDependencies(_data->spookySitesUsingSite,
                         _data->spookySitesUsedBySite,
                         &fwd, &rev);
        _DumpDependencies(s, "spooky", fwd, rev);
    }
}

std::string 
Pcp_Dependencies::DumpDependencies() const
{
    std::stringstream ss;
    DumpDependencies(ss);
    return ss.str();
}

void
Pcp_Dependencies::CheckInvariants() const
{
    bool fail = false;

    {
        std::multiset<Pcp_DependenciesSpecTuple> fwd, rev;
        _GetDependencies(_data->sdToPcp, _data->pcpToSd, &fwd, &rev);
        _DiscardMatches(&fwd, &rev);
        _DumpDependencies(std::cerr, "ERROR: no match in ", fwd, rev);
        fail = fail or not fwd.empty() or not rev.empty();

        _GetDependencies(_data->spookySdToPcp, _data->spookyPcpToSd, &fwd,&rev);
        _DiscardMatches(&fwd, &rev);
        _DumpDependencies(std::cerr, "ERROR: no match in spooky ", fwd, rev);
        fail = fail or not fwd.empty() or not rev.empty();
    }
    {
        std::multiset<Pcp_DependenciesSiteTuple> fwd, rev;
        _GetDependencies(_data->sitesUsingSite, _data->sitesUsedBySite,
                         &fwd, &rev);
        _DiscardMatches(&fwd, &rev);
        _DumpDependencies(std::cerr, "ERROR: no match in ", fwd, rev);
        fail = fail or not fwd.empty() or not rev.empty();

        _GetDependencies(_data->spookySitesUsingSite,
                         _data->spookySitesUsedBySite,
                         &fwd, &rev);
        _DiscardMatches(&fwd, &rev);
        _DumpDependencies(std::cerr, "ERROR: no match in spooky ", fwd, rev);
        fail = fail or not fwd.empty() or not rev.empty();
    }

    TF_VERIFY(not fail);
}
