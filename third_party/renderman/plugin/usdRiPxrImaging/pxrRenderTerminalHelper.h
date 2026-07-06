//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_RENDER_TERMINAL_HELPER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_RENDER_TERMINAL_HELPER_H

/// \file usdRiPxrImaging/pxrRenderTerminalHelper.h

#include "pxr/imaging/hd/material.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdRiPxrImagingRenderTerminalHelper
///
/// Helper to translate the PxrRenderTerminalsAPI (Integrator, Sample Filter
/// and Display Filter) prims into their corresponding HdMaterialNode2 resource.
///
class UsdRiPxrImagingRenderTerminalHelper
{
public:
    static
    HdMaterialNode2 CreateHdMaterialNode2(
        UsdPrim const& prim,
        TfToken const& shaderIdToken,
        TfToken const& primTypeToken);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_RENDER_TERMINAL_HELPER_H
