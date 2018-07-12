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
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/pointsShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,      "points.glslfx"))

    // point id fallback mixin
    // The only reason this is here is to generate fallback versions of
    // functions to keep the compiler happy during code gen.
    ((pointIdVS,       "PointId.Vertex.None"))
    ((pointIdFS,       "PointId.Fragment.Fallback"))

    // main for all the shader stages
    ((mainVS,          "Point.Vertex"))
    ((mainFS,          "Point.Fragment"))

    // terminals
    ((commonFS,        "Fragment.CommonTerminals"))
    ((surfaceFS,       "Fragment.Surface"))
    
    // instancing
    ((instancing,      "Instancing.Transform"))
);

HdSt_PointsShaderKey::HdSt_PointsShaderKey()
    : glslfx(_tokens->baseGLSLFX)
{
    VS[0] = _tokens->instancing;
    VS[1] = _tokens->mainVS;
    VS[2] = _tokens->pointIdVS;
    VS[3] = TfToken();

    // Common must be first as it defines terminal interfaces
    FS[0] = _tokens->commonFS;
    FS[1] = _tokens->surfaceFS;
    FS[2] = _tokens->mainFS;
    FS[3] = _tokens->pointIdFS;
    FS[4] = TfToken();
}

HdSt_PointsShaderKey::~HdSt_PointsShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

