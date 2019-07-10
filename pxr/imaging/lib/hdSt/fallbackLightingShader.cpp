//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/hdSt/fallbackLightingShader.h"
#include "pxr/imaging/hdSt/package.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/hio/glslfx.h"

#include <boost/functional/hash.hpp>

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
HdSt_FallbackLightingShader::BindResources(HdSt_ResourceBinder const &binder,
                                        int program)
{
    // nothing
}

void
HdSt_FallbackLightingShader::UnbindResources(HdSt_ResourceBinder const &binder,
                                          int program)
{
    // nothing
}

/*virtual*/
void
HdSt_FallbackLightingShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // no-op
}

PXR_NAMESPACE_CLOSE_SCOPE

