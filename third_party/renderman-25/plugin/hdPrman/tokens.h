//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_TOKENS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_TOKENS_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_PRMAN_TOKENS                         \
    (meshLight)                                 \
    (meshLightSourceMesh)                       \
    (meshLightSourceVolume)                     \
    (sourceGeom)

TF_DECLARE_PUBLIC_TOKENS(HdPrmanTokens, HDPRMAN_API, HD_PRMAN_TOKENS);

///
/// HdPrmanRileyPrimTypeTokens correspond to Riley::Create/Modify/Delete calls.
///
#define HD_PRMAN_RILEY_PRIM_TYPE_TOKENS                \
    ((camera,             "riley:camera"))             \
    ((clippingPlane,      "riley:clippingPlane"))      \
    ((coordinateSystem,   "riley:coordinateSystem"))   \
    ((displacement,       "riley:displacement"))      \
    ((display,            "riley:display"))            \
    ((displayFilter,      "riley:displayFilter"))      \
    ((geometryInstance,   "riley:geometryInstance"))   \
    ((geometryPrototype,  "riley:geometryPrototype"))  \
    ((globals,            "riley:globals"))            \
    ((integrator,         "riley:integrator"))         \
    ((lightInstance,      "riley:lightInstance"))      \
    ((lightShader,        "riley:lightShader"))        \
    ((material,           "riley:material"))           \
    ((renderOutput,       "riley:renderOutput"))       \
    ((renderTarget,       "riley:renderTarget"))       \
    ((renderView,         "riley:renderView"))         \
    ((sampleFilter,       "riley:sampleFilter"))

TF_DECLARE_PUBLIC_TOKENS(HdPrmanRileyPrimTypeTokens, HDPRMAN_API,
                         HD_PRMAN_RILEY_PRIM_TYPE_TOKENS);

#define HD_PRMAN_RILEY_ADDITIONAL_ROLE_TOKENS         \
    (colorReference)                                  \
    (floatReference)

TF_DECLARE_PUBLIC_TOKENS(HdPrmanRileyAdditionalRoleTokens, HDPRMAN_API,
                         HD_PRMAN_RILEY_ADDITIONAL_ROLE_TOKENS);

#define HD_PRMAN_PLUGIN_TOKENS \
    ((motionBlur,       "HdPrman_MotionBlurSceneIndexPlugin")) \
    ((extComp,          "HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin"))

TF_DECLARE_PUBLIC_TOKENS(HdPrmanPluginTokens, HD_PRMAN_PLUGIN_TOKENS);

const std::vector<std::string>& HdPrman_GetPluginDisplayNames();

PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_TOKENS_H
