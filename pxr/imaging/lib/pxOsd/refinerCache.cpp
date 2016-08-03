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
#include "pxr/imaging/pxOsd/refinerCache.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include <opensubdiv/far/stencilTable.h>
#include <opensubdiv/far/stencilTableFactory.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>


#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include "pxr/base/tracelite/trace.h"

#include <boost/shared_ptr.hpp>


using std::vector;
using std::string;


TF_INSTANTIATE_SINGLETON(PxOsdRefinerCache);


bool
PxOsdRefinerCache::CacheEntry::CreateRefiner()
{
    TRACE_FUNCTION();
    
    if (refiner) {
        TF_WARN("Expected NULL refiner, memory leak?");
    }
    
    refiner = PxOsdRefinerFactory::Create(topology);

    if (bilinearStencils) {
        OpenSubdiv::Far::TopologyRefiner::UniformOptions options(level);
        options.fullTopologyInLastLevel = true;
        refiner->RefineUniform(options);        
    } else {
        // XXX:
        // Set the refinement level to 3 for stencil with
        // adaptive for quality. Used to be 10 to avoid artifacts
        // in OpenSubdiv 2.x but now 3 works fine and is faster.
        int stencilRefinementLevel = 3;
        OpenSubdiv::Far::TopologyRefiner::AdaptiveOptions options(
            stencilRefinementLevel);
        options.useSingleCreasePatch = false;
        refiner->RefineAdaptive(options);
    }
    
    
    // Not that we've refined, generate and cache information used
    // later to extract limit stencils from arbitrary parametric
    // positions. Data cached here are CV stencils and patch
    // tables.
    //
    // Options here copied from: LimitStencilTableFactory::Create
    // implementation in OpenSubdiv::Far from:
    //
    // 3.0 RC1 OpenSubdiv tree, stencilTablesFactory.cpp
    OpenSubdiv::Far::StencilTableFactory::Options cvStencilOptions;
    cvStencilOptions.generateOffsets = true;
    cvStencilOptions.generateIntermediateLevels = true;
    cvStencilOptions.generateControlVerts = true;
    
    OpenSubdiv::Far::StencilTable const* cvStencilsRaw =
        OpenSubdiv::Far::StencilTableFactory::Create(
            *refiner, cvStencilOptions);

    OpenSubdiv::Far::PatchTableFactory::Options patchTableOptions;
    patchTableOptions.SetEndCapType(
        OpenSubdiv::Far::PatchTableFactory::Options::ENDCAP_GREGORY_BASIS);
        
    OpenSubdiv::Far::PatchTable *patchTableRaw =
        OpenSubdiv::Far::PatchTableFactory::Create(
            *refiner, patchTableOptions);        
    patchTable = PatchTableSharedPtr(patchTableRaw);

    if (not bilinearStencils) {    
        // Append endcap stencils
        OpenSubdiv::Far::StencilTable const* localPointStencilTable =
            patchTableRaw->GetLocalPointStencilTable();
        if (localPointStencilTable) {
            OpenSubdiv::Far::StencilTable const* stencilTableWithEndcapsRaw =
            OpenSubdiv::Far::StencilTableFactory::AppendLocalPointStencilTable(
                *refiner, cvStencilsRaw, localPointStencilTable);
        
            delete cvStencilsRaw;
            cvStencilsRaw = stencilTableWithEndcapsRaw;
        }
    }
    
    cvStencils = StencilTableSharedPtr(cvStencilsRaw);
    
    return true;
}

PxOsdTopologyRefinerSharedPtr
PxOsdRefinerCache::GetOrCreateRefiner(
    PxOsdMeshTopology topology,
    bool bilinearStencils,
    int level,
    StencilTableSharedPtr *cvStencils,
    PatchTableSharedPtr *patchTable)
{
    TRACE_FUNCTION();    
    std::lock_guard<std::mutex> lock(_mutex);    

    if ((topology.GetScheme() != PxOsdOpenSubdivTokens->catmullClark
                and topology.GetScheme() != PxOsdOpenSubdivTokens->catmark)
          and not bilinearStencils) {
        // XXX: This refiner will be adaptively refined, so we need to ensure
        // we're using catmull-clark subdivision scheme, since that's the only
        // option currently. Once OpenSubdiv supports adaptive loop subdivision,
        // we should remove this hack.
        topology.SetScheme(PxOsdOpenSubdivTokens->catmullClark);
    }

    // This is quick, just compute the hash
    CacheEntry entry = CacheEntry(topology, bilinearStencils, level);

    _CacheEntrySet::iterator iter = _cachedEntries.find(entry);

    if (iter != _cachedEntries.end()) {
        // Cache hit, return the refiner already constructed
        if (cvStencils)
            *cvStencils = iter->cvStencils;
        if (patchTable)
            *patchTable = iter->patchTable;
        return iter->refiner;
    }

    // Cache miss, do the expensive work of creating a new refiner
    entry.CreateRefiner();

    if (cvStencils)    
        *cvStencils = entry.cvStencils;
    if (patchTable)
        *patchTable = entry.patchTable;
    PxOsdTopologyRefinerSharedPtr refiner = entry.refiner;

    _cachedEntries.insert(entry);    

    return refiner;
}
