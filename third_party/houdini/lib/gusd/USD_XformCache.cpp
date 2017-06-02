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
#include "gusd/USD_XformCache.h"

#include "gusd/USD_PropertyMap.h"
#include "gusd/UT_Gf.h"
#include "gusd/UT_CappedCache.h"

#include <UT/UT_Interrupt.h>
#include <UT/UT_Matrix4.h>
#include <UT/UT_ParallelUtil.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {


typedef GusdUT_CappedKey<GusdUSD_UnvaryingPropertyKey,
                         GusdUSD_UnvaryingPropertyKey::HashCmp> _UnvaryingKey;

typedef GusdUT_CappedKey<GusdUSD_VaryingPropertyKey,
                         GusdUSD_VaryingPropertyKey::HashCmp> _VaryingKey;


struct _CappedXformItem : public UT_CappedItem
{
    _CappedXformItem(const UT_Matrix4D& xform)
        : UT_CappedItem(), xform(xform) {}

    virtual ~_CappedXformItem() {}

    virtual int64   getMemoryUsage() const  { return sizeof(*this); }

    const UT_Matrix4D   xform;
};

typedef UT_IntrusivePtr<const _CappedXformItem> _CappedXformItemHandle;

static inline void intrusive_ptr_add_ref(const _CappedXformItem *o) { const_cast<_CappedXformItem *>(o)->incref(); }
static inline void intrusive_ptr_release(const _CappedXformItem *o) { const_cast<_CappedXformItem *>(o)->decref(); }

} /*namespace*/

static inline void intrusive_ptr_add_ref(const GusdUSD_XformCache::XformInfo *o) { const_cast<GusdUSD_XformCache::XformInfo *>(o)->incref(); }
static inline void intrusive_ptr_release(const GusdUSD_XformCache::XformInfo *o) { const_cast<GusdUSD_XformCache::XformInfo *>(o)->decref(); }

void
GusdUSD_XformCache::XformInfo::ComputeFlags(const UsdPrim& prim,
                                            GusdUSD_XformCache& cache)
{
    if(query.TransformMightBeTimeVarying()) {
        _flags = FLAGS_LOCAL_MAYBE_TIMEVARYING|
                 FLAGS_WORLD_MAYBE_TIMEVARYING;
    } else {
        /* Local transform isn't time-varying, but maybe the parent is.*/
        if(!query.GetResetXformStack()) {
            UsdPrim parent = prim.GetParent();
            if(parent && parent.GetPath() != SdfPath::AbsoluteRootPath()) {
                auto info = cache.GetXformInfo(parent);
                if(info && info->WorldXformIsMaybeTimeVarying())
                    _flags = FLAGS_WORLD_MAYBE_TIMEVARYING;
            }
        }
    }
    
    if(!query.GetResetXformStack()) {
        UsdPrim parent = prim.GetParent();
        if(parent && parent.GetPath() != SdfPath::AbsoluteRootPath())
            _flags |= FLAGS_HAS_PARENT_XFORM;
    }
}


GusdUSD_XformCache&
GusdUSD_XformCache::GetInstance()
{
    static GusdUSD_XformCache cache;
    return cache;
}


GusdUSD_XformCache::XformInfoHandle
GusdUSD_XformCache::GetXformInfo(const UsdPrim& prim)
{
    _UnvaryingKey key((GusdUSD_UnvaryingPropertyKey(prim)));

    if(auto item = _xformInfos.findItem(key))
        return XformInfoHandle(UTverify_cast<XformInfo*>(item.get()));

    auto* info = new XformInfo(UsdGeomXformable(prim));
    info->ComputeFlags(prim, *this);
    auto item = UT_CappedItemHandle(info);
    return XformInfoHandle(UTverify_cast<XformInfo*>(
                               _xformInfos.addItem(key,item).get()));
}


