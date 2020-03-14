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
#include "pxr/usd/pcp/statistics.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/layerStackRegistry.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/primIndex_Graph.h"

#include "pxr/base/arch/defines.h"

#include <memory>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

struct Pcp_GraphStats
{
public:
    Pcp_GraphStats()
        : numNodes(0)
        , numImpliedInherits(0)
    { }

    size_t numNodes;
    std::map<PcpArcType, size_t> typeToNumNodes;
    size_t numImpliedInherits;
};

struct Pcp_CacheStats
{
public:
    Pcp_CacheStats()
        : numPrimIndexes(0)
        , numPropertyIndexes(0)
        , numGraphInstances(0)
    { }

    size_t numPrimIndexes;
    size_t numPropertyIndexes;

    Pcp_GraphStats allGraphStats;
    Pcp_GraphStats culledGraphStats;

    size_t numGraphInstances;
    Pcp_GraphStats sharedAllGraphStats;
    Pcp_GraphStats sharedCulledGraphStats;
    std::map<size_t, size_t> mapFunctionSizeDistribution;
    std::map<size_t, size_t> layerStackRelocationsSizeDistribution;
};

class Pcp_Statistics
{
public:
    static void AccumulateGraphStats(
        const PcpPrimIndex& primIndex, 
        Pcp_GraphStats* stats,
        bool culledNodesOnly)
    {
        for (const PcpNodeRef &node: primIndex.GetNodeRange()) {
            if (culledNodesOnly && !node.IsCulled()) {
                continue;
            }

            ++(stats->numNodes);
            ++(stats->typeToNumNodes[node.GetArcType()]);
            
            const bool nodeIsImpliedInherit = 
                node.GetOriginNode() != node.GetParentNode();
            if (nodeIsImpliedInherit) {
                if (node.GetArcType() == PcpArcTypeInherit)
                    ++(stats->numImpliedInherits);
            }
        }
    }

    struct MapFuncHash {
        size_t operator()(const PcpMapFunction &m) const {
            return m.Hash();
        }
    };

    static void AccumulateCacheStats(
        const PcpCache* cache, Pcp_CacheStats* stats)
    {
        typedef std::shared_ptr<PcpPrimIndex_Graph::_SharedData> 
            _SharedNodePool;
        std::set<_SharedNodePool> seenNodePools;
        TfHashSet<PcpMapFunction, MapFuncHash> allMapFuncs;

        TF_FOR_ALL(it, cache->_primIndexCache) {
            const PcpPrimIndex& primIndex = it->second;
            if (!primIndex.IsValid()) {
                continue;
            }

            ++(stats->numPrimIndexes);

            AccumulateGraphStats(
                primIndex, &stats->allGraphStats, 
                /* culledNodesOnly = */ false);
            AccumulateGraphStats(
                primIndex, &stats->culledGraphStats, 
                /* culledNodesOnly = */ true);

            if (seenNodePools.insert(primIndex.GetGraph()->_data).second) {
                ++(stats->numGraphInstances);

                AccumulateGraphStats(
                    primIndex, &stats->sharedAllGraphStats, 
                    /* culledNodesOnly = */ false);
                AccumulateGraphStats(
                    primIndex, &stats->sharedCulledGraphStats, 
                    /* culledNodesOnly = */ true);
            }

            // Gather map functions
            for (const PcpNodeRef &node: primIndex.GetNodeRange()) {
                allMapFuncs.insert(node.GetMapToParent().Evaluate());
                allMapFuncs.insert(node.GetMapToRoot().Evaluate());
            }
        }

        TF_FOR_ALL(it, cache->_propertyIndexCache) {
            const PcpPropertyIndex& propIndex = it->second;
            if (propIndex.IsEmpty()) {
                continue;
            }

            ++(stats->numPropertyIndexes);
        }

        // PcpMapFunction size distribution
        TF_FOR_ALL(i, allMapFuncs) {
            size_t size = i->GetSourceToTargetMap().size();
            stats->mapFunctionSizeDistribution[size] += 1;
        }

        // PcpLayerStack _relocatesPrimPaths size distribution
        for(const PcpLayerStackPtr &layerStack:
            cache->_layerStackCache->GetAllLayerStacks()) {
            size_t size = layerStack->GetPathsToPrimsWithRelocates().size();
            stats->layerStackRelocationsSizeDistribution[size] += 1;
        }
    }

    // Shamelessly stolen from Csd/Scene_PrimCachePopulation.cpp.
    struct _Helper {
        static std::string FormatNumber(size_t n)
        {
            return TfStringPrintf("%'zd", n);
        }

        static std::string FormatAverage(size_t n, size_t d)
        {
            if (d == 0) {
                return "N/A";
            }
            return TfStringPrintf("%'.3f", (double)n / (double)d);
        }

        static std::string FormatSize(size_t n)
        {
            if (n < 1024) {
                return TfStringPrintf("%zd B", n);
            }
            if (n < 10 * 1024) {
                return TfStringPrintf("%4.2f kB", (double)n / 1024.0);
            }
            if (n < 100 * 1024) {
                return TfStringPrintf("%4.1f kB", (double)n / 1024.0);
            }
            if (n < 1024 * 1024) {
                return TfStringPrintf("%3zd kB", n / 1024);
            }
            if (n < 10 * 1024 * 1024) {
                return TfStringPrintf("%4.2f MB", 
                                      (double)n /(1024.0 * 1024.0));
            }
            if (n < 100 * 1024 * 1024) {
                return TfStringPrintf("%4.1f MB", 
                                      (double)n /(1024.0 * 1024.0));
            }
            if (n < 1024 * 1024 * 1024) {
                return TfStringPrintf("%3zd MB", n / (1024 * 1024));
            }
            return TfStringPrintf("%f GB", n / (1024.0 * 1024.0 * 1024.0));
        }
    };

