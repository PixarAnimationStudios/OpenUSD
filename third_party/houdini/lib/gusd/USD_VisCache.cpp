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
#include "gusd/USD_VisCache.h"

#include "gusd/USD_PropertyMap.h"
#include "gusd/USD_Utils.h"

PXR_NAMESPACE_OPEN_SCOPE

GusdUSD_VisCache::GusdUSD_VisCache(GusdUSD_StageCache& cache)
  : GusdUSD_DataCache(cache),
    _visInfos(GUSDUT_USDCACHE_NAME, 256)
{}


GusdUSD_VisCache::GusdUSD_VisCache()
  : GusdUSD_VisCache(GusdUSD_StageCache::GetInstance())
{}


GusdUSD_VisCache&
GusdUSD_VisCache::GetInstance()
{
    static GusdUSD_VisCache cache;
    return cache;
}


namespace {


enum VisFlags
{
    FLAGS_ISMAYBETIMEVARYING=0x1,
    FLAGS_RESOLVED_ISMAYBETIMEVARYING=0x2
};


enum VisType
{
    VIS_UNVARYING,
    VIS_VARYING,
    VIS_UNVARYING_RESOLVED,
    VIS_VARYING_RESOLVED
};

        
enum VisState
{
    STATE_VISIBLE=0x1,
    STATE_COMPUTED=0x2
};


int _GetStateFlags(int flags, VisType type)
    { return flags>>(2*type + 2); }

int _SetStateFlags(int flags, int stateFlags, VisType type)
    { return flags|(stateFlags<<(2*type + 2)); }


using _UnvaryingKey = GusdUT_CappedKey<GusdUSD_UnvaryingPropertyKey,
                                       GusdUSD_UnvaryingPropertyKey::HashCmp>;


bool _QueryVisibility(const UsdAttributeQuery& query, UsdTimeCode time)
{
    TfToken vis;
    query.Get(&vis, time);  
    return vis == UsdGeomTokens->inherited;
}


} /*namespace*/


GusdUSD_VisCache::VisInfoHandle
GusdUSD_VisCache::_GetVisInfo(const UsdPrim& prim)
{
    _UnvaryingKey key((GusdUSD_UnvaryingPropertyKey(prim)));

    if(UT_CappedItemHandle info = _visInfos.findItem(key))
        return VisInfoHandle(UTverify_cast<VisInfo*>(info.get()));
    /* XXX: Potential race in construction, but in the worst case that will
            just mean a few extra computes.*/

    UsdGeomImageable ip(prim);
    if(BOOST_UNLIKELY(!ip))
        return VisInfoHandle();

    UsdAttribute visAttr(ip.GetVisibilityAttr());

    int flags = 0;
    if(visAttr.ValueMightBeTimeVarying())
        flags |= FLAGS_ISMAYBETIMEVARYING|FLAGS_RESOLVED_ISMAYBETIMEVARYING;
    else if(UsdPrim parent = prim.GetParent()) {
        
        if(auto parentInfo = _GetVisInfo(parent)) {
            if(parentInfo->flags.relaxedLoad()&
               FLAGS_RESOLVED_ISMAYBETIMEVARYING) {
                flags |= FLAGS_RESOLVED_ISMAYBETIMEVARYING;
            }
        }
    }
    return VisInfoHandle(
        UTverify_cast<VisInfo*>(
            _visInfos.addItem(
                key, UT_CappedItemHandle(
                    new VisInfo(flags, visAttr))).get()));
}


bool
GusdUSD_VisCache::GetVisibility(const UsdPrim& prim,
                                UsdTimeCode time)
{
    if(auto info = _GetVisInfo(prim)) {
        int flags = info->flags.relaxedLoad();
        bool vis = true;
        if(_GetVisibility(flags, info->query, time, vis))
            info->flags.store(flags);
        return vis;
    }
    return false;
}


bool
GusdUSD_VisCache::_GetVisibility(int& flags,
                                 const UsdAttributeQuery& query,
                                 UsdTimeCode time,
                                 bool& vis)
{
    if(!(flags&FLAGS_ISMAYBETIMEVARYING) || time.IsDefault()) {
        VisType visType = time.IsDefault() ? VIS_UNVARYING : VIS_VARYING;
        int stateFlags = _GetStateFlags(flags, visType);
        if(stateFlags&STATE_COMPUTED) {
            vis = stateFlags&STATE_VISIBLE;
            return false;
        } else {
            if(vis == _QueryVisibility(query, time))
                stateFlags |= STATE_VISIBLE;
            flags = _SetStateFlags(flags, stateFlags|STATE_COMPUTED, visType);
        }
    } else {
        vis = _QueryVisibility(query, time);
        return false;
    }
    return true;
}


bool
GusdUSD_VisCache::GetResolvedVisibility(const UsdPrim& prim,
                                        UsdTimeCode time)
{
    auto info = _GetVisInfo(prim);
    if(BOOST_UNLIKELY(!info))
        return false;

    int flags = info->flags.relaxedLoad();
    if(!(flags&FLAGS_RESOLVED_ISMAYBETIMEVARYING) || time.IsDefault()) {
        VisType visType = time.IsDefault() ?
            VIS_UNVARYING_RESOLVED : VIS_VARYING_RESOLVED;
        int stateFlags = _GetStateFlags(flags, visType);
        if(stateFlags&STATE_COMPUTED) {
            return stateFlags&STATE_VISIBLE;
        } else {
            bool vis = true;
            _GetVisibility(flags, info->query, time, vis);
            if(vis) {
                if(UsdPrim parent = prim.GetParent()) {
                    if(parent.GetPath() != SdfPath::AbsoluteRootPath())
                        vis = GetResolvedVisibility(parent, time);
                }
            }
            if(vis)
                stateFlags |= STATE_VISIBLE;
            flags = _SetStateFlags(flags, stateFlags|STATE_COMPUTED, visType);
            info->flags.store(flags);
            return vis;
        }
    } else {
        if(_QueryVisibility(info->query, time)) {
            if(UsdPrim parent = prim.GetParent()) {
                if(parent.GetPath() != SdfPath::AbsoluteRootPath())
                    return GetResolvedVisibility(parent, time);
            }
            return true;
        }
        return false;
    }
}


void
GusdUSD_VisCache::Clear()
{
    _visInfos.clear();
}


namespace {


template <typename KeyT>
int64
_RemoveKeysT(const UT_Set<std::string>& paths, GusdUT_CappedCache& cache)
{
    return cache.ClearEntries(
        [&](const UT_CappedKeyHandle& key,
            const UT_CappedItemHandle& item) {

        return GusdUSD_DataCache::ShouldClearPrim(
            (*UTverify_cast<const KeyT*>(key.get()))->prim,
            paths);
    });
}


} /*namespace*/


int64
GusdUSD_VisCache::Clear(const UT_Set<std::string>& paths)
{   
    return _RemoveKeysT<_UnvaryingKey>(paths, _visInfos);
}

PXR_NAMESPACE_CLOSE_SCOPE
