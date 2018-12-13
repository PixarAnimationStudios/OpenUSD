// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#include "GT_PrimHydra.h"
#include <GU/GU_PrimPacked.h>

#include <gusd/GT_PackedUSD.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_GEOAttributeFilter.h>

int GT_PrimHydra::mTypeId = GT_PRIM_UNDEFINED;

const char*
GT_PrimHydra::className() const { return "GT_PrimHydra"; }

GT_PrimitiveHandle
GT_PrimHydra::doSoftCopy() const
{ return new GT_PrimHydra(*this); }

void
GT_PrimHydra::enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    for (int i = 0; i < nsegments; ++i)
        boxes[i].enlargeBounds(bbox);
}

int
GT_PrimHydra::getMotionSegments() const { return 1; }

int64
GT_PrimHydra::getMemoryUsage() const { return sizeof(*this) + sizeof(prims[0]) * prims.size(); }

void
GT_PrimHydra::collect(const GEO_Primitive* prim)
{
    assert(prim->getTypeId() == pxr::GusdGU_PackedUSD::typeId());
    const GU_PrimPacked* pack = static_cast<const GU_PrimPacked*>(prim);
    const pxr::GusdGU_PackedUSD* p =
        static_cast<const pxr::GusdGU_PackedUSD*>(pack->implementation());
    if (prims.empty()) {
        p->getBounds(bbox);
        UT_Matrix4D m; pack->getFullTransform4(m);
        setPrimitiveTransform(new GT_Transform(&m, 1));
    } else {
        UT_BoundingBox box;
        p->getBounds(box);
        bbox.enlargeBounds(box);
    }
    prims.emplace_back(pack);
    pids.emplace_back(prim->getMapOffset());
}

class CollectData : public GT_GEOPrimCollectData
{
    GT_PrimHydra* pointer;
    bool _refineToUSD;
public:
    CollectData(bool r): pointer(new GT_PrimHydra), _refineToUSD(r) { }
    static GT_PrimHydra* get(GT_GEOPrimCollectData* data) {
        return data->asPointer<CollectData>()->pointer;
    }
    static bool refineToUSD(GT_GEOPrimCollectData* data) {
        return data->asPointer<CollectData>()->_refineToUSD;
    }
};

GT_GEOPrimCollectData*
GT_PrimHydraCollect::beginCollecting(
    const GT_GEODetailListHandle& geo,
    const GT_RefineParms* parms) const
{
    // This flag is set by ROP_usdoutput. Unknown why it does this while much more complex
    // operations such as USD unpack avoid calling this code. What this pretty much does is
    // make fake reference prims that are then written.
    bool refineToUSD = parms && parms->get("refineToUSD", false);
    return new CollectData(refineToUSD);
}

class FilterUnderscore : public GT_GEOAttributeFilter {
    virtual bool isValid( const GA_Attribute &attrib ) const {
        if (!GT_GEOAttributeFilter::isValid(attrib))
            return false;
        const char *n = attrib.getName().buffer();
        return n && n[0] && n[0] != '_';
    }
};

GT_PrimitiveHandle
GT_PrimHydraCollect::collect(
    const GT_GEODetailListHandle& geo,
    const GEO_Primitive* const* prims, int nsegments,
    GT_GEOPrimCollectData* data) const
{
    // example code only used first entry in prims. Not clear what the others are for
    // or what "nsegments" means

    // Replicate the refineToUSD behavior of GT_PackedUSD collector
    // This is necessary so that ROP_usdoutput works
    if (CollectData::refineToUSD(data)) {
        const GU_PrimPacked *pack = UTverify_cast<const GU_PrimPacked *>(prims[0]);
        UT_Matrix4D m;
        pack->getFullTransform4(m);
        GT_TransformHandle xform(new GT_Transform(&m, 1));

        FilterUnderscore filter;
        GT_GEOOffsetList pointOffsets, vertexOffsets, primOffsets;

        pointOffsets.append(prims[0]->getPointOffset(0));
        vertexOffsets.append(prims[0]->getVertexOffset(0));
        primOffsets.append(prims[0]->getMapOffset());
        GT_AttributeListHandle uniformAttrs
            = geo->getPrimitiveAttributes(filter, &primOffsets);

        const pxr::GusdGU_PackedUSD* impl =
            static_cast<const pxr::GusdGU_PackedUSD*>(pack->implementation());
        pxr::GusdGT_PackedUSD* gtPrim = 
            new pxr::GusdGT_PackedUSD(impl->fileName(),
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
                                      pack);
        gtPrim->setPrimitiveTransform(xform);
        return gtPrim;
    }

    GT_PrimHydra* out = CollectData::get(data);
    out->collect(prims[0]);
    return GT_PrimitiveHandle();
}

GT_PrimitiveHandle
GT_PrimHydraCollect::endCollecting(
    const GT_GEODetailListHandle& geo,
    GT_GEOPrimCollectData* data) const
{
    GT_PrimHydra* out = CollectData::get(data);
    if (out->empty()) { delete out; return GT_PrimitiveHandle(); }
    return GT_PrimitiveHandle(out);
}

// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
