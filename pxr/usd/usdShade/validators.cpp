//
// Copyright 2024 Pixar
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

#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/validatorTokens.h"

#include <algorithm>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

static
UsdValidationErrorVector
_ShaderPropertyTypeConformance(const UsdPrim &usdPrim)
{
    if (!(usdPrim && usdPrim.IsInFamily<UsdShadeShader>(
        UsdSchemaRegistry::VersionPolicy::All))) {
        return {};
    }
    UsdShadeShader shader(usdPrim);
    if (!shader) {
        return {};
    }

    const TfTokenVector expectedImplSource =
        {UsdShadeTokens->id, UsdShadeTokens->sourceAsset, 
            UsdShadeTokens->sourceCode};

    const UsdValidationErrorSites primErrorSite = {
        UsdValidationErrorSite(usdPrim.GetStage(), 
                               usdPrim.GetPath()) };

    const TfToken implSource = shader.GetImplementationSource();
    if (std::find(expectedImplSource.begin(), expectedImplSource.end(),
                  implSource) == expectedImplSource.end()) {
        return {UsdValidationError(UsdValidationErrorType::Error, 
                            primErrorSite,
                            TfStringPrintf("Shader <%s> has invalid "
                                           "implementation source '%s'.", 
                                           usdPrim.GetPath().GetText(), 
                                           implSource.GetText()))};
    }

    const std::vector<std::string> sourceTypes = shader.GetSourceTypes();
    if (sourceTypes.empty() && implSource != UsdShadeTokens->id) {
        return {UsdValidationError(
            UsdValidationErrorType::Error, 
            primErrorSite, 
            TfStringPrintf("Shader <%s> has no sourceType.", 
                           usdPrim.GetPath().GetText()))};
    }

    UsdValidationErrorVector errors;
    std::unordered_map<TfToken, SdrShaderPropertyConstPtr,
        TfToken::HashFunctor> propNameToPropertyMap;
    if (sourceTypes.empty() && implSource == UsdShadeTokens->id) {
        TfToken shaderId;
        if (shader.GetShaderId(&shaderId)) {
            // Single shaderNode, just emplace all properties, no need to find
            // anything.
            SdrShaderNodeConstPtr sdrShaderNode = 
                SdrRegistry::GetInstance().GetShaderNodeByIdentifier(
                    shaderId);
            for (const TfToken &propName : sdrShaderNode->GetInputNames()) {
                if (const SdrShaderPropertyConstPtr sdrProp =
                        sdrShaderNode->GetShaderInput(propName)) {
                    propNameToPropertyMap.emplace(propName, sdrProp);
                }
            }
        }
    } else {
        // Use the SdrShaderNode::CheckPropertyCompliance to find if these do 
        // not match, then report a ValidationError as a warning, since asset 
        // authors have no control on fixing the shaders.
        std::vector<SdrShaderNodeConstPtr> shaderNodesFromSourceTypes;

        // We need to gather all unique inputs from all sdrShaderNodes queried
        // using multiple sourceTypes.
        for (const auto& sourceType : sourceTypes) {
            SdrShaderNodeConstPtr sdrShaderNode = 
                shader.GetShaderNodeForSourceType(TfToken(sourceType));
            shaderNodesFromSourceTypes.push_back(sdrShaderNode);

            for (const TfToken &propName : sdrShaderNode->GetInputNames()) {
                // Check if property has already been added to the map.
                if (propNameToPropertyMap.find(propName) == 
                    propNameToPropertyMap.end()) {
                    if (const SdrShaderPropertyConstPtr sdrProp =
                        sdrShaderNode->GetShaderInput(propName)) {
                            propNameToPropertyMap.emplace(propName, sdrProp);
                    }
                }
            }
        }
        SdrShaderNode::ComplianceResults sdrShaderComplianceResults = 
            SdrShaderNode::CheckPropertyCompliance(shaderNodesFromSourceTypes);
        const UsdValidationErrorSites sdrWarnSite = {
                UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
            };
        for (const auto& entry : sdrShaderComplianceResults) {
            // need this for getting the error message!
            std::vector<std::string> shaderNames;
            shaderNames.reserve(entry.second.size());
            for (const auto& shaderName : entry.second) {
                shaderNames.push_back(shaderName.GetString());
            }
            errors.emplace_back(
                UsdValidationErrorType::Warn, sdrWarnSite,
                TfStringPrintf("Shader nodes '%s' have incompatible property "
                               "'%s'.", TfStringJoin(shaderNames).c_str(), 
                                   entry.first.GetText()));
        }
    }

    // Get ground truth data about inputName to types from sdrNode
    const auto sdrPropnameToSdfType = [&propNameToPropertyMap]() {
        std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor> map;
        for (const auto& prop : propNameToPropertyMap) {
            map.emplace(prop.first, prop.second->GetTypeAsSdfType().first);
        }
        return map;
    }();

    // Compare ground truth data with the inputs on UsdShadeShader prim
    for (const UsdShadeInput &input : shader.GetInputs(false)) {
        const TfToken baseName = input.GetBaseName();
        if (sdrPropnameToSdfType.find(baseName) != 
                sdrPropnameToSdfType.end()) {
            const SdfValueTypeName &expectedSdrInputType = 
                sdrPropnameToSdfType.at(baseName);
            const SdfValueTypeName usdInputType = input.GetTypeName();
            if (usdInputType != expectedSdrInputType) {
                const UsdValidationErrorSites inputErrorSite = {
                        UsdValidationErrorSite(usdPrim.GetStage(),
                                               input.GetAttr().GetPath())
                    };
                errors.emplace_back(
                    UsdValidationErrorType::Error,
                    inputErrorSite,
                    TfStringPrintf("Incorrect type for %s. "
                                   "Expected '%s'; got '%s'.",
                                   input.GetAttr().GetPath().GetText(),
                                   expectedSdrInputType.GetAsToken().GetText(),
                                   usdInputType.GetAsToken().GetText()));
            }
        }
    }
    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdShadeValidatorNameTokens->shaderSdrCompliance, 
        _ShaderPropertyTypeConformance);
}

PXR_NAMESPACE_CLOSE_SCOPE
