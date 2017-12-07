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
#include "GT_PackedUSD.h"

#include "GU_PackedUSD.h"
#include "GU_USD.h"

#include "GT_Utils.h"
#include "UT_Gf.h"

#include <GT/GT_DAIndexedString.h>
#include <GT/GT_DANumeric.h>
#include <GT/GT_DASubArray.h>
#include <GT/GT_GEOAttributeFilter.h>
#include <GT/GT_GEODetailList.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_GEOPrimCollectBoxes.h>
#include <GT/GT_PrimCollect.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_PrimPointMesh.h>
#include <GT/GT_PrimPolygonMesh.h>
#include <GT/GT_PrimSubdivisionMesh.h>
#include <GT/GT_Refine.h>
#include <GT/GT_RefineCollect.h>
#include <GT/GT_RefineParms.h>
#include <GU/GU_PrimPacked.h>
#include <UT/UT_Options.h>

#include <pxr/usd/usdGeom/xformCache.h>

#include <unordered_map>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::vector;

//#define DEBUG

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

namespace std {

template <>
struct hash<const UT_Options>
{
    size_t operator()(const UT_Options& options) const
    {
        return static_cast<size_t>(options.hash());
    }
};

}

PXR_NAMESPACE_OPEN_SCOPE

namespace {

int s_gtPackedUsdPrimId     = GT_PRIM_UNDEFINED;
int s_gtPrimInstanceColorId = GT_PRIM_UNDEFINED;

struct _ViewportAttrFilter : public GT_GEOAttributeFilter
{
    virtual bool isValid(const GA_Attribute& attrib) const
    {
        // TODO Verify atts have correct type and dimension.
        return attrib.getName() == "__primitive_id"
            || attrib.getName() == "Cd";
    }
};

// GT_PrimInstanceWithColor is used to visualize PackedUSD prims which have a
// Cd attribute assigned. Unlike GT_PrimInstance, it will pass down Cd when it
// refines. This scheme breaks GL instancing and is potentially much slower to
// draw than GT_PrimInstance. We should have a first-class way to do this in 
// the HDK.
class GT_PrimInstanceWithColor : public GT_PrimInstance
{
public:
    GT_PrimInstanceWithColor(
            const GT_PrimitiveHandle &geometry,
            const GT_TransformArrayHandle& transforms,
            const GT_GEOOffsetList& packed_prim_offsets=GT_GEOOffsetList(),
            const GT_AttributeListHandle& uniform=GT_AttributeListHandle(),
            const GT_AttributeListHandle& detail=GT_AttributeListHandle(),
            const GT_GEODetailListHandle& source=GT_GEODetailListHandle())
        : GT_PrimInstance(
                geometry,
                transforms,
                packed_prim_offsets,
                uniform,
                detail,
                source)
        , m_uniformAttrs(uniform)
    {
    }

    GT_PrimInstanceWithColor(const GT_PrimInstanceWithColor& src)
        : GT_PrimInstance(src)
        , m_uniformAttrs( src.m_uniformAttrs )
    {
    }

    virtual ~GT_PrimInstanceWithColor()
    {}

    virtual int getPrimitiveType() const
    {
        if( s_gtPrimInstanceColorId == GT_PRIM_UNDEFINED )
            s_gtPrimInstanceColorId = GT_Primitive::createPrimitiveTypeId(); 
        return  s_gtPrimInstanceColorId;
    }

    virtual const char* className() const
    {
        return "GT_PrimInstanceWithColor";
    }


    inline GT_AttributeListHandle 
    appendAttrs( GT_AttributeListHandle dest, int i ) const
    {
        if( dest && m_uniformAttrs ) {
            if( auto colorArray = m_uniformAttrs->get( "Cd", 0 )) {
                dest = dest->addAttribute( 
                    "Cd", 
                    new GT_DASubArray( colorArray, i, 1), true );
            }
            if( auto idArray = m_uniformAttrs->get( "__primitive_id", 0 )) {
                dest = dest->addAttribute( 
                    "__primitive_id",
                    new GT_DASubArray( idArray, i, 1), true );
            }
        }
        return dest;
    }

