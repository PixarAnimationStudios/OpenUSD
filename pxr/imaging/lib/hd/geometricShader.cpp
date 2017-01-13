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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/geometricShader.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/glslfx.h"

#include <boost/functional/hash.hpp>

#include <string>

Hd_GeometricShader::Hd_GeometricShader(std::string const &glslfxString,
                                       int16_t primitiveMode,
                                       int16_t primitiveIndexSize,
                                       HdCullStyle cullStyle,
                                       HdPolygonMode polygonMode,
                                       bool cullingPass,
                                       SdfPath const &debugId)
    : _primitiveMode(primitiveMode)
    , _primitiveIndexSize(primitiveIndexSize)
    , _cullStyle(cullStyle)
    , _polygonMode(polygonMode)
    , _cullingPass(cullingPass)
    , _hash(0)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX
    // we will likely move this (the constructor or the entire class) into
    // the base class (HdShader) at the end of refactoring, to be able to
    // use same machinery other than geometric shaders.

    if (TfDebug::IsEnabled(HD_DUMP_GLSLFX_CONFIG)) {
        std::cout << debugId << "\n"
                  << glslfxString << "\n";
    }

    std::stringstream ss(glslfxString);
    _glslfx.reset(new GlfGLSLFX(ss));
    boost::hash_combine(_hash, _glslfx->GetHash());
    boost::hash_combine(_hash, cullingPass);
    boost::hash_combine(_hash, primitiveMode);
    boost::hash_combine(_hash, primitiveIndexSize);
    //
    // note: Don't include cullStyle and polygonMode into the hash.
    //      They are independent from the GLSL program.
    //
}

Hd_GeometricShader::~Hd_GeometricShader()
{
    // nothing
}

/* virtual */
HdShader::ID
Hd_GeometricShader::ComputeHash() const
{
    return _hash;
}

/* virtual */
std::string
Hd_GeometricShader::GetSource(TfToken const &shaderStageKey) const
{
    return _glslfx->GetSource(shaderStageKey);
}

void
Hd_GeometricShader::BindResources(Hd_ResourceBinder const &binder, int program)
{
    if (_cullStyle != HdCullStyleDontCare) {
        unsigned int cullStyle = _cullStyle;
        binder.BindUniformui(HdShaderTokens->cullStyle, 1, &cullStyle);
    } else {
        // don't care -- use renderPass's fallback
    }

    if (_primitiveMode == GL_PATCHES) {
        glPatchParameteri(GL_PATCH_VERTICES, _primitiveIndexSize);
    }

    if (_polygonMode == HdPolygonModeLine) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
}

void
Hd_GeometricShader::UnbindResources(Hd_ResourceBinder const &binder, int program)
{
    if (_polygonMode == HdPolygonModeLine) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

/*virtual*/
void
Hd_GeometricShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // no-op
}
