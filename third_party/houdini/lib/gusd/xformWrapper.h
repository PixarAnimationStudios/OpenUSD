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
#ifndef __GUSD_XFORMWRAPPER_H__
#define __GUSD_XFORMWRAPPER_H__

#include <pxr/pxr.h>

#include "groupBaseWrapper.h"
#include <pxr/usd/usdGeom/xform.h>

PXR_NAMESPACE_OPEN_SCOPE

typedef GusdUSD_HolderT<UsdGeomXform> GusdUSD_XformHolder;

class GusdXformWrapper : public GusdGroupBaseWrapper
{
public:
    GusdXformWrapper( 
            const UsdStagePtr& stage,
            const SdfPath& path,
            bool isOverride = false );
    GusdXformWrapper( const GusdXformWrapper& in );
    GusdXformWrapper( 
            const GusdUSD_StageProxyHandle& stage, 
            const UsdGeomXform&             usdXform, 
            const UsdTimeCode&              t,
            const GusdPurposeSet&           purposes );
    virtual ~GusdXformWrapper();

    // GusdPrimWrapper interface -----------------------------------------------

public:

    virtual const UsdGeomImageable getUsdPrimForWrite() const override { return m_usdXformForWrite; }

    virtual const UsdGeomImageable 
        getUsdPrimForRead(GusdUSD_ImageableHolder::ScopedLock &lock) const override;
        
    virtual bool redefine( 
           const UsdStagePtr& stage,
           const SdfPath& path,
           const GusdContext& ctxt,
           const GT_PrimitiveHandle& sourcePrim ) override;
    
    virtual bool getUniqueID(int64& id) const override;
    
    virtual const char* className() const override;

    virtual void enlargeBounds(UT_BoundingBox boxes[], int nsegments) const override;

    virtual int getMotionSegments() const override;

    virtual int64 getMemoryUsage() const override;

    virtual GT_PrimitiveHandle doSoftCopy() const override;

    virtual bool
    updateFromGTPrim(const GT_PrimitiveHandle& sourcePrim,
                     const UT_Matrix4D&        localXform,
                     const GusdContext&        ctxt,
                     GusdSimpleXformCache&     xformCache) override;

    virtual bool isValid() const override;

    virtual bool refine(GT_Refine& refiner,
                        const GT_RefineParms* parms=NULL) const override;

    // -------------------------------------------------------------------------
    
public:
    
    static GT_PrimitiveHandle
    defineForWrite(const GT_PrimitiveHandle& sourcePrim,
                     const UsdStagePtr& stage,
                     const SdfPath& path,
                     const GusdContext& ctxt);

    static GT_PrimitiveHandle
    defineForRead( const GusdUSD_StageProxyHandle&  stage,
                   const UsdGeomImageable&          sourcePrim, 
                   const UsdTimeCode&               time,
                   const GusdPurposeSet&            purposes );

private:

    bool initUsdPrim(const UsdStagePtr& stage,
                     const SdfPath& path,
                     bool asOverride);

    GusdUSD_StageProxyHandle    m_stageProxy;
    GusdUSD_XformHolder         m_usdXformForRead;
    UsdGeomXformable            m_usdXformForWrite;
};

PXR_NAMESPACE_CLOSE_SCOPE
#endif // __GUSD_XFORMWRAPPER_H__

