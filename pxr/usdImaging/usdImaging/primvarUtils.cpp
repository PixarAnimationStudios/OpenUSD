//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/primvarUtils.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TfToken
UsdImagingUsdToHdRole(TfToken const& usdRole)
{
    if (usdRole == SdfValueRoleNames->Point) {
        return HdPrimvarRoleTokens->point;
    }
    else if (usdRole == SdfValueRoleNames->Normal) {
        return HdPrimvarRoleTokens->normal;
    }
    else if (usdRole == SdfValueRoleNames->Vector) {
        return HdPrimvarRoleTokens->vector;
    }
    else if (usdRole == SdfValueRoleNames->Color) {
        return HdPrimvarRoleTokens->color;
    }
    else if (usdRole == SdfValueRoleNames->TextureCoordinate) {
        return HdPrimvarRoleTokens->textureCoordinate;
    }
    // Empty token means no role specified
    return TfToken();
}

HdInterpolation
UsdImagingUsdToHdInterpolation(TfToken const& usdInterp)
{
    if (usdInterp == UsdGeomTokens->uniform) {
        return HdInterpolationUniform;
    }
    else if (usdInterp == UsdGeomTokens->vertex) {
        return HdInterpolationVertex;
    }
    else if (usdInterp == UsdGeomTokens->varying) {
        return HdInterpolationVarying;
    }
    else if (usdInterp == UsdGeomTokens->faceVarying) {
        return HdInterpolationFaceVarying;
    }
    else if (usdInterp == UsdGeomTokens->constant) {
        return HdInterpolationConstant;
    }
    TF_CODING_ERROR(
        "Unknown USD interpolation %s; treating as constant",
        usdInterp.GetText());
    return HdInterpolationConstant;
}

TfToken 
UsdImagingUsdToHdInterpolationToken(TfToken const& usdInterp)
{
    // Technically, the more correct thing to do is:
    // return TfToken(TEnum::GetDisplayName(UsdImagingUsdToHdInterpolation(usdInterp)))
    //
    // But ultimately, the tokens are the same.  Ideally, this would be
    // something we can static_assert.
    return usdInterp;
}

PXR_NAMESPACE_CLOSE_SCOPE
