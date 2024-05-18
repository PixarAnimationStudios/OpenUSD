//
// Copyright 2020 Pixar
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
#ifndef PXR_IMAGING_HD_ST_MATERIALX_FILTER_H
#define PXR_IMAGING_HD_ST_MATERIALX_FILTER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/usd/sdf/path.h"
#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/Shader.h>

PXR_NAMESPACE_OPEN_SCOPE

// Storing MaterialX-Hydra counterparts and other Hydra specific information
struct HdSt_MxShaderGenInfo {
    HdSt_MxShaderGenInfo() 
        : textureMap(MaterialX::StringMap()), 
          primvarMap(MaterialX::StringMap()), 
          primvarDefaultValueMap(MaterialX::StringMap()), 
          defaultTexcoordName("st"),
          materialTag(HdStMaterialTagTokens->defaultMaterialTag.GetString()),
          bindlessTexturesEnabled(false) {}
    MaterialX::StringMap textureMap;
    MaterialX::StringMap primvarMap;
    MaterialX::StringMap primvarDefaultValueMap;
    std::string defaultTexcoordName;
    std::string materialTag;
    bool bindlessTexturesEnabled;
};

/// MaterialX Filter
/// Converting a MaterialX node to one with a generated MaterialX glslfx file
MaterialX::ShaderPtr HdSt_ApplyMaterialXFilter(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& materialPath,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath,
    HdSt_MaterialParamVector* materialParams,
    HdStResourceRegistry *resourceRegistry);

// Generates the glsfx shader for the given MaterialX Document
MaterialX::ShaderPtr HdSt_GenMaterialXShader(
    MaterialX::DocumentPtr const& mxDoc,
    MaterialX::DocumentPtr const& stdLibraries,
    MaterialX::FileSearchPath const& searchPath,
    HdSt_MxShaderGenInfo const& mxHdInfo=HdSt_MxShaderGenInfo(),
    TfToken const& apiName=TfToken());

PXR_NAMESPACE_CLOSE_SCOPE

#endif
