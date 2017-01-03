//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdRi/rmanUtilities.h"

#include "pxr/usd/usdGeom/tokens.h"

int
UsdRiConvertToRManInterpolateBoundary(TfToken const& token)
{
    if(token == UsdGeomTokens->none)
        return 0;
    else if(token == UsdGeomTokens->edgeAndCorner)
        return 1;
    else if(token == UsdGeomTokens->edgeOnly)
        return 2;
    else{
        TF_CODING_ERROR("Invalid InterpolateBoundary Token: %s",
            token.GetText());
        return 0;
    }
}

TfToken const&
UsdRiConvertFromRManInterpolateBoundary(int i)
{
    switch(i){
    case 0:
        return UsdGeomTokens->none;
    case 1:
        return UsdGeomTokens->edgeAndCorner;
    case 2:
        return UsdGeomTokens->edgeOnly;
    default:
        TF_CODING_ERROR("Invalid InterpolateBoundary int: %d", i);
        return UsdGeomTokens->none;
    }
}

int
UsdRiConvertToRManFaceVaryingLinearInterpolation(TfToken const& token)
{ 
    if(token == UsdGeomTokens->bilinear 
       || token == UsdGeomTokens->all) {
        return 0;
    }
    else if(token == UsdGeomTokens->edgeAndCorner 
            || token == UsdGeomTokens->cornersOnly
            || token == UsdGeomTokens->cornersPlus1
            || token == UsdGeomTokens->cornersPlus2) {
        return 1;
    }
    else if(token == UsdGeomTokens->edgeOnly
            || token == UsdGeomTokens->none) {
        return 2;
    }
    else if(token == UsdGeomTokens->alwaysSharp
            || token == UsdGeomTokens->boundaries) {
        return 3;
    }

    else{
        TF_CODING_ERROR("Invalid FaceVaryingLinearInterpolation Token: %s",
            token.GetText());
        return 1;
    }
}

TfToken const&
UsdRiConvertFromRManFaceVaryingLinearInterpolation(int i)
{
    switch(i){
    case 0:
        return UsdGeomTokens->all;
    case 1:
        return UsdGeomTokens->cornersPlus1;
    case 2:
        return UsdGeomTokens->none;
    case 3:
        return UsdGeomTokens->boundaries;
    default:
        TF_CODING_ERROR("Invalid FaceVaryingLinearInterpolation int: %d", i);
        return UsdGeomTokens->none;
    }
}

