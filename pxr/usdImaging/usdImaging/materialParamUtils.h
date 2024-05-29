//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_MATERIAL_PARAM_UTILS_H
#define PXR_USD_IMAGING_MATERIAL_PARAM_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdMaterialNetworkMap;
class UsdAttribute;
class UsdPrim;
class UsdTimeCode;
class VtValue;

/// Builds an HdMaterialNetwork for the usdTerminal prim and 
/// populates it in the materialNetworkMap under the terminalIdentifier. 
/// This shared implementation is usable for populating material networks for
/// any connectable source including lights and light filters in addition to 
/// materials.
USDIMAGING_API
void 
UsdImagingBuildHdMaterialNetworkFromTerminal(
    UsdPrim const& usdTerminal,
    TfToken const& terminalIdentifier,
    TfTokenVector const& shaderSourceTypes,
    TfTokenVector const& renderContexts,
    HdMaterialNetworkMap *materialNetworkMap,
    UsdTimeCode time);

/// Returns whether the material network built by 
/// UsdImagingBuildHdMaterialNetworkFromTerminal for the given usdTerminal 
/// prim is time varying.
USDIMAGING_API
bool
UsdImagingIsHdMaterialNetworkTimeVarying(
    UsdPrim const& usdTerminal);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_MATERIAL_PARAM_UTILS_H
