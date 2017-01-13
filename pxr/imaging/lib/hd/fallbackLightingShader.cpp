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
#include "pxr/imaging/hd/fallbackLightingShader.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/imaging/glf/glslfx.h"

#include <boost/functional/hash.hpp>

#include <string>

Hd_FallbackLightingShader::Hd_FallbackLightingShader()
{
    _glslfx.reset(new GlfGLSLFX(HdPackageFallbackLightingShader()));
}

Hd_FallbackLightingShader::~Hd_FallbackLightingShader()
{
    // nothing
}

/* virtual */
Hd_FallbackLightingShader::ID
Hd_FallbackLightingShader::ComputeHash() const
{
    TfToken glslfxFile = HdPackageFallbackLightingShader();

    size_t hash = glslfxFile.Hash();

    return (ID)hash;
}

/* virtual */
std::string
Hd_FallbackLightingShader::GetSource(TfToken const &shaderStageKey) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    return _glslfx->GetSource(shaderStageKey);
}

/* virtual */
void
Hd_FallbackLightingShader::SetCamera(GfMatrix4d const &worldToViewMatrix,
                                            GfMatrix4d const &projectionMatrix)
{
    // nothing
}

void
Hd_FallbackLightingShader::BindResources(Hd_ResourceBinder const &binder,
                                        int program)
{
    // nothing
}

void
Hd_FallbackLightingShader::UnbindResources(Hd_ResourceBinder const &binder,
                                          int program)
{
    // nothing
}

/*virtual*/
void
Hd_FallbackLightingShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // no-op
}
