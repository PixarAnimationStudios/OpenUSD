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
#include "groupBaseWrapper.h"

#include "context.h"
#include "UT_Gf.h"
#include "GU_PackedUSD.h"
#include "USD_XformCache.h"
#include "GU_USD.h"

#include "pxr/usd/usdGeom/boundable.h"

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
using std::string;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

GusdGroupBaseWrapper::GusdGroupBaseWrapper() 
    : GusdPrimWrapper()
{
}

GusdGroupBaseWrapper::GusdGroupBaseWrapper( 
        UsdTimeCode     time, 
        GusdPurposeSet  purposes  ) 
    : GusdPrimWrapper( time, purposes )
{
}

GusdGroupBaseWrapper::GusdGroupBaseWrapper( const GusdGroupBaseWrapper &in )
    : GusdPrimWrapper( in )
{
}

GusdGroupBaseWrapper::~GusdGroupBaseWrapper()
{}

namespace {
bool
containsBoundable( const UsdPrim& p, GusdPurposeSet purposes )
{
    // Return true if this prim has a boundable geom descendant.
    // Boundables are gprims and the point instancers.
    // Used when unpacking so we don't create empty GU prims.

    UsdGeomImageable ip( p );
    if(!ip)
        return false;

    TfToken purpose;
    ip.GetPurposeAttr().Get(&purpose);
    if( !GusdPurposeInSet( purpose, purposes ) && !p.IsMaster() )
        return false;
    
    if( p.IsA<UsdGeomBoundable>() )
        return true;

    for( const auto& child : p.GetFilteredChildren(
                        UsdTraverseInstanceProxies(UsdPrimDefaultPredicate)) )
    {
        if( containsBoundable( child, purposes ))
            return true;
    }
    return false;
}
}

bool
GusdGroupBaseWrapper::unpack( 
    GU_Detail&          gdr,
    const UT_StringRef& fileName,
    const SdfPath&      primPath,
    const UT_Matrix4D&  xform,
    fpreal              frame,
    const char*         viewportLod,
    GusdPurposeSet      purposes )
{
    UsdPrim usdPrim = getUsdPrim().GetPrim();

    // To unpack a xform or a group, create a packed prim for
    // each child
    UT_Array<UsdPrim> usefulChildren;
    for( const auto& child : usdPrim.GetFilteredChildren(
                        UsdTraverseInstanceProxies(UsdPrimDefaultPredicate)) )
    {
        if( containsBoundable( child, purposes ))
            usefulChildren.append( child );
    }

    // Sort the children to maintain consistency in unpacking.
    GusdUSD_Utils::SortPrims(usefulChildren);

    SdfPath strippedPathHead(primPath.StripAllVariantSelections());
    for( const auto &child : usefulChildren )
    {
        // Replace the head of the path to perserve variant specs.
        SdfPath path = child.GetPath().ReplacePrefix(strippedPathHead,
                                                      primPath );

        GU_PrimPacked *guPrim = 
            GusdGU_PackedUSD::Build( gdr, fileName, path, 
                                     frame, viewportLod, purposes );

        UT_Matrix4D m;
        GusdUSD_XformCache::GetInstance().GetLocalTransformation( 
                child, frame, m );      

        UT_Matrix4D m1 = m * xform;
        UT_Vector3 p;
        m1.getTranslates( p );

        guPrim->setLocalTransform(UT_Matrix3( m1 ));
        guPrim->setPos3(0, p);
    }
    return true;
}

bool
GusdGroupBaseWrapper::refineGroup( 
    const UsdPrim& prim,
    GT_Refine& refiner,
    const GT_RefineParms* parms ) const
{
    UsdPrimSiblingRange children =  prim.GetFilteredChildren(
                        UsdTraverseInstanceProxies(UsdPrimDefaultPredicate));

    GT_PrimCollect* collection = NULL;
    for( const UsdPrim& child : children )
    {
        GT_PrimitiveHandle gtPrim = 
            GusdPrimWrapper::defineForRead( 
                    UsdGeomImageable(child), 
                    m_time,
                    m_purposes );

        if( gtPrim )
        {
            UT_Matrix4D m;
            GusdUSD_XformCache::GetInstance().GetLocalTransformation( 
                    child, m_time, m );
            gtPrim->setPrimitiveTransform( new GT_Transform( &m, 1 ) );

            if( !collection ) {
                collection = new GT_PrimCollect();
            }
            collection->appendPrimitive( gtPrim );        
        }
    }
    if( collection ) {
        refiner.addPrimitive( collection );
        return true;
    }
    return false;
}

bool 
GusdGroupBaseWrapper::updateGroupFromGTPrim(
    const UsdGeomImageable&   destPrim,
    const GT_PrimitiveHandle& sourcePrim,
    const UT_Matrix4D&        houXform,
    const GusdContext&        ctxt,
    GusdSimpleXformCache&     xformCache )
{
    if( !destPrim )
        return false;

    if( !ctxt.writeOverlay && ctxt.purpose != UsdGeomTokens->default_ ) {
        destPrim.GetPurposeAttr().Set( ctxt.purpose );
    }

    if( !ctxt.writeOverlay || ctxt.overlayTransforms || ctxt.overlayAll )
    {
        GfMatrix4d xform = computeTransform( 
                        destPrim.GetPrim().GetParent(),
                        ctxt.time,
                        houXform,
                        xformCache );


        updateTransformFromGTPrim( xform, ctxt.time, 
                                   ctxt.granularity == GusdContext::PER_FRAME );

        // cache this transform so that if we write a child, we can compute its
        // relative transform.
        xformCache[destPrim.GetPrim().GetPath()] = houXform;
    }

    // sourcePrim can be NULL if the ROP wants to write a transform without having
    // a corresponding GT_Primitive
    if( !sourcePrim ) {
        return true;
    }

    if( !ctxt.writeOverlay || ctxt.overlayPrimvars || ctxt.overlayAll )
    {
        GusdGT_AttrFilter filter = ctxt.attributeFilter;
        filter.appendPattern(GT_OWNER_UNIFORM, "^P");
        if( const GT_AttributeListHandle uniformAttrs = sourcePrim->getUniformAttributes())
        {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_UNIFORM;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( uniformAttrs, filter, UsdGeomTokens->uniform, ctxt.time );
        }
    }

    // Set active state
    updateGroupActiveFromGTPrim(destPrim, sourcePrim, ctxt.time);

    return true;
}

void
GusdGroupBaseWrapper::updateGroupActiveFromGTPrim(
        const UsdGeomImageable& destPrim,
        const GT_PrimitiveHandle& sourcePrim,
        UsdTimeCode time)
{
    UsdPrim prim = destPrim.GetPrim();

    GT_Owner attrOwner;
    GT_DataArrayHandle houAttr
        = sourcePrim->findAttribute(GUSD_ACTIVE_ATTR, attrOwner, 0);
    if (houAttr) {
        GT_String state = houAttr->getS(0);
        if (state) {
            if (strcmp(state, "active") == 0) {
                prim.SetActive(true);
            } else if (strcmp(state, "inactive") == 0) {
                prim.SetActive(false);
            }
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