bool
GusdUSD_XformCache::GetLocalTransformation(const UsdPrim& prim,
                                           UsdTimeCode time,
                                           UT_Matrix4D& xform)
{
    const auto info = GetXformInfo(prim);
    if(BOOST_UNLIKELY(!info))
        return false;
    return _GetLocalTransformation(prim, time, xform, info);
}


bool
GusdUSD_XformCache::_GetLocalTransformation(const UsdPrim& prim,
                                            UsdTimeCode time,
                                            UT_Matrix4D& xform,
                                            const XformInfoHandle& info)
{
    // See if we can remap the time to for unvarying xforms.
    if(!time.IsDefault() && !info->LocalXformIsMaybeTimeVarying()) {
        /* XXX: we know we're not time varying, but that doesn't
           mean that we can key default, since there might still
           be a single varying value that we'd miss.
           Key off of time=0 instead.*/
        time = UsdTimeCode(0.0);
    }
    _VaryingKey key(GusdUSD_VaryingPropertyKey(prim, time));

    if(auto item = _xforms.findItem(key)) {
        xform = UTverify_cast<const _CappedXformItem*>(item.get())->xform;
        return true;
    }
    /* XXX: Race is possible when setting computed value,
       but it's preferable to have multiple threads compute the
       same thing than to cause lock contention.*/
    if(info->query.GetLocalTransformation(GusdUT_Gf::Cast(&xform), time)) {
        _xforms.addItem(key, UT_CappedItemHandle(new _CappedXformItem(xform)));
        return true;
    }
    return false;
}


bool
GusdUSD_XformCache::GetLocalToWorldTransform(const UsdPrim& prim,
                                             UsdTimeCode time,
                                             UT_Matrix4D& xform)
{
    const auto info = GetXformInfo(prim);
    if(BOOST_UNLIKELY(!info))
        return false;

    // See if we can remap the time to for unvarying xforms.
    if(!time.IsDefault() && !info->WorldXformIsMaybeTimeVarying()) {
        /* XXX: we know we're not time varying, but that doesn't
           mean that we can key default, since there might still
           be a single varying value that we'd miss.
           Key off of time=0 instead.*/
        time = UsdTimeCode(0.0);
    }
    _VaryingKey key(GusdUSD_VaryingPropertyKey(prim, time));

    if(auto item = _worldXforms.findItem(key)) {
        xform = UTverify_cast<const _CappedXformItem*>(item.get())->xform;
        return true;
    }
    /* XXX: Race is possible when setting computed value,
       but it's preferable to have multiple threads compute the
       same thing than to cause lock contention.*/
    if(_GetLocalTransformation(prim, time, xform, info)) {
        if(BOOST_UNLIKELY(!info->HasParentXform())) {
            _worldXforms.addItem(key, UT_CappedItemHandle(
                                     new _CappedXformItem(xform)));
            return true;
        }
        UsdPrim parent = prim.GetParent();
        UT_ASSERT_P(parent);

        UT_Matrix4D parentXf;
        if(GetLocalToWorldTransform(parent, time, parentXf)) {
            xform *= parentXf;
            _worldXforms.addItem(
                key, UT_CappedItemHandle(new _CappedXformItem(xform)));
            return true;
        }
    }
    return false;
}



GusdUSD_XformCache::GusdUSD_XformCache(GusdUSD_StageCache& cache)
    : GusdUSD_DataCache(cache),
      _xforms(GUSDUT_USDCACHE_NAME, 512),
      _worldXforms(GUSDUT_USDCACHE_NAME, 512),
      _xformInfos(GUSDUT_USDCACHE_NAME, 256) {}

    
GusdUSD_XformCache::GusdUSD_XformCache()
    : GusdUSD_XformCache(GusdUSD_StageCache::GetInstance())
{}


