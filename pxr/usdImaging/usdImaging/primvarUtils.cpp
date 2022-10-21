//
// Copyright 2022 Pixar
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
