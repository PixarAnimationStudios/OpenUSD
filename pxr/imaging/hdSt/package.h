//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_PACKAGE_H
#define PXR_IMAGING_HD_ST_PACKAGE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


HDST_API
TfToken HdStPackageComputeShader();

HDST_API
TfToken HdStPackageDomeLightShader();

HDST_API
TfToken HdStPackageFallbackDomeLightTexture();

HDST_API
TfToken HdStPackagePtexTextureShader();

HDST_API
TfToken HdStPackageRenderPassShader();

HDST_API
TfToken HdStPackageFallbackLightingShader();

HDST_API
TfToken HdStPackageFallbackMaterialNetworkShader();

HDST_API
TfToken HdStPackageInvalidMaterialNetworkShader();

HDST_API
TfToken HdStPackageFallbackVolumeShader();

HDST_API
TfToken HdStPackageImageShader();

HDST_API
TfToken HdStPackageSimpleLightingShader();

HDST_API
TfToken HdStPackageOverlayShader();

PXR_NAMESPACE_CLOSE_SCOPE

#endif
