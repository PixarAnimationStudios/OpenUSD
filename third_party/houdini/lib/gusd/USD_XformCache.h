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
#ifndef _GUSD_USD_XFORMCACHE_H_
#define _GUSD_USD_XFORMCACHE_H_

#include "gusd/defaultArray.h"
#include "gusd/UT_CappedCache.h"
#include "gusd/USD_DataCache.h"
#include "gusd/USD_Utils.h"

#include <pxr/pxr.h>
#include "pxr/usd/usdGeom/xformable.h"

PXR_NAMESPACE_OPEN_SCOPE

/** Concurrent memory-capped cache for primitive transforms.*/
class GusdUSD_XformCache final : public GusdUSD_DataCache
{
public:

    static GusdUSD_XformCache&  GetInstance();

    GusdUSD_XformCache(GusdStageCache& cache);
    GusdUSD_XformCache();

    virtual ~GusdUSD_XformCache() {}

    bool    GetLocalTransformation(const UsdPrim& prim,
                                   UsdTimeCode time,
                                   UT_Matrix4D& xform);

    bool    GetLocalToWorldTransform(const UsdPrim& prim,
                                     UsdTimeCode time,
                                     UT_Matrix4D& xform);

    /** Compute multiple local transforms in parallel. */
    bool    GetLocalTransformations(
                const UT_Array<UsdPrim>& prims,
                const GusdDefaultArray<UsdTimeCode>& times,
                UT_Matrix4D* xfroms);

    /** Compute multiple world transforms in parallel.*/
    bool    GetLocalToWorldTransforms(
                const UT_Array<UsdPrim>& prims,
                const GusdDefaultArray<UsdTimeCode>& times,
                UT_Matrix4D* xforms);

    /* Compute constraint transforms given a common constraint name
       for all prims. Constraint transforms not cached.*/
    bool    GetConstraintTransforms(
                const TfToken& constraint,
                const UT_Array<UsdPrim>& prims,
                const GusdDefaultArray<UsdTimeCode>& times,
                UT_Matrix4D* xforms);

    /* Given tokens representing *full* names of attributes
       (I.e., including the namespace), compute constraint transforms.
       Constraint transforms not cached.*/
    bool    GetConstraintTransforms(
                const UT_Array<TfToken>& constraints,
                const UT_Array<UsdPrim>& prims,
                const GusdDefaultArray<UsdTimeCode>& times,
                UT_Matrix4D* xforms);

    struct XformInfo : public UT_CappedItem
    {
        enum Flags
        {
            FLAGS_LOCAL_MAYBE_TIMEVARYING=0x1,
            FLAGS_WORLD_MAYBE_TIMEVARYING=0x2,
            FLAGS_HAS_PARENT_XFORM=0x4
        };

        XformInfo(const UsdGeomXformable& xf)
            : UT_CappedItem(), query(xf), _flags(0) {}

        virtual ~XformInfo() {}

        virtual int64           getMemoryUsage() const  { return sizeof(*this); }

        void                    ComputeFlags(const UsdPrim& prim,
                                             GusdUSD_XformCache& cache);

        SYS_FORCE_INLINE bool   LocalXformIsMaybeTimeVarying() const
                                { return _flags&FLAGS_LOCAL_MAYBE_TIMEVARYING; }

        SYS_FORCE_INLINE bool   WorldXformIsMaybeTimeVarying() const
                                { return _flags&FLAGS_WORLD_MAYBE_TIMEVARYING; }

        SYS_FORCE_INLINE bool   HasParentXform() const
                                { return _flags&FLAGS_HAS_PARENT_XFORM; }

        const UsdGeomXformable::XformQuery  query;
    private:
        int                                 _flags;
    };
    typedef UT_IntrusivePtr<const XformInfo>    XformInfoHandle;


    XformInfoHandle GetXformInfo(const UsdPrim& prim);


    virtual void    Clear() override;
    virtual int64   Clear(const UT_StringSet& paths) override;

private:
    bool    _GetLocalTransformation(const UsdPrim& prim,
                                    UsdTimeCode time,
                                    UT_Matrix4D& xform,
                                    const XformInfoHandle& info);
                                    


private:
    GusdUT_CappedCache  _xforms, _worldXforms, _xformInfos;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_XFORMCACHE_H_*/