    virtual bool refine(GT_Refine& refiner, const GT_RefineParms* parms=NULL) const
    {
        GT_RefineCollect refineCollect;
        GT_PrimInstance::refine(refineCollect, parms);
        for(int i=0; i<refineCollect.entries(); ++i) {
            GT_PrimitiveHandle curPrim = refineCollect.getPrim(i);
            if(curPrim->getPrimitiveType() == GT_PRIM_INSTANCE) {
                auto instPrim = UTverify_cast<const GT_PrimInstance*>(curPrim.get());
                curPrim.reset(new GT_PrimInstanceWithColor(
                            instPrim->geometry(),
                            instPrim->transforms(),
                            instPrim->packedPrimOffsets(),
                            m_uniformAttrs)); 
            }
            else if(curPrim->getPrimitiveType() == GT_PRIM_SUBDIVISION_MESH) {
                auto meshPrim = UTverify_cast<const GT_PrimSubdivisionMesh*>(curPrim.get());
                curPrim.reset(new GT_PrimSubdivisionMesh(
                            *meshPrim,
                            meshPrim->getShared(),
                            meshPrim->getVertex(),
                            meshPrim->getUniform(),
                            appendAttrs( meshPrim->getDetail(), i )));
            }
            else if(curPrim->getPrimitiveType() == GT_PRIM_POLYGON_MESH) {
                auto meshPrim = UTverify_cast<const GT_PrimPolygonMesh*>(curPrim.get());
                curPrim.reset(new GT_PrimPolygonMesh(
                            *meshPrim,
                            meshPrim->getShared(),
                            meshPrim->getVertex(),
                            meshPrim->getUniform(),
                            appendAttrs( meshPrim->getDetail(), i )));
            }

            refiner.addPrimitive(curPrim);
        }
        return true;
    }

private:
    GT_AttributeListHandle m_uniformAttrs;
};

// -----------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////


class CollectData : public GT_GEOPrimCollectData
{
public:
    CollectData(
        const GT_GEODetailListHandle &geometry,
        bool useViewportLOD,
        bool refineToUSD)
          : GT_GEOPrimCollectData(),
            _geometry(geometry),
            _useViewportLOD(useViewportLOD),
            _refineToUSD(refineToUSD)
    {
    }

    virtual ~CollectData()
    {
    }

    bool    append(const GU_PrimPacked &prim)
    {
        const GusdGU_PackedUSD *impl = 
            UTverify_cast<const GusdGU_PackedUSD *>(prim.implementation());

        if (!impl->visibleGT())
            return true; 

        if (!_refineToUSD)
        {
            if( _useViewportLOD )
            {
                switch (prim.viewportLOD())
                {
                case GEO_VIEWPORT_HIDDEN:
                    return true;    // Handled
                case GEO_VIEWPORT_CENTROID:
                    _centroidPrims.append(&prim);
                    return true;
                case GEO_VIEWPORT_BOX:
                    _boxPrims.append(&prim);
                    return true;
                case GEO_VIEWPORT_FULL:
                    _geoPrims.append(&prim);
                    return true;
                default:
                    // Fall through to generic handling
                    break;
                }
            }
            else 
            {
                _geoPrims.append(&prim);
                return true;
            }
        }
        return false;
    }

    class FillTask
    {
    public:
        FillTask(UT_BoundingBox *boxes,
            UT_Matrix4F *xforms,
            const UT_Array<const GU_PrimPacked *>&prims)
            : myBoxes(boxes)
            , myXforms(xforms)
            , myPrims(prims)
        {
        }
        void    operator()(const UT_BlockedRange<exint> &range) const
        {
            UT_Matrix4D     m4d;
            for (exint i = range.begin(); i != range.end(); ++i)
            {
                const GU_PrimPacked &prim = *myPrims(i);
                prim.getUntransformedBounds(myBoxes[i]);
                prim.getFullTransform4(m4d);
                myXforms[i] = m4d; 
            }
        }
    private:
        UT_BoundingBox  *myBoxes;
        UT_Matrix4F     *myXforms;
        const UT_Array<const GU_PrimPacked *>   &myPrims;
    };

