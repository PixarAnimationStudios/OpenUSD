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
#include "scopeWrapper.h"

#include "context.h"
#include "UT_Gf.h"
#include "USD_Proxy.h"


#include <GT/GT_PrimInstance.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_Refine.h>
#include <GT/GT_PrimCollect.h>

#include <boost/foreach.hpp>

PXR_NAMESPACE_OPEN_SCOPE

// drand48 and srand48 defined in SYS_Math.h as of 13.5.153. and conflicts with imath.
#undef drand48
#undef srand48

using std::cout;
using std::cerr;
using std::endl;
using std::vector;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

GusdScopeWrapper::GusdScopeWrapper(
        const UsdStagePtr& stage,
        const SdfPath& path,
        bool isOverride )
{
    initUsdPrim( stage, path, isOverride );
}

GusdScopeWrapper::GusdScopeWrapper( 
        const GusdUSD_StageProxyHandle& stage, 
        const UsdGeomScope&             scope, 
        const UsdTimeCode&              time,
        const GusdPurposeSet&           purposes )
    : GusdGroupBaseWrapper( time, purposes )
    , m_usdScopeForRead( scope, stage->GetLock() )
    , m_stageProxy( stage )
{
}

GusdScopeWrapper::GusdScopeWrapper( const GusdScopeWrapper &in )
    : GusdGroupBaseWrapper( in )
    , m_usdScopeForRead( in.m_usdScopeForRead )
    , m_usdScopeForWrite( in.m_usdScopeForWrite )
    , m_stageProxy( in.m_stageProxy )
{
}

GusdScopeWrapper::~GusdScopeWrapper()
{}

bool GusdScopeWrapper::
initUsdPrim(const UsdStagePtr& stage,
            const SdfPath& path,
            bool asOverride)
{
    bool newPrim = true;
    if( asOverride ) {
        UsdPrim existing = stage->GetPrimAtPath( path );
        if( existing ) {
            // Note that we are creating a Xformable rather than a Xform. 
            // If we are writing an overlay and the ROP sees a geometry packed prim,
            // we want to write just the xform. In that case we can use a xform
            // wrapper to write the xform on any prim type.
            m_usdScopeForWrite = UsdGeomScope(stage->OverridePrim( path ));
            newPrim = false;
        }
        else {
            m_usdScopeForWrite = UsdGeomScope::Define( stage, path );   
        }
    }
    else {
        m_usdScopeForWrite = UsdGeomScope::Define( stage, path );
    }
    if( !m_usdScopeForWrite || !m_usdScopeForWrite.GetPrim().IsValid() ) {
        TF_WARN( "Unable to create %s scope '%s'.", newPrim ? "new" : "override", path.GetText() );
    }
    return bool(m_usdScopeForWrite);
}

GT_PrimitiveHandle GusdScopeWrapper::
defineForWrite(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt)
{
    return new GusdScopeWrapper( stage, path, ctxt.overlayGeo );
}

GT_PrimitiveHandle GusdScopeWrapper::
defineForRead(
        const GusdUSD_StageProxyHandle& stage,
        const UsdGeomImageable&         sourcePrim, 
        const UsdTimeCode&              time,
        const GusdPurposeSet&           purposes )
{
    return new GusdScopeWrapper( 
                    stage, 
                    UsdGeomScope( sourcePrim.GetPrim() ),
                    time,
                    purposes );
}

bool GusdScopeWrapper::
redefine( const UsdStagePtr& stage,
          const SdfPath& path,
          const GusdContext& ctxt,
          const GT_PrimitiveHandle& sourcePrim )
{
    initUsdPrim( stage, path, ctxt.overlayGeo );
    clearCaches();
    return true;
}

bool GusdScopeWrapper::
getUniqueID(int64& id) const
{
    static const int s_id = GT_Primitive::createPrimitiveTypeId();
    id = s_id;
    return true;
}

const char* GusdScopeWrapper::
className() const
{
    return "GusdScopeWrapper";
}


void GusdScopeWrapper::
enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    // TODO
}


int GusdScopeWrapper::
getMotionSegments() const
{
    // TODO
    return 1;
}


int64 GusdScopeWrapper::
getMemoryUsage() const
{
    // TODO
    return 0;
}


GT_PrimitiveHandle GusdScopeWrapper::
doSoftCopy() const
{
    return GT_PrimitiveHandle(new GusdScopeWrapper( *this ));
}


bool GusdScopeWrapper::
isValid() const
{
    return m_usdScopeForWrite || m_usdScopeForRead;
}

const UsdGeomImageable 
GusdScopeWrapper::getUsdPrimForRead( GusdUSD_ImageableHolder::ScopedLock &lock) const
{
    // obtain first lock to get geomtry as UsdGeomScope.
    GusdUSD_ScopeHolder::ScopedReadLock innerLock;
    innerLock.Acquire( m_usdScopeForRead );

    // Build new holder after casting to imageable
    GusdUSD_ImageableHolder tmp( UsdGeomImageable( (*innerLock).GetPrim() ),
                                 m_usdScopeForRead.GetLock() );
    lock.Acquire(tmp, /*write*/false);
    return *lock;
}

bool GusdScopeWrapper::
refine(
    GT_Refine& refiner,
    const GT_RefineParms* parms) const
{
    //cerr << "GusdScopeWrapper::refine, enter: " << m_usdScope.GetPath() << endl;

    GusdUSD_ScopeHolder::ScopedReadLock lock;
    lock.Acquire( m_usdScopeForRead );

    UsdGeomScope scope = *lock;

    return refineGroup( m_stageProxy, (*lock).GetPrim(), refiner, parms );
}


bool GusdScopeWrapper::
updateFromGTPrim(const GT_PrimitiveHandle& sourcePrim,
                 const UT_Matrix4D&        localXform,
                 const GusdContext&        ctxt,
                 GusdSimpleXformCache&     xformCache)
{
    if( !m_usdScopeForWrite ) {
        return false;
    }

    DBG( cout << "GusdScopeWrapper::updateFromGTPrim, primType = " << sourcePrim->className() << endl );

    return updateGroupFromGTPrim( m_usdScopeForWrite, sourcePrim, localXform, 
                                  ctxt, xformCache );
}

PXR_NAMESPACE_CLOSE_SCOPE

