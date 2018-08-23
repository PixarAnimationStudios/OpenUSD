// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#pragma once

#include <pxr/usd/usd/prim.h>

#include <GR/GR_Primitive.h> // includes stuff missing from RE_Geometry.h
#include <RE/RE_Geometry.h>

bool
USDtoRE(const pxr::UsdPrim& prim,
        double frame, const pxr::GfMatrix4d& xform, const pxr::TfTokenVector& purposes,
        RE_Render* r, std::unique_ptr<RE_Geometry>& geo,
        unsigned* numPrims = nullptr, bool getAll = true);

// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

