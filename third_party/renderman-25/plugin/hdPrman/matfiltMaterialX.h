//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_MATERIALX_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_MATERIALX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include "pxr/usd/ndr/declare.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMaterialNetworkInterface;

/// Processes MaterialX shading node graphs for RenderMan.
/// 
/// The terminal nodes are converted to PxrSurface, PxrDisplacement,
/// and PxrVolume respectively, and any input graphs that use MaterialX
/// shader code-generation are compiled and replaced with a single node.
void
MatfiltMaterialX(
    HdMaterialNetworkInterface *netInterface,
    std::vector<std::string> *outputErrorMessages);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_MATERIALX_H
