// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#include "GT_PrimHydra.h"
#include "GR_PrimHydra.h"
#include <DM/DM_RenderTable.h>
#include <UT/UT_DSOVersion.h> // necessary to get plugin to load

void
newGeometryPrim(GA_PrimitiveFactory* factory)
{
    // Must initialize the USD Import first for the typeId to be allocated
    pxr::GusdGU_PackedUSD::install(*factory);
}

void
newRenderHook(DM_RenderTable *dm_table)
{
    if (const char* s = getenv("HYDRA_HOUDINI_DISABLE")) {
        if (s[0] == '2') // 2 == use RE_Geometry
            GR_PrimHydra::disable = 2;
        else if (s[0] != '0') // 0 = do not disable, any other value = disable
            return;
    }
    if (const char* s = getenv("HYDRA_HOUDINI_POSTPASS"))
        GR_PrimHydra::postpass = (s[0] != '0');
    if (GT_PrimHydra::install()) {
        // add a collector for PackedUSD prims (replaces one defined by pxr)
        (new GT_PrimHydraCollect)->bind(pxr::GusdGU_PackedUSD::typeId().get());
        // add converter to GR_PrimHydra which renders them
        dm_table->registerGTHook(
            new GR_PrimHydraHook,
            GT_PrimitiveType(GT_PrimHydra::typeId()),
            10000 // priority
        );
    }
}

// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
