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
#ifndef _GUSD_USD_VISCACHE_
#define _GUSD_USD_VISCACHE_

#include "gusd/api.h"

#include "gusd/USD_DataCache.h"
#include "gusd/UT_CappedCache.h"

#include <SYS/SYS_AtomicInt.h>

#include <pxr/pxr.h>
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usdGeom/imageable.h"

PXR_NAMESPACE_OPEN_SCOPE

/** Thread-safe, memory-capped visibility cache.
    This does not cache varying visibility state; only unvarying visibility
    values and information about whether or not visibility might vary
    with time is cached.*/
class GusdUSD_VisCache final : public GusdUSD_DataCache
{
public:

    GUSD_API
    static GusdUSD_VisCache&    GetInstance();

    GusdUSD_VisCache(GusdStageCache& cache);
    GusdUSD_VisCache();

    virtual ~GusdUSD_VisCache() {}

    // Not cached.
    GUSD_API
    bool    GetVisibility(const UsdPrim& prim, UsdTimeCode time);
    
    // Cached.
    GUSD_API
    bool    GetResolvedVisibility(const UsdPrim& prim, UsdTimeCode time);

    GUSD_API
    virtual void    Clear() override;

    GUSD_API
    virtual int64   Clear(const UT_StringSet& paths) override;

private:
    struct VisInfo : public UT_CappedItem
    {
        VisInfo(int flags, const UsdAttribute& attr)
            : UT_CappedItem(), flags(flags), query(attr) {}

        virtual ~VisInfo() {}

        virtual int64   getMemoryUsage() const  { return sizeof(*this); }
        
        SYS_AtomicInt32     flags;
        UsdAttributeQuery   query;
    };
    typedef UT_IntrusivePtr<VisInfo> VisInfoHandle;

    VisInfoHandle   _GetVisInfo(const UsdPrim& prim);

    /** Query visibility. Returns true if @a flags were modified.*/
    bool            _GetVisibility(int& flags,
                                   const UsdAttributeQuery& query,
                                   UsdTimeCode time,
                                   bool& vis);

    GusdUT_CappedCache  _visInfos;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_VISCACHE_*/
