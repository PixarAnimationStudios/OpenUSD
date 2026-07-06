//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_VERSION_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_VERSION_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

//        1: Clone off EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_VERSION_H and add light filter support
//  1 ->  2: Add support for camera projection plugins.
//  2 ->  3: Add UsdRiPxrImagingPrimTypeTokens->volumeFilter.
//  3 ->  4: Remove UsdRiPxrImaging from UsdImaging. It now lives with other
//           RenderMan plugins.

#define USD_RI_PXR_IMAGING_API_VERSION 4

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_VERSION_H
