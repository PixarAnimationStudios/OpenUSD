//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_FALLBACK_LIGHTING_SHADER_H
#define PXR_IMAGING_HD_ST_FALLBACK_LIGHTING_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/lightingShader.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HioGlslfx;

/// \class HdSt_FallbackLightingShader
///
/// A shader that provides fallback lighting behavior.
///
class HdSt_FallbackLightingShader : public HdStLightingShader
{
public:
    HDST_API
    HdSt_FallbackLightingShader();
    HDST_API
    ~HdSt_FallbackLightingShader() override;

    // HdStShaderCode overrides
    HDST_API
    ID ComputeHash() const override;
    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                       HdSt_ResourceBinder const &binder) override;
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder) override;
    HDST_API
    void AddBindings(HdStBindingRequestVector *customBindings) override;

    // HdStLightingShader overrides
    HDST_API
    void SetCamera(GfMatrix4d const &worldToViewMatrix,
                   GfMatrix4d const &projectionMatrix) override;

private:
    std::unique_ptr<HioGlslfx> _glslfx;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_FALLBACK_LIGHTING_SHADER_H
