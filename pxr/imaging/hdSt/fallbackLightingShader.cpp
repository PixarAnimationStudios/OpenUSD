//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/fallbackLightingShader.h"
#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/package.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hio/glslfx.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


HdSt_FallbackLightingShader::HdSt_FallbackLightingShader()
{
    _glslfx.reset(new HioGlslfx(HdStPackageFallbackLightingShader()));
}

HdSt_FallbackLightingShader::~HdSt_FallbackLightingShader()
{
    // nothing
}

/* virtual */
HdSt_FallbackLightingShader::ID
HdSt_FallbackLightingShader::ComputeHash() const
{
    TfToken glslfxFile = HdStPackageFallbackLightingShader();

    size_t hash = glslfxFile.Hash();

    return (ID)hash;
}

/* virtual */
std::string
HdSt_FallbackLightingShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    return _glslfx->GetSource(shaderStageKey);
}

/* virtual */
void
HdSt_FallbackLightingShader::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                            GfMatrix4d const &projectionMatrix)
{
    // nothing
}

void
HdSt_FallbackLightingShader::BindResources(const int program,
                                           HdSt_ResourceBinder const &binder)
{
    // nothing
}

void
HdSt_FallbackLightingShader::UnbindResources(const int program,
                                             HdSt_ResourceBinder const &binder)
{
    // nothing
}

/*virtual*/
void
HdSt_FallbackLightingShader::AddBindings(
    HdStBindingRequestVector *customBindings)
{
    // no-op
}

PXR_NAMESPACE_CLOSE_SCOPE