namespace {


/** Functor for computing transforms from USD prims.*/
template <typename XformFn>
struct _ComputeXformsT
{
    _ComputeXformsT(const XformFn& xformFn,
                    const GusdUSD_StageProxy::MultiAccessor& accessor,
                    const UT_Array<UsdPrim>& prims,
                    const GusdUSD_Utils::PrimTimeMap& timeMap,
                    UT_Matrix4D* xforms)
        : _xformFn(xformFn), _accessor(accessor),
          _prims(prims), _timeMap(timeMap), _xforms(xforms) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
            {
                auto* boss = UTgetInterrupt();
                char bcnt = 0;

                for(size_t i = r.begin(); i < r.end(); ++i) {
                    if(!++bcnt && boss->opInterrupt())
                        return;
                    if(const auto* accessor = _accessor(i)) {
                        if(UsdPrim prim = _prims(i)) {
                            _xformFn(_xforms[i],  prim, _timeMap(i), i);
                            continue;
                        }
                    }
                    _xforms[i].identity();
                }
            }

private:
    const XformFn&                              _xformFn;
    const GusdUSD_StageProxy::MultiAccessor&    _accessor;
    const UT_Array<UsdPrim>&                    _prims;
    const GusdUSD_Utils::PrimTimeMap&           _timeMap;
    UT_Matrix4D* const                          _xforms;
};


struct _WorldXformFn
{
    _WorldXformFn(GusdUSD_XformCache& cache)
        : _cache(cache) {}

    void    operator()(UT_Matrix4D& xf,
                       const UsdPrim& prim, UsdTimeCode time, size_t i) const
            {
                if(!_cache.GetLocalToWorldTransform(prim, time, xf))
                    xf.identity();
            }

private:
    GusdUSD_XformCache& _cache;
};


struct _LocalXformFn
{
    _LocalXformFn(GusdUSD_XformCache& cache)
        : _cache(cache) {}

    void    operator()(UT_Matrix4D& xf,
                       const UsdPrim& prim, UsdTimeCode time, size_t i) const
            {
                if(!_cache.GetLocalTransformation(prim, time, xf))
                    xf.identity();
            }

private:
    GusdUSD_XformCache& _cache;
};


template <typename XformFn>
bool
_ComputeXforms(const XformFn& xformFn,
               const GusdUSD_StageProxy::MultiAccessor& accessor,
               const UT_Array<UsdPrim>& prims,
               const GusdUSD_Utils::PrimTimeMap& timeMap,
               UT_Matrix4D* xforms)
{
    UT_ASSERT(accessor.size() == prims.size());
    UTparallelFor(UT_BlockedRange<size_t>(0, prims.size()),
                  _ComputeXformsT<XformFn>(xformFn, accessor, prims,
                                           timeMap, xforms));
    return !UTgetInterrupt()->opInterrupt();
}


} /*namespace*/


bool
GusdUSD_XformCache::GetLocalTransformations(
    const GusdUSD_StageProxy::MultiAccessor& accessor,
    const UT_Array<UsdPrim>& prims,
    const GusdUSD_Utils::PrimTimeMap& timeMap,
    UT_Matrix4D* xforms)
{
    return _ComputeXforms<_LocalXformFn>(_LocalXformFn(*this), accessor,
                                         prims, timeMap, xforms);
}


bool
GusdUSD_XformCache::GetLocalToWorldTransforms(
    const GusdUSD_StageProxy::MultiAccessor& accessor,
    const UT_Array<UsdPrim>& prims,
    const GusdUSD_Utils::PrimTimeMap& timeMap,
    UT_Matrix4D* xforms)
{
    return _ComputeXforms<_WorldXformFn>(_WorldXformFn(*this), accessor,
                                         prims, timeMap, xforms);
}


