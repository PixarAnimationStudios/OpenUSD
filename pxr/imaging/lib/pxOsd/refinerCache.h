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
///
/// \file pxOsd/refinerCache.h
///

#ifndef PXOSD_REFINER_CACHE_H
#define PXOSD_REFINER_CACHE_H

#include "meshTopology.h"
#include "refinerFactory.h"

#include <opensubdiv/far/stencilTable.h>
#include <opensubdiv/far/stencilTableFactory.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>


#include "pxr/base/tf/diagnostic.h"

#include "pxr/base/tracelite/trace.h"

#include <boost/shared_ptr.hpp>
#include "pxr/base/tf/hashset.h"

#include <mutex>


// A class that acts as a singleton cache of expensive OpenSubdiv stencil
// tables, patch tables, and topology refiners. This data is used to
// project onto subdivs by OsdProjector.
//
class PxOsdRefinerCache {
public:
    typedef boost::shared_ptr<class OpenSubdiv::Far::StencilTable const> StencilTableSharedPtr;
    typedef boost::shared_ptr<class OpenSubdiv::Far::PatchTable const> PatchTableSharedPtr;

    
    PxOsdRefinerCache static& GetInstance() {
        return TfSingleton< PxOsdRefinerCache >::GetInstance();
    }

    PxOsdTopologyRefinerSharedPtr GetOrCreateRefiner(
        PxOsdMeshTopology topology,
        bool bilinearStencils,
        int level,
        StencilTableSharedPtr *cvStencils = NULL,
        PatchTableSharedPtr *patchTable = NULL);        

private:
    struct CacheEntry {
        PxOsdMeshTopology topology;
        bool bilinearStencils;
        int level;

        // Stored hash, based on topology, level, and bilinear
        PxOsdMeshTopology::ID hash;

        // Caches of stuff derived from topology
        PxOsdTopologyRefinerSharedPtr refiner;
        StencilTableSharedPtr cvStencils;
        PatchTableSharedPtr patchTable;

        CacheEntry(
            PxOsdMeshTopology _topology, bool _bilinearStencils,
            int _level):
            topology(_topology),
            bilinearStencils(_bilinearStencils),
            level(_level)
            {
                hash = ComputeHash(topology, bilinearStencils,level);
            }

        ~CacheEntry( ) {
        }

        bool CreateRefiner();
        
        static PxOsdMeshTopology::ID ComputeHash(
            PxOsdMeshTopology const& topology, bool bilinearStencils, int level)
            {
                // Take the hash key computed from topology and salt
                // it with bilinear and level to produce "unique" key
                PxOsdMeshTopology::ID hash = topology.ComputeHash();
                int valsToHash[2] = {level, (int)bilinearStencils};
                return ArchHash((char const*) valsToHash,
                                sizeof(int) * 2, hash);
            }

        bool operator==(CacheEntry const& x) const
            {
                return ((x.topology == topology) and
                        (x.bilinearStencils == bilinearStencils) and
                        (x.level == level));
            }
        CacheEntry& operator=(CacheEntry const& x) 
            {
                topology = x.topology;
                bilinearStencils = x.bilinearStencils;
                level = x.level;
                refiner = x.refiner;
                cvStencils = x.cvStencils;
                patchTable = x.patchTable;
                return *this;
            }                
        
    };

    struct HashFunctor {
    public:
        size_t operator() (CacheEntry const& entry) const {
            return entry.hash;
        }
    };
    
    PxOsdRefinerCache() {
        TfSingleton< PxOsdRefinerCache >::SetInstanceConstructed(
            *this);
    }

    ~PxOsdRefinerCache() {
    }

    std::mutex _mutex;

    typedef TfHashSet<CacheEntry, HashFunctor> _CacheEntrySet;
    _CacheEntrySet _cachedEntries;
    
    PxOsdRefinerCache(PxOsdRefinerCache const&);
    PxOsdRefinerCache const& operator=(PxOsdRefinerCache const&);
    friend class TfSingleton<PxOsdRefinerCache>;
};

#endif // PXOSD_REFINER_CACHE_H
