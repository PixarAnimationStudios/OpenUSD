//
// Copyright 2021 Pixar
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
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/textShaderKey.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX, "text.glslfx"))

    // text shader.
    ((vsShaderText, "VSShaderText"))
    ((psShaderText, "PSShaderText"))

    // point id mixins
    ((pointIdNoneVS, "PointId.Vertex.None"))
    ((pointIdFallbackFS, "PointId.Fragment.Fallback"))
    
    // instancing related mixins
    ((instancing, "Instancing.Transform"))

    // terminals
    ((commonFS, "Fragment.CommonTerminals"))
    ((surfaceFS, "Fragment.Surface"))
    ((scalarOverrideFS, "Fragment.ScalarOverride"))
);

HdSt_TextShaderKey::HdSt_TextShaderKey()
    : glslfx(_tokens->baseGLSLFX)
{
    primType = HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;
    VS[0] = _tokens->instancing;
    VS[1] = _tokens->vsShaderText;
    VS[2] = _tokens->pointIdNoneVS;
    VS[3] = TfToken();
    FS[0] = _tokens->psShaderText;
    FS[1] = _tokens->pointIdFallbackFS;
    FS[2] = _tokens->commonFS;
    FS[3] = _tokens->surfaceFS;
    FS[4] = _tokens->scalarOverrideFS;
    FS[5] = TfToken();
}

HdSt_TextShaderKey::~HdSt_TextShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

