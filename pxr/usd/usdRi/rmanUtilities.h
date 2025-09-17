//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_RI_RMAN_UTILITIES_H
#define PXR_USD_USD_RI_RMAN_UTILITIES_H

/// \file usdRi/rmanUtilities.h
/// Utilities for converting between USD encodings and Renderman encodings in
/// cases where there is a difference.

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

/// Given a \p token representing a UsdGeom interpolate boundary value, returns
/// corresponding rman enum (converted to int).
USDRI_API
int UsdRiConvertToRManInterpolateBoundary(const TfToken &token);

/// Given the integer \p i that corresponds to an rman enum for interpolate
/// boundary condition, returns the equivalent UsdGeom token.
USDRI_API
const TfToken &UsdRiConvertFromRManInterpolateBoundary(int i);

/// Given a \p token representing a UsdGeom face-varying interpolate boundary 
/// value, returns corresponding rman enum (converted to int).
USDRI_API
int UsdRiConvertToRManFaceVaryingLinearInterpolation(const TfToken &token);

/// Given the integer \p i that corresponds to an rman enum for face-varying
/// interpolate boundary condition, returns the equivalent UsdGeom token.
USDRI_API
const TfToken &UsdRiConvertFromRManFaceVaryingLinearInterpolation(int i);

/// Given a \p token representing a UsdGeom Catmull-Clark triangle subdivision
/// rule value, returns corresponding rman enum (converted to int).
USDRI_API
int UsdRiConvertToRManTriangleSubdivisionRule(const TfToken &token);

/// Given the integer \p i that corresponds to an rman enum for a Catmull-
/// Clark triangle subdivision rule, returns the equivalent UsdGeom token.
USDRI_API
const TfToken &UsdRiConvertFromRManTriangleSubdivisionRule(int i);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_RI_RMAN_UTILITIES_H
