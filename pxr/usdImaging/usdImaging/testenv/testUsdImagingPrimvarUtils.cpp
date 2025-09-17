//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/primvarUtils.h"

#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
_TestUsdToHdInterpolation()
{
    // Verifies that our implementation of UsdImagingUsdToHdInterpolationToken
    // is correct.  It currently assumes that the usd interpolation tokens match
    // the HdInterpolation (display) names.
    TF_VERIFY(
        TfToken(TfEnum::GetDisplayName(
            UsdImagingUsdToHdInterpolation(UsdGeomTokens->uniform)))
        == UsdImagingUsdToHdInterpolationToken(UsdGeomTokens->uniform));
    TF_VERIFY(
        TfToken(TfEnum::GetDisplayName(
            UsdImagingUsdToHdInterpolation(UsdGeomTokens->vertex)))
        == UsdImagingUsdToHdInterpolationToken(UsdGeomTokens->vertex));
    TF_VERIFY(
        TfToken(TfEnum::GetDisplayName(
            UsdImagingUsdToHdInterpolation(UsdGeomTokens->varying)))
        == UsdImagingUsdToHdInterpolationToken(UsdGeomTokens->varying));
    TF_VERIFY(
        TfToken(TfEnum::GetDisplayName(
            UsdImagingUsdToHdInterpolation(UsdGeomTokens->faceVarying)))
        == UsdImagingUsdToHdInterpolationToken(UsdGeomTokens->faceVarying));
    TF_VERIFY(
        TfToken(TfEnum::GetDisplayName(
            UsdImagingUsdToHdInterpolation(UsdGeomTokens->constant)))
        == UsdImagingUsdToHdInterpolationToken(UsdGeomTokens->constant));
}

int
main()
{
    _TestUsdToHdInterpolation();
    return 0;
}
