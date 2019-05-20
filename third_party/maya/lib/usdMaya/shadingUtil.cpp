//
// Copyright 2018 Pixar
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
#include "usdMaya/shadingUtil.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"


PXR_NAMESPACE_USING_DIRECTIVE


UsdShadeInput
UsdMayaShadingUtil::CreateMaterialInputAndConnectShader(
        UsdShadeMaterial& material,
        const TfToken& materialInputName,
        const SdfValueTypeName& inputTypeName,
        UsdShadeShader& shader,
        const TfToken& shaderInputName)
{
    if (!material || !shader) {
        return UsdShadeInput();
    }

    UsdShadeInput materialInput =
        material.CreateInput(materialInputName, inputTypeName);

    UsdShadeInput shaderInput =
        shader.CreateInput(shaderInputName, inputTypeName);

    shaderInput.ConnectToSource(materialInput);

    return materialInput;
}

UsdShadeOutput
UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
        UsdShadeShader& shader,
        const TfToken& shaderOutputName,
        const SdfValueTypeName& outputTypeName,
        UsdShadeMaterial& material,
        const TfToken& materialOutputName)
{
    if (!shader || !material) {
        return UsdShadeOutput();
    }

    UsdShadeOutput shaderOutput =
        shader.CreateOutput(shaderOutputName, outputTypeName);

    UsdShadeOutput materialOutput =
        material.CreateOutput(materialOutputName, outputTypeName);

    materialOutput.ConnectToSource(shaderOutput);

    return shaderOutput;
}
