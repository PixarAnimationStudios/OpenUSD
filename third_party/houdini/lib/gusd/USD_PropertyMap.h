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
#ifndef _GUSD_USD_PROPERTYMAP_H_
#define _GUSD_USD_PROPERTYMAP_H_

#include <UT/UT_ConcurrentHashMap.h>

#include <pxr/pxr.h>
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/** TBB-style hash key for time-varying prim properties.*/
struct GusdUSD_VaryingPropertyKey
{
    GusdUSD_VaryingPropertyKey() : hash(0) {}
    
    GusdUSD_VaryingPropertyKey(const UsdPrim& prim, UsdTimeCode time)
        : prim(prim)
        , time(time)
        , hash(ComputeHash(prim, time)) {}

    static std::size_t  ComputeHash(const UsdPrim& prim, UsdTimeCode time)
                        {
                            std::size_t h = hash_value(prim);
                            boost::hash_combine(h, time);
                            return h; 
                        }

    bool                operator==(const GusdUSD_VaryingPropertyKey& o) const
                        { return prim == o.prim && time == o.time; }

    friend size_t       hash_value(const GusdUSD_VaryingPropertyKey& o)
                        { return o.hash; }

    struct HashCmp
    {
        static std::size_t  hash(const GusdUSD_VaryingPropertyKey& key)
                            { return key.hash; }
        static bool         equal(const GusdUSD_VaryingPropertyKey& a,
                                  const GusdUSD_VaryingPropertyKey& b)
                            { return a == b; }
    };

    UsdPrim     prim;
    UsdTimeCode time;
    std::size_t hash;
};


/** Concurrent hash map for holding a time-varying property on a prim.*/
template <typename T>
class GusdUSD_VaryingPropertyMap
    : public UT_ConcurrentHashMap<GusdUSD_VaryingPropertyKey, T,
                                  GusdUSD_VaryingPropertyKey::HashCmp>
{
public:
    typedef GusdUSD_VaryingPropertyKey          Key;
    typedef GusdUSD_VaryingPropertyKey::HashCmp HashCmp;
    typedef T value_type;

    GusdUSD_VaryingPropertyMap() :
        UT_ConcurrentHashMap<Key,T,HashCmp>() {}
};


/** TBB-style hash key for unvarying prim properties.*/
struct GusdUSD_UnvaryingPropertyKey
{
    GusdUSD_UnvaryingPropertyKey() {}
    
    GusdUSD_UnvaryingPropertyKey(const UsdPrim& prim)
        : prim(prim) {}

    static std::size_t  ComputeHash(const UsdPrim& prim)
                        { return hash_value(prim); }

    bool                operator==(const GusdUSD_UnvaryingPropertyKey& o) const
                        { return prim == o.prim; }

    friend size_t       hash_value(const GusdUSD_UnvaryingPropertyKey& o)
                        { return hash_value(o.prim); }

    struct HashCmp
    {
        static std::size_t  hash(const GusdUSD_UnvaryingPropertyKey& key)
                            { return hash_value(key.prim); }
        static bool         equal(const GusdUSD_UnvaryingPropertyKey& a,
                                  const GusdUSD_UnvaryingPropertyKey& b)
                            { return a == b; }
    };
    
    UsdPrim prim;
};


/** Concurrent hash map for holding an unvarying property of a prim.*/
template <typename T>
class GusdUSD_UnvaryingPropertyMap
    : public UT_ConcurrentHashMap<GusdUSD_UnvaryingPropertyKey, T,
                                  GusdUSD_UnvaryingPropertyKey::HashCmp>
{
public:
    typedef GusdUSD_UnvaryingPropertyKey            Key;
    typedef GusdUSD_UnvaryingPropertyKey::HashCmp   HashCmp;
    typedef T value_type;

    GusdUSD_UnvaryingPropertyMap() :
        UT_ConcurrentHashMap<Key,T,HashCmp>() {}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_PROPERTYMAP_H_*/