    void
    addInstances( 
        GT_PrimCollect& collection, 
        GT_PrimitiveHandle geo, 
        const UT_Array<const GU_PrimPacked*>& instances ) const
    {
        GT_GEOOffsetList primOffsetList;
        GT_GEOOffsetList vtxOffsetList;
        GT_TransformArrayHandle xformArray(new GT_TransformArray());

        // Work around a SideFx Bug. If the geo has a non-identity transform,
        // Houdini will draw the instance prim ok but does weird frustum culling.
        UT_Matrix4D geoMat( 1 );
        auto geoTransform = geo->getPrimitiveTransform();
        geoTransform->getMatrix( geoMat );
        if( !geoMat.isIdentity() )
        {
            geo = geo->clone();
            geo->setPrimitiveTransform( GT_Transform::identity() );
        }

        // get the offsets and transforms for each prim
        for(auto const &packedPrim : instances ) {

            primOffsetList.append(packedPrim->getMapOffset());
            vtxOffsetList.append(packedPrim->getVertexOffset(0));

            UT_Matrix4D instXform;
            packedPrim->getFullTransform4(instXform);
            UT_Matrix4D m = geoMat * instXform;
            xformArray->append( new GT_Transform( &m, 1 ));
        }

        // Copy __primitive_id attribute to support viewport picking
        _ViewportAttrFilter filter;
        GT_AttributeListHandle uniformAttrs 
            = _geometry->getPrimitiveVertexAttributes(
                    filter, primOffsetList, vtxOffsetList);

        GT_PrimInstance* gtInst;
        if( uniformAttrs->hasName( "Cd" )) {
            gtInst = new GT_PrimInstanceWithColor( 
                                geo, 
                                xformArray, 
                                primOffsetList, 
                                uniformAttrs );
        }
        else {
            gtInst = new GT_PrimInstance( 
                                geo, 
                                xformArray, 
                                primOffsetList, 
                                uniformAttrs );
        }
        collection.appendPrimitive( gtInst );
    }

    GT_PrimitiveHandle  finish() const
    {
        DBG( cerr << "CollectData::finish" << endl );

        exint           ngeo = _geoPrims.entries();
        exint           nbox = _boxPrims.entries();
        exint           ncentroid = _centroidPrims.entries();
        exint           nproxies = SYSmax(nbox, ncentroid);

        if (!ngeo && !nproxies)
            return GT_PrimitiveHandle();

        GT_GEOPrimCollectBoxes          boxdata(_geometry, true);
        UT_StackBuffer<UT_BoundingBox>  boxes(nproxies);
        UT_StackBuffer<UT_Matrix4F>     xforms(nproxies);

        GT_PrimCollect* rv = new GT_PrimCollect();

        if (nbox)
        {
            //UTparallelFor(UT_BlockedRange<exint>(0, nbox),
            UTserialFor(UT_BlockedRange<exint>(0, nbox),
                FillTask(boxes, xforms, _boxPrims));
            for (exint i = 0; i < nbox; ++i)
            {
                boxdata.appendBox(boxes[i], xforms[i],
                    _boxPrims(i)->getMapOffset(),
                    _boxPrims(i)->getVertexOffset(0),
                    _boxPrims(i)->getPointOffset(0));
            }
            rv->appendPrimitive(boxdata.getPrimitive());

        }
        if (ncentroid)
        {
            //UTparallelFor(UT_BlockedRange<exint>(0, ncentroid),
            UTserialFor(UT_BlockedRange<exint>(0, ncentroid),
                FillTask(boxes, xforms, _centroidPrims));
            for (exint i = 0; i < ncentroid; ++i)
            {
                boxdata.appendCentroid(boxes[i], xforms[i],
                    _centroidPrims(i)->getMapOffset(),
                    _centroidPrims(i)->getVertexOffset(0),
                    _centroidPrims(i)->getPointOffset(0));
            }
            rv->appendPrimitive(boxdata.getPrimitive());
        }

        if( ngeo ) {

            // sort packed prims into collections of identical instances
            std::unordered_map<const UT_Options, UT_Array<const GU_PrimPacked*> >
                                    instanceMap;

            for( auto const & prim : _geoPrims ) {

                auto  impl = 
                    UTverify_cast<const GusdGU_PackedUSD*>(prim->implementation());

                UT_Options instanceKey;                
                impl->getInstanceKey(instanceKey);
                auto instanceMapIt = instanceMap.find(instanceKey);
                if (instanceMapIt == instanceMap.end())
                {
                    UT_Array<const GU_PrimPacked*> prims;
                    prims.append(prim);
                    instanceMap[instanceKey] = prims;
                }
                else
                {
                    instanceMapIt->second.append(prim);                    
                }
            }

            // Iterate over groups of instances
            for( auto const &kv : instanceMap ) {

                auto const & instancePrims = kv.second;        
                
                auto  impl = 
                    UTverify_cast<const GusdGU_PackedUSD*>(instancePrims(0)->implementation());

                // Use the first prim for geometry
                GT_PrimitiveHandle geo = impl->fullGT();

                if( !geo )
                    continue;

                if( geo->getPrimitiveType() == GT_PRIM_COLLECT ) {
                    auto collect = UTverify_cast<const GT_PrimCollect*>( geo.get() );

                    for( int i = 0; i < collect->entries(); ++i ) {
                        addInstances( *rv, collect->getPrim(i), kv.second );
                    }
		        }
                else {
                    addInstances( *rv, geo, kv.second );
                }
            }
        }
        return rv;
    }

