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
#include "xformWrapper.h"

#include "context.h"
#include "UT_Gf.h"

#include <GT/GT_PrimInstance.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_Refine.h>
#include <GT/GT_PrimCollect.h>

#include <boost/foreach.hpp>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// drand48 and srand48 defined in SYS_Math.h as of 13.5.153. and conflicts with imath.
#undef drand48
#undef srand48

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::string;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

GusdXformWrapper::GusdXformWrapper(
        const UsdStagePtr& stage,
        const SdfPath& path,
        bool isOverride )
{
    initUsdPrim( stage, path, isOverride );
}

GusdXformWrapper::GusdXformWrapper( 
            const UsdGeomXform& usdXform,
            UsdTimeCode         time,
            GusdPurposeSet      purposes )
    : GusdGroupBaseWrapper( time, purposes )
    , m_usdXform( usdXform )
{
}

GusdXformWrapper::GusdXformWrapper( const GusdXformWrapper &in )
    : GusdGroupBaseWrapper( in )
    , m_usdXform( in.m_usdXform )
{
}

GusdXformWrapper::~GusdXformWrapper()
{}

bool GusdXformWrapper::
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
            m_usdXform = UsdGeomXformable(stage->OverridePrim( path ));
            newPrim = false;
        }
        else {
            m_usdXform = UsdGeomXform::Define( stage, path );

            // Make sure our ancestors have proper types.
            UsdPrim p = m_usdXform.GetPrim().GetParent();
            while( p && p.GetTypeName().IsEmpty() ) {
                UsdGeomXform::Define( stage, p.GetPath() );
                p = p.GetParent();
            } 
        }
    }
    else {
        m_usdXform = UsdGeomXform::Define( stage, path );
    }
    if( !m_usdXform || !m_usdXform.GetPrim().IsValid() ) {
        TF_WARN( "Unable to create %s xform '%s'.", newPrim ? "new" : "override", path.GetText() );
    }
    return bool(m_usdXform);
}

GT_PrimitiveHandle GusdXformWrapper::
defineForWrite(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt)
{
    return new GusdXformWrapper( 
                    stage, 
                    path, 
                    ctxt.writeOverlay );
}

GT_PrimitiveHandle GusdXformWrapper::
defineForRead(
        const UsdGeomImageable& sourcePrim, 
        UsdTimeCode             time,
        GusdPurposeSet          purposes )
{
    return new GusdXformWrapper( 
                    UsdGeomXform( sourcePrim.GetPrim() ),
                    time,
                    purposes );
}

bool GusdXformWrapper::
redefine( const UsdStagePtr& stage,
          const SdfPath& path,
          const GusdContext& ctxt,
          const GT_PrimitiveHandle& sourcePrim )
{
    initUsdPrim( stage, path, ctxt.writeOverlay );
    clearCaches();
    return true;
}


const char* GusdXformWrapper::
className() const
{
    return "GusdXformWrapper";
}


void GusdXformWrapper::
enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    // TODO
}


int GusdXformWrapper::
getMotionSegments() const
{
    // TODO
    return 1;
}


int64 GusdXformWrapper::
getMemoryUsage() const
{
    // TODO
    return 0;
}


GT_PrimitiveHandle GusdXformWrapper::
doSoftCopy() const
{
    return GT_PrimitiveHandle(new GusdXformWrapper( *this ));
}


bool GusdXformWrapper::
isValid() const
{
    return static_cast<bool>(m_usdXform);
}

bool GusdXformWrapper::
refine(
    GT_Refine& refiner,
    const GT_RefineParms* parms) const
{
    return refineGroup( m_usdXform.GetPrim(), refiner, parms );
}


bool GusdXformWrapper::
updateFromGTPrim(const GT_PrimitiveHandle& sourcePrim,
                 const UT_Matrix4D&        localXform,
                 const GusdContext&        ctxt,
                 GusdSimpleXformCache&     xformCache)
{
    if( !m_usdXform )
        return false;

    DBG( cout << "GusdXformWrapper::updateFromGTPrim, primType = " << sourcePrim->className() << endl );

    return updateGroupFromGTPrim( m_usdXform, sourcePrim, localXform, ctxt, xformCache );
}

PXR_NAMESPACE_CLOSE_SCOPE
