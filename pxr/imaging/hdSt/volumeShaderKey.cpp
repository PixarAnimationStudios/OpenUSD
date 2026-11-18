//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/volumeShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,         "volume.glslfx"))

    // point id mixins (provide functions for picking system)
    ((pointIdFS,          "PointId.Fragment.Fallback"))

    // main for all the shader stages
    ((mainVS,             "Volume.Vertex"))
    ((mainFS,             "Volume.Fragment"))

    // instancing       
    ((instancing,         "Instancing.Transform"))

    // utils       
    ((transformUtils,     "CoordUtils.Transform"))
    ((projectionUtils,    "CoordUtils.Projection"))
    ((depthUtilsCommon,   "CoordUtils.Depth.Common"))
    ((depthUtilsFragment, "CoordUtils.Depth.Fragment"))
);

HdSt_VolumeShaderKey::HdSt_VolumeShaderKey()
    : glslfx(_tokens->baseGLSLFX),
      VS{ _tokens->instancing, _tokens->mainVS, TfToken() },
      FS{ _tokens->pointIdFS,
          _tokens->transformUtils,
          _tokens->projectionUtils,
          _tokens->depthUtilsCommon,
          _tokens->depthUtilsFragment,
          _tokens->instancing,
          _tokens->mainFS, TfToken() }
{
}

HdSt_VolumeShaderKey::~HdSt_VolumeShaderKey() = default;

PXR_NAMESPACE_CLOSE_SCOPE