    bool refineToUSD() { return _refineToUSD; }

private:
    const GT_GEODetailListHandle    _geometry;
    UT_Array<const GU_PrimPacked *> _boxPrims;
    UT_Array<const GU_PrimPacked *> _centroidPrims;
    UT_Array<const GU_PrimPacked *> _geoPrims;
    bool                            _useViewportLOD;
    bool                            _refineToUSD;
};

} // close anonymous namespace

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// class GusdGT_PackedUSD implementation
////////////////////////////////////////////////////////////////////////////////

GusdGT_PackedUSD::
GusdGT_PackedUSD(
        const UT_StringHolder& fileName,
        const UT_StringHolder& auxFileName,
        const SdfPath& primPath,
        const SdfPath& srcPrimPath,
        exint instanceIndex,
        UsdTimeCode frame,
        GusdPurposeSet,
        GT_AttributeListHandle pointAttributes,
        GT_AttributeListHandle vertexAttributes,
        GT_AttributeListHandle uniformAttributes,
        GT_AttributeListHandle detailAttributes,
        const GU_PrimPacked* prim)
    : m_fileName(fileName)
    , m_auxFileName(auxFileName)
    , m_primPath(primPath)
    , m_srcPrimPath(srcPrimPath)
    , m_instanceIndex(instanceIndex)
    , m_frame(frame)
    , m_pointAttributes(pointAttributes)
    , m_vertexAttributes(vertexAttributes)
    , m_uniformAttributes(uniformAttributes)
    , m_detailAttributes(detailAttributes)
{
    UTverify_cast<const GusdGU_PackedUSD*>(prim->implementation())->getBounds( m_box );
}

GusdGT_PackedUSD::
GusdGT_PackedUSD(const GusdGT_PackedUSD& other)
    : GT_Primitive( other )
    , m_fileName(other.m_fileName)
    , m_auxFileName(other.m_auxFileName)
    , m_primPath(other.m_primPath)
    , m_srcPrimPath(other.m_srcPrimPath)
    , m_instanceIndex(other.m_instanceIndex)
    , m_frame(other.m_frame)
    , m_pointAttributes(other.m_pointAttributes)
    , m_vertexAttributes(other.m_vertexAttributes)
    , m_uniformAttributes(other.m_uniformAttributes)
    , m_detailAttributes(other.m_detailAttributes)
    , m_box(other.m_box)
{
}


GusdGT_PackedUSD::
~GusdGT_PackedUSD()
{}


const char* GusdGT_PackedUSD::
className() const
{ return "GusdGT_PackedUSD"; }


GT_PrimitiveHandle GusdGT_PackedUSD::
doSoftCopy() const
{ return new GusdGT_PackedUSD(*this); }


bool GusdGT_PackedUSD::
getUniqueID(int64& id) const
{
    id = getPrimitiveType();
    return true;
}

/* static */
int GusdGT_PackedUSD::
getStaticPrimitiveType() 
{
    if( s_gtPackedUsdPrimId == GT_PRIM_UNDEFINED )
        s_gtPackedUsdPrimId = GT_Primitive::createPrimitiveTypeId(); 
    return  s_gtPackedUsdPrimId;
}
    

int GusdGT_PackedUSD::
getPrimitiveType() const
{
    return getStaticPrimitiveType();
}

void GusdGT_PackedUSD::
enlargeBounds(UT_BoundingBox boxes[],
              int nsegments) const
{
    for( size_t i = 0; i < nsegments; ++i )
        boxes[i].enlargeBounds( m_box );
}

int GusdGT_PackedUSD::
getMotionSegments() const
{
    return 1;
}

int64 GusdGT_PackedUSD::
getMemoryUsage() const
{
    return sizeof(*this);
}

const GT_AttributeListHandle& GusdGT_PackedUSD::
getPointAttributes() const
{
    return m_pointAttributes;
}

const GT_AttributeListHandle& GusdGT_PackedUSD::
getVertexAttributes() const
{
    return m_vertexAttributes;
}

const GT_AttributeListHandle& GusdGT_PackedUSD::
getUniformAttributes() const
{
    return m_uniformAttributes;
}

