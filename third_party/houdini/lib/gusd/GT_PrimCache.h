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
#ifndef __GUSD_GT_PRIMCACHE_H__
#define __GUSD_GT_PRIMCACHE_H__

#include "gusd/api.h"

#include "purpose.h"
#include "USD_DataCache.h"
#include "UT_CappedCache.h"

#include <GT/GT_Primitive.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Cache of refined GT prims created draw USD in the viewport. 
//
// We cache a GT prim for each imageable (leaf node) USD prim and each instance.
// The GT prims are refined to prims that can be directly imaged in the Houdini 
// view port.
// The cache is build atop a UT_CappedCache (a LRU cache).

class GusdGT_PrimCache : public GusdUSD_DataCache {

public:

    GUSD_API
    static GusdGT_PrimCache&  GetInstance();

    GusdGT_PrimCache();
    virtual ~GusdGT_PrimCache();

    GT_PrimitiveHandle GetPrim( const UsdPrim &usdPrim, 
                                UsdTimeCode time, 
                                GusdPurposeSet purposes,
                                bool skipRoot = false );

    virtual void    Clear() override;
    virtual int64   Clear(const UT_StringSet& paths) override;

private:

    GusdUT_CappedCache _prims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_GT_PRIMCACHE_H__ 
