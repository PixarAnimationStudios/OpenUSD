// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#include "GT_PrimHydra.h"
#include <GU/GU_PrimPacked.h>

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
public:
    CollectData(): pointer(new GT_PrimHydra) { }
    static GT_PrimHydra* get(GT_GEOPrimCollectData* data) {
        return data->asPointer<CollectData>()->pointer;
    }
};

GT_GEOPrimCollectData*
GT_PrimHydraCollect::beginCollecting(
    const GT_GEODetailListHandle& geometry,
    const GT_RefineParms* parms) const
{
    return new CollectData;
}

GT_PrimitiveHandle
GT_PrimHydraCollect::collect(
    const GT_GEODetailListHandle& geometry,
    const GEO_Primitive* const* prims, int nsegments,
    GT_GEOPrimCollectData* data) const
{
    // example code only used first entry in prims. Not clear what the others are for
    // or what "nsegments" means
    GT_PrimHydra* out = CollectData::get(data);
    out->collect(prims[0]);
    return GT_PrimitiveHandle();
}

GT_PrimitiveHandle
GT_PrimHydraCollect::endCollecting(
    const GT_GEODetailListHandle& geometry,
    GT_GEOPrimCollectData* data) const
{
    GT_PrimHydra* out = CollectData::get(data);
    if (out->empty()) { delete out; return GT_PrimitiveHandle(); }
    return GT_PrimitiveHandle(out);
}

// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