    static void PrintGraphStats(
        const Pcp_GraphStats& totalStats,
        const Pcp_GraphStats& culledStats,
        std::ostream& out)
    {
        using namespace std;

        out << "  Total nodes:                       " 
            << _Helper::FormatNumber(totalStats.numNodes) << endl;
        out << "  Total culled* nodes:               " 
            << _Helper::FormatNumber(culledStats.numNodes) << endl;
        out << "  By type (total / culled*):         " << endl;

        std::map<PcpArcType, size_t> typeToNumNodes = 
            totalStats.typeToNumNodes;
        std::map<PcpArcType, size_t> typeToNumCulledNodes = 
            culledStats.typeToNumNodes;
        for (PcpArcType t = PcpArcTypeRoot; t != PcpNumArcTypes; 
             t = (PcpArcType)(t + 1)) {
            const std::string nodeTypeName = TfEnum::GetDisplayName(t);
            out << "    " << nodeTypeName << ": "
                << TfStringPrintf("%*s%s / %s", 
                    (int)(31 - nodeTypeName.size()), "",
                    _Helper::FormatNumber(typeToNumNodes[t]).c_str(),
                    _Helper::FormatNumber(typeToNumCulledNodes[t]).c_str())
                << endl;
            
            if (t == PcpArcTypeInherit) {
                out << "      implied inherits: "
                    << TfStringPrintf("%*s%s / %s",
                        13, "",
                        _Helper::FormatNumber(totalStats.numImpliedInherits).c_str(),
                        _Helper::FormatNumber(culledStats.numImpliedInherits).c_str())
                    << endl;
            }
        }

        out << "  (*) This does not include culled nodes that were erased "
            << "from the graph" << endl;
    }

    static void PrintCacheStats(
        const PcpCache* cache, std::ostream& out)
    {
        using namespace std;

        Pcp_CacheStats stats;
        AccumulateCacheStats(cache, &stats);

        out << "PcpCache Statistics" << endl
            << "-------------------" << endl;
        
        out << "Entries: " << endl;
        out << "  Prim indexes:                      " 
            << _Helper::FormatNumber(stats.numPrimIndexes) << endl;
        out << "  Property indexes:                  " 
            << _Helper::FormatNumber(stats.numPropertyIndexes) << endl;
        out << endl;

        out << "Prim graphs: " << endl;
        PrintGraphStats(
            stats.allGraphStats, stats.culledGraphStats, out);
        out << endl;

        out << "Prim graphs (shared): " << endl;
        out << "  Graph instances:                   "
            << _Helper::FormatNumber(stats.numGraphInstances) << endl;
        PrintGraphStats(
            stats.sharedAllGraphStats, stats.sharedCulledGraphStats, out);
        out << endl;

        out << "Memory usage: " << endl;
        out << "  sizeof(PcpMapFunction):            " 
            << _Helper::FormatSize(sizeof(PcpMapFunction)) << endl;
        out << "  sizeof(PcpLayerStackPtr):          " 
            << _Helper::FormatSize(sizeof(PcpLayerStackPtr)) << endl;
        out << "  sizeof(PcpLayerStackSite):         " 
            << _Helper::FormatSize(sizeof(PcpLayerStackSite)) << endl;
        out << "  sizeof(PcpPrimIndex):              " 
            << _Helper::FormatSize(sizeof(PcpPrimIndex)) << endl;
        out << "  sizeof(PcpPrimIndex_Graph):        " 
            << _Helper::FormatSize(sizeof(PcpPrimIndex_Graph)) << endl;
        out << "  sizeof(PcpPrimIndex_Graph::_Node): " 
            << _Helper::FormatSize(sizeof(PcpPrimIndex_Graph::_Node)) << endl;
        out << endl;

        out << "PcpMapFunction size histogram: " << endl;
        out << "SIZE    COUNT" << endl;
        TF_FOR_ALL(i, stats.mapFunctionSizeDistribution) {
            printf("%zu   %zu\n", i->first, i->second);
        }

        out << "PcpLayerStack pathsWithRelocates size histogram: " << endl;
        out << "SIZE    COUNT" << endl;
        TF_FOR_ALL(i, stats.layerStackRelocationsSizeDistribution) {
            printf("%zu   %zu\n", i->first, i->second);
        }
    }

    static void PrintPrimIndexStats(
        const PcpPrimIndex& primIndex, std::ostream& out)
    {
        using namespace std;

        Pcp_GraphStats totalStats, culledStats;
        AccumulateGraphStats(
            primIndex, &totalStats, /* culledNodesOnly = */ false);
        AccumulateGraphStats(
            primIndex, &culledStats, /* culledNodesOnly = */ true);

        out << "PcpPrimIndex Statistics - " 
            << primIndex.GetRootNode().GetPath() << endl
            << "-----------------------" << endl;

        PrintGraphStats(totalStats, culledStats, out);
        out << endl;
    }
};

void
Pcp_PrintCacheStatistics(
    const PcpCache* cache, std::ostream& out)
{
    Pcp_Statistics::PrintCacheStats(cache, out);
}

void
Pcp_PrintPrimIndexStatistics(
    const PcpPrimIndex& primIndex, std::ostream& out)
{
    Pcp_Statistics::PrintPrimIndexStats(primIndex, out);
}

PXR_NAMESPACE_CLOSE_SCOPE