namespace {


template <typename NameFn>
struct _QueryConstraintsT
{
    _QueryConstraintsT(const NameFn& nameFn,
                       const GusdUSD_StageProxy::MultiAccessor& accessor,
                       const UT_Array<UsdPrim>& prims,
                       const GusdUSD_Utils::PrimTimeMap& timeMap,
                       UT_Matrix4D* xforms)
        : _nameFn(nameFn), _accessor(accessor), _prims(prims),
          _timeMap(timeMap), _xforms(xforms) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
            {
                auto* boss = UTgetInterrupt();
                char bcnt = 0;

                for(size_t i = r.begin(); i < r.end(); ++i) {
                    if(!++bcnt && boss->opInterrupt())
                        return;
                    if(UsdPrim prim = _prims(i)) {
                        const TfToken& name = _nameFn(i);
                        if(!name.IsEmpty()) {
                            if(prim.GetAttribute(name).Get(
                                   GusdUT_Gf::Cast(_xforms + i), _timeMap(i)))
                                continue;
                        }
                    }
                    _xforms[i].identity();
                }
            }


private:
    const NameFn&                               _nameFn;
    const GusdUSD_StageProxy::MultiAccessor&    _accessor;
    const UT_Array<UsdPrim>&                    _prims;
    const GusdUSD_Utils::PrimTimeMap&           _timeMap;
    UT_Matrix4D* const                          _xforms;
};


struct _SingleNameFn
{
    _SingleNameFn(const TfToken& name) : _name(name) {}
    
    const TfToken&  operator()(exint i) const   { return _name; }
private:
    const TfToken&  _name;
};


struct _ArrayOfNamesFn
{
    _ArrayOfNamesFn(const UT_Array<TfToken>& names) : _names(names) {}

    const TfToken&  operator()(exint i) const   { return _names(i); }
private:
    const UT_Array<TfToken>&    _names;
};


template <typename NameFn>
bool
_QueryConstraints(const NameFn& nameFn,
                  const GusdUSD_StageProxy::MultiAccessor& accessor,
                  const UT_Array<UsdPrim>& prims,
                  const GusdUSD_Utils::PrimTimeMap& timeMap,
                  UT_Matrix4D* xforms)
{
    UTparallelFor(UT_BlockedRange<size_t>(0, prims.size()),
                  _QueryConstraintsT<NameFn>(nameFn, accessor, prims,
                                             timeMap, xforms));
    return !UTgetInterrupt()->opInterrupt();
}


} /*namespace*/


bool
GusdUSD_XformCache::GetConstraintTransforms(
    const TfToken& constraint,
    const GusdUSD_StageProxy::MultiAccessor& accessor,
    const UT_Array<UsdPrim>& prims,
    const GusdUSD_Utils::PrimTimeMap& timeMap,
    UT_Matrix4D* xforms)
{
    return _QueryConstraints<_SingleNameFn>(_SingleNameFn(constraint),
                                            accessor, prims, timeMap, xforms);
}


bool
GusdUSD_XformCache::GetConstraintTransforms(
    const UT_Array<TfToken>& constraints,
    const GusdUSD_StageProxy::MultiAccessor& accessor,
    const UT_Array<UsdPrim>& prims,
    const GusdUSD_Utils::PrimTimeMap& timeMap,
    UT_Matrix4D* xforms)
{
    return _QueryConstraints<_ArrayOfNamesFn>(_ArrayOfNamesFn(constraints),
                                              accessor, prims, timeMap, xforms);
}


void
GusdUSD_XformCache::Clear()
{
    _xforms.clear();
    _worldXforms.clear();
    _xformInfos.clear();
}


namespace {


template <typename KeyT>
int64
_RemoveKeysT(const UT_Set<std::string>& paths,
             GusdUT_CappedCache& cache)
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
GusdUSD_XformCache::Clear(const UT_Set<std::string>& paths)
{   
    return _RemoveKeysT<_VaryingKey>(paths, _xforms) +
           _RemoveKeysT<_VaryingKey>(paths, _worldXforms ) +
           _RemoveKeysT<_UnvaryingKey>(paths, _xformInfos);
}

PXR_NAMESPACE_CLOSE_SCOPE
