//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_PACKAGE_H
#define PXR_IMAGING_HDX_PACKAGE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


TfToken HdxPackageFullscreenShader();
TfToken HdxPackageRenderPassColorShader();
TfToken HdxPackageRenderPassColorAndSelectionShader();
TfToken HdxPackageRenderPassColorWithOccludedSelectionShader();
TfToken HdxPackageRenderPassPickingShader();
TfToken HdxPackageRenderPassShadowShader();
TfToken HdxPackageColorChannelShader();
TfToken HdxPackageColorCorrectionShader();
TfToken HdxPackageVisualizeAovShader();
TfToken HdxPackageRenderPassOitShader();
TfToken HdxPackageRenderPassOitOpaqueShader();
TfToken HdxPackageRenderPassOitVolumeShader();
TfToken HdxPackageOitResolveImageShader();
TfToken HdxPackageOutlineShader();
TfToken HdxPackageSkydomeShader();
TfToken HdxPackageBoundingBoxShader();

TfToken HdxPackageDefaultDomeLightTexture();

PXR_NAMESPACE_CLOSE_SCOPE

#endif
