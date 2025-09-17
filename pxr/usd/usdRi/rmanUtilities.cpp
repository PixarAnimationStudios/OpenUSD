//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdRi/rmanUtilities.h"

#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


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
    if(token == UsdGeomTokens->all) {
        return 0;
    }
    else if(token == UsdGeomTokens->cornersOnly
            || token == UsdGeomTokens->cornersPlus1
            || token == UsdGeomTokens->cornersPlus2) {
        return 1;
    }
    else if(token == UsdGeomTokens->none) {
        return 2;
    }
    else if(token == UsdGeomTokens->boundaries) {
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

int
UsdRiConvertToRManTriangleSubdivisionRule(TfToken const& token)
{
    // XXX A value of 2 is needed in order for the smoothing algorithm to work.
    if(token == UsdGeomTokens->catmullClark) {
        return 0;
    }
    else if(token == UsdGeomTokens->smooth)
        return 2;
    else{
        TF_CODING_ERROR("Invalid TriangleSubdivisionRule Token: %s",
            token.GetText());
        return 0;
    }
}

TfToken const&
UsdRiConvertFromRManTriangleSubdivisionRule(int i)
{
    // XXX A value of 2 is needed in order for the smoothing algorithm to work.
    switch(i){
    case 0:
        return UsdGeomTokens->catmullClark;
    case 2:
        return UsdGeomTokens->smooth;
    default:
        TF_CODING_ERROR("Invalid TriangleSubdivisionRule int: %d", i);
        return UsdGeomTokens->catmullClark;
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

