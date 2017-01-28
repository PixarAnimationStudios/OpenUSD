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
#ifndef PXOSD_REFINER_CACHE_H
#define PXOSD_REFINER_CACHE_H

/// \file pxOsd/refinerCache.h

#include "pxr/pxr.h"
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

PXR_NAMESPACE_OPEN_SCOPE


/// \class PxOsdRefinerCache
///
/// An API remnant to be removed soon. Does no caching at all.
/// 
///
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
    
    PxOsdRefinerCache() {
        TfSingleton< PxOsdRefinerCache >::SetInstanceConstructed(
            *this);
    }

    ~PxOsdRefinerCache() {
    }
    
    PxOsdRefinerCache(PxOsdRefinerCache const&);
    PxOsdRefinerCache const& operator=(PxOsdRefinerCache const&);
    friend class TfSingleton<PxOsdRefinerCache>;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXOSD_REFINER_CACHE_H
