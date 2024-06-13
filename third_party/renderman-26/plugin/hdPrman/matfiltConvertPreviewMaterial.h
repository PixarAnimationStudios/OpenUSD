//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_CONVERT_PREVIEW_MATERIAL_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_CONVERT_PREVIEW_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMaterialNetworkInterface;

/// Converts USD preview shading nodes to Renderman equivalents.
void
MatfiltConvertPreviewMaterial(
    HdMaterialNetworkInterface *networkInterface,
    std::vector<std::string> *outputErrorMessages);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_CONVERT_PREVIEW_MATERIAL_H
