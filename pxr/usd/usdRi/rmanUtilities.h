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
