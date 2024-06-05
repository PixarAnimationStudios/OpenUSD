//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_LIGHTING_SHADER_H
#define PXR_IMAGING_HD_ST_LIGHTING_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdStLightingShaderSharedPtr = std::shared_ptr<class HdStLightingShader>;

/// \class HdStLightingShader
///
/// A lighting shader base class.
///
class HdStLightingShader : public HdStShaderCode {
public:
    HDST_API
    HdStLightingShader();
    HDST_API
    virtual ~HdStLightingShader();

    /// Sets camera state.
    virtual void SetCamera(GfMatrix4d const &worldToViewMatrix,
                           GfMatrix4d const &projectionMatrix) = 0;

private:

    // No copying
    HdStLightingShader(const HdStLightingShader &)                     = delete;
    HdStLightingShader &operator =(const HdStLightingShader &)         = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_LIGHTING_SHADER_H
