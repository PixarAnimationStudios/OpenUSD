//
// Copyright 2024 Pixar
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
