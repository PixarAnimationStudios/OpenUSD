//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/homogeneous.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

PXR_NAMESPACE_OPEN_SCOPE

GfVec4f GfGetHomogenized(const GfVec4f &v)
{
    GfVec4f ret(v);

    if(ret[3] == 0) ret[3] = 1;
    ret /= ret[3];
    return ret;
}

GfVec4f GfHomogeneousCross(const GfVec4f &a, const GfVec4f &b)
{
    GfVec4f ah(GfGetHomogenized(a));
    GfVec4f bh(GfGetHomogenized(b));
    
    GfVec3f prod =
        GfCross(GfVec3f(ah[0], ah[1], ah[2]), GfVec3f(bh[0], bh[1], bh[2]));
    
    return GfVec4f(prod[0], prod[1], prod[2], 1);
}

GfVec4d GfGetHomogenized(const GfVec4d &v)
{
    GfVec4d ret(v);

    if(ret[3] == 0) ret[3] = 1;
    ret /= ret[3];
    return ret;
}

GfVec4d GfHomogeneousCross(const GfVec4d &a, const GfVec4d &b)
{
    GfVec4d ah(GfGetHomogenized(a));
    GfVec4d bh(GfGetHomogenized(b));
    
    GfVec3d prod =
        GfCross(GfVec3d(ah[0], ah[1], ah[2]), GfVec3d(bh[0], bh[1], bh[2]));
    
    return GfVec4d(prod[0], prod[1], prod[2], 1);
}

PXR_NAMESPACE_CLOSE_SCOPE