const GT_AttributeListHandle& GusdGT_PackedUSD::
getDetailAttributes() const
{
    return m_detailAttributes;
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// class GusdGT_PrimCollect implementation
////////////////////////////////////////////////////////////////////////////////

GusdGT_PrimCollect::~GusdGT_PrimCollect()
{
}

GT_GEOPrimCollectData *
GusdGT_PrimCollect::beginCollecting(
    const GT_GEODetailListHandle&   geometry,
    const GT_RefineParms*           parms) const
{
    return new CollectData( 
        geometry,
        GT_GEOPrimPacked::useViewportLOD(parms),
        parms ? parms->get( "refineToUSD", false) : false );
}

class FilterUnderscore : public GT_GEOAttributeFilter {

    virtual bool isValid( const GA_Attribute &attrib ) const {
        if( !GT_GEOAttributeFilter::isValid(attrib) )
            return false;
        const char *n = attrib.getName().buffer();
        if( n == NULL || (strlen( n ) >= 1 && n[0] == '_' )) {
            return false;
        }
        return true;
    }
};

GT_PrimitiveHandle
GusdGT_PrimCollect::collect(
    const GT_GEODetailListHandle &geo,
    const GEO_Primitive *const* prim_list,
    int nsegments,
    GT_GEOPrimCollectData *data) const
{
    CollectData *collector = data->asPointer<CollectData>();
    const GU_PrimPacked *pack = 
        UTverify_cast<const GU_PrimPacked *>(prim_list[0]);

    // If the prim is something simple that can be drawn in bulk (bbox or centroid),
    // append to a list. 
    if (!collector->append(*pack))
    {
        const GusdGU_PackedUSD *impl =
           UTverify_cast<const GusdGU_PackedUSD *>(pack->implementation());

        // if writing to a usd file we return a GusdGT_PackedUSD which can be interpreted
        // as a usd reference
        if(collector->refineToUSD())
        {
            UT_Matrix4D m;
            pack->getFullTransform4(m);
            GT_TransformHandle xform(new GT_Transform(&m, 1));

            FilterUnderscore filter;
            GT_GEOOffsetList pointOffsets, vertexOffsets, primOffsets;

            pointOffsets.append( prim_list[0]->getPointOffset(0) );
            vertexOffsets.append( prim_list[0]->getVertexOffset(0) );
            primOffsets.append( prim_list[0]->getMapOffset() );
            GT_AttributeListHandle uniformAttrs
                = geo->getPrimitiveAttributes(filter, &primOffsets);

            GusdGT_PackedUSD* gtPrim = 
                new GusdGT_PackedUSD( impl->fileName(),
                                      impl->altFileName(),
                                      impl->primPath(),
                                      impl->srcPrimPath(),
                                      impl->index(),
                                      impl->frame(),
                                      impl->getPurposes(),
                                      geo->getPointAttributes( filter, &pointOffsets ),
                                      geo->getVertexAttributes( filter, &vertexOffsets ),
                                      geo->getPrimitiveAttributes( filter, &primOffsets ),
                                      geo->getDetailAttributes( filter ),
                                      pack );
            gtPrim->setPrimitiveTransform(xform);
            
            return gtPrim;
        }
    }
    return GT_PrimitiveHandle();
}

GT_PrimitiveHandle
GusdGT_PrimCollect::endCollecting(
    const GT_GEODetailListHandle &geometry,
    GT_GEOPrimCollectData *data) const
{
    CollectData *collector = data->asPointer<CollectData>();
    GT_PrimitiveHandle rv = collector->finish();
#ifdef DEBUG
    cerr << "endCollecting, rv type = " << rv->className() << endl;
    if( rv->getPrimitiveType() == GT_PRIM_COLLECT ) {
        const GT_PrimCollect* collect = UTverify_cast<const GT_PrimCollect*>(rv.get());
        cerr << "collection entries = " << collect->entries() << endl;
        for( exint i = 0; i < collect->entries(); ++i ) {
            GT_PrimitiveHandle p = collect->getPrim(i);

            cerr << "entry type = " << p->className() << endl;
            if( p->getPrimitiveType() == GT_PRIM_INSTANCE ) {
                const GT_PrimInstance* inst1 = UTverify_cast<const GT_PrimInstance*>(p.get());

                GT_PrimitiveHandle geo1 = inst1->geometry();
                cerr << "inst geo1 type = " << geo1->className() << endl;

                if( geo1->getPrimitiveType() == GT_PRIM_INSTANCE ) {
                    const GT_PrimInstance* inst2 = UTverify_cast<const GT_PrimInstance*>(geo1.get());

                    GT_PrimitiveHandle geo2 = inst2->geometry();
                    cerr << "inst geo2 type = " << geo2->className() << endl;

                }
            }
        }
    }
#endif
    return rv;
}

PXR_NAMESPACE_CLOSE_SCOPE
