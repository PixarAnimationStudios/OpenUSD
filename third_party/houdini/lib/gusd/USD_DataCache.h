//
// Copyright 2017 Pixar
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
#ifndef _GUSD_USD_DATACACHE_H_
#define _GUSD_USD_DATACACHE_H_

#include "gusd/api.h"
#include "gusd/stageCache.h"

#include <pxr/pxr.h>
#include "pxr/base/tf/token.h"

#include <UT/UT_StringSet.h>


PXR_NAMESPACE_OPEN_SCOPE


#define GUSDUT_USDCACHE_NAME "USD Cache"

class GusdStageCache;


class GUSD_API GusdUSD_DataCache
{
public:
    GusdUSD_DataCache(GusdStageCache& cache);
    GusdUSD_DataCache();

    virtual ~GusdUSD_DataCache();

    /// Clear all caches.
    virtual void    Clear() {}

    /// Clear caches for a set of stages by path    
    virtual int64   Clear(const UT_StringSet& stagePaths) { return 0; }


    /// Helper for implementations to decide if a cache entry
    /// corresponding to @a prim should be discarded.
    static bool     ShouldClearPrim(
                        const UsdPrim& prim,
                        const UT_StringSet& stagesToClear);

protected:
    GusdStageCache& _stageCache;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif /*_GUSD_USD_DATACACHE_H_*/
