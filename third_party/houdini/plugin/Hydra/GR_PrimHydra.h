// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#pragma once

#include <GR/GR_Primitive.h>
#include <GU/GU_PrimPacked.h>
#include <GUI/GUI_PrimitiveHook.h>
#include <RE/RE_Geometry.h>
#include <RE/RE_ShaderHandle.h>

#include "GT_PrimHydra.h"

/// GR_PrimHydra draws a GT_PrimHydra, which is a vector of GU_PackedUSD prims. It
/// uses the Hydra renderer as much as possible, falling back to a RE_Geometry
/// simulation of the usd data when necessary.

class GR_PrimHydraHook : public GUI_PrimitiveHook
{
public:
    GR_PrimHydraHook(): GUI_PrimitiveHook("Hydra") {}
    ~GR_PrimHydraHook() override {}
    GR_Primitive* createPrimitive(const GT_PrimitiveHandle& gt_prim,
                                  const GEO_Primitive* geo_prim,
                                  const GR_RenderInfo* info,
                                  const char* cache_name,
                                  GR_PrimAcceptResult& processed) override;
};

class GR_PrimHydra : public GR_Primitive
{
public:
    GR_PrimHydra(const GR_RenderInfo* info, const char* cache_name):
        GR_Primitive(info, cache_name, GEO_PrimTypeCompat::GEOPRIMPOLYSOUP) {}
    ~GR_PrimHydra() override;
    void cleanup(RE_Render*) override;

    const char* className() const override { return "GR_PrimHydra"; }

    GR_PrimAcceptResult acceptPrimitive(GT_PrimitiveType t,
                                        int geo_type,
                                        const GT_PrimitiveHandle& ph,
                                        const GEO_Primitive* prim) override;

    void update(RE_Render*, const GT_PrimitiveHandle&, const GR_UpdateParms&) override;
    void render(RE_Render*, GR_RenderMode, GR_RenderFlags, GR_DrawParms) override;
    int renderPick(RE_Render*, const GR_DisplayOption*,
                   unsigned int pick_type, GR_PickStyle pick_style, bool has_pick_map) override;

    // The GT_PrimHydra we are drawing
    const GT_PrimHydra* GT() const { return static_cast<const GT_PrimHydra*>(ph.get()); }

    // Extract data from the prims
    size_t size() const { return GT()->prims.size(); }
    const GU_PrimPacked* getPrimPacked(size_t i) const { return GT()->prims[i]; }
    const pxr::GusdGU_PackedUSD* getPackedUSD(size_t i) const
    { return static_cast<const pxr::GusdGU_PackedUSD*>(getPrimPacked(i)->implementation()); }
    pxr::UsdPrim getUsdPrim(size_t i) const { return getPackedUSD(i)->getUsdPrim(); }
    double getFrame(size_t i) const { return getPackedUSD(i)->intrinsicFrame(); }
    pxr::GusdPurposeSet getPurposes(size_t i) const { return getPackedUSD(i)->getPurposes(); }
    GEO_ViewportLOD getViewportLOD(size_t i) const { return getPrimPacked(i)->viewportLOD(); }

    enum class DrawType {HIDDEN, BOX, CENTROID, RE, HYDRA};
    static int disable; // set to HYDRA_HOUDINI_DISABLE value
    static bool postpass; // true if HYDRA_HOUDINI_POSTPASS is not zero
    struct Hydra; // pass info from render() to Hydra drawing functions
private:
    GT_PrimitiveHandle ph;
    UT_BitArray selected; // what parts are selected
    UT_BitArray hasXform; // what parts need extra xform to draw
    std::vector<DrawType> drawType;
    std::vector<size_t> instanceOf; // index+1 of first prim of a set of instances
    std::vector<UT_Matrix4D> xforms;
    bool updateGeo = true; // myGeo needs to be recreated
    bool updateSelection = true; // selection attribute needs to be updated
    bool hasRE = false; // true if any drawType == RE
    unsigned badPrims = 0; // number of null prims (these should not happen)
    bool initRender();
    struct RE_Geo {
        std::unique_ptr<RE_Geometry> geo;
        unsigned prims; // number of prims in geo
        unsigned instances; // if non-zero use instance drawing
        bool good; // true if it looks acceptably close to Hydra version
        bool update; // dirty flag
    };
    std::vector<RE_Geo> myGeo;
    const RE_Geo& buildGeo(RE_Render* r, size_t i)
    { return (updateGeo || myGeo[i].update) ? _buildGeo(r,i) : myGeo[i]; }
    const RE_Geo& _buildGeo(RE_Render*, size_t);
    RE_Geo boxes;
    void buildBoxes(RE_Render* r);
    static void setupLighting(RE_Render* r, GR_DrawParms& dp);
    static void runHydra(RE_Render* r, Hydra& h, GR_DrawParms& dp);
};

// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
