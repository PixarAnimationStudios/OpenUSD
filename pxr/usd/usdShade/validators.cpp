//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/validatorTokens.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

static
UsdValidationErrorVector
_EncapsulationValidator(const UsdPrim& usdPrim)
{
    const UsdShadeConnectableAPI& connectable =
            UsdShadeConnectableAPI(usdPrim);

    if (!connectable){
        return {};
    }

    const UsdPrim& parentPrim = usdPrim.GetParent();

    if (!parentPrim || parentPrim.IsPseudoRoot()){
        return {};
    }

    UsdShadeConnectableAPI parentConnectable =
            UsdShadeConnectableAPI(parentPrim);
    UsdValidationErrorVector errors;
    if (parentConnectable && !parentConnectable.IsContainer()) {
        // It is a violation of the UsdShade OM which enforces
        // encapsulation of connectable prims under a Container-type
        // connectable prim.
        errors.emplace_back(
            UsdShadeValidationErrorNameTokens->connectableInNonContainer,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites{
                    UsdValidationErrorSite(usdPrim.GetStage(),
                                           usdPrim.GetPath())
            },
            TfStringPrintf("Connectable %s <%s> cannot reside "
                           "under a non-Container Connectable %s",
                           usdPrim.GetTypeName().GetText(),
                           usdPrim.GetPath().GetText(),
                           parentPrim.GetTypeName().GetText()));
    }
    else if (!parentConnectable) {
        std::function<void(const UsdPrim&)> _VerifyValidAncestor =
                [&](const UsdPrim& currentAncestor) -> void {
            if (!currentAncestor || currentAncestor.IsPseudoRoot()) {
                return;
            }
            const UsdShadeConnectableAPI& ancestorConnectable =
                    UsdShadeConnectableAPI(currentAncestor);
            if (ancestorConnectable) {
                // it's only OK to have a non-connectable parent if all
                // the rest of your ancestors are also non-connectable.
                // The error message we give is targeted at the most common
                // infraction, using Scope or other grouping prims inside
                // a Container like a Material
                errors.emplace_back(
                    UsdShadeValidationErrorNameTokens->
                        invalidConnectableHierarchy,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(usdPrim.GetStage(),
                                               usdPrim.GetPath()) },
                    TfStringPrintf("Connectable %s <%s> can only have "
                                   "Connectable Container ancestors up to %s "
                                   "ancestor <%s>, but its parent %s is a %s.",
                                   usdPrim.GetTypeName().GetText(),
                                   usdPrim.GetPath().GetText(),
                                   currentAncestor.GetTypeName().GetText(),
                                   currentAncestor.GetPath().GetText(),
                                   parentPrim.GetName().GetText(),
                                   parentPrim.GetTypeName().GetText()));
                return;
            }
            _VerifyValidAncestor(currentAncestor.GetParent());
        };
        _VerifyValidAncestor(parentPrim.GetParent());
    }

    return errors;
}

static
UsdValidationErrorVector
_MaterialBindingApiAppliedValidator(const UsdPrim &usdPrim)
{
    UsdValidationErrorVector errors;

    auto hasMaterialBindingRelationship = [](const UsdPrim& usdPrim) {
        const std::vector<UsdRelationship> relationships = 
            usdPrim.GetRelationships();
        static const std::string materialBindingString = 
            (UsdShadeTokens->materialBinding).GetString();

        return std::any_of(relationships.begin(), 
                           relationships.end(), 
                           [&](const UsdRelationship &rel) {
                               return TfStringStartsWith(rel.GetName(), 
                                                         materialBindingString);
});
    };

    if (!usdPrim.HasAPI<UsdShadeMaterialBindingAPI>() &&
        hasMaterialBindingRelationship(usdPrim)) {
        errors.emplace_back(
            UsdShadeValidationErrorNameTokens->missingMaterialBindingAPI,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites{
                    UsdValidationErrorSite(usdPrim.GetStage(),
                                           usdPrim.GetPath())
            },
            TfStringPrintf("Found material bindings but no MaterialBindingAPI "
                           "applied on the prim <%s>.", 
                           usdPrim.GetPath().GetText()));
    }

    return errors;
}

static
UsdValidationErrorVector
_MaterialBindingRelationships(const UsdPrim& usdPrim)
{
    if (!usdPrim) {
        return {};
    }

    const std::vector<UsdProperty> matBindingProperties =
        usdPrim.GetProperties(
            /* predicate = */ [](const TfToken& name) {
                return UsdShadeMaterialBindingAPI::CanContainPropertyName(
                    name);
            }
        );

    UsdValidationErrorVector errors;

    for (const UsdProperty& matBindingProperty : matBindingProperties) {
        if (matBindingProperty.Is<UsdRelationship>()) {
            continue;
        }

        const UsdValidationErrorSites propertyErrorSites = {
            UsdValidationErrorSite(
                usdPrim.GetStage(),
                matBindingProperty.GetPath())
        };

        errors.emplace_back(
            UsdShadeValidationErrorNameTokens->materialBindingPropNotARel,
            UsdValidationErrorType::Error,
            propertyErrorSites,
            TfStringPrintf(
                "Prim <%s> has material binding property '%s' that is not "
                "a relationship.",
                usdPrim.GetPath().GetText(),
                matBindingProperty.GetName().GetText())
        );
    }

    return errors;
}

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

    const TfToken implSource = shader.GetImplementationSource();
    if (std::find(expectedImplSource.begin(), expectedImplSource.end(),
                  implSource) == expectedImplSource.end()) {
        const UsdValidationErrorSites implSourceErrorSite = {
            UsdValidationErrorSite(
                usdPrim.GetStage(), 
                shader.GetImplementationSourceAttr().GetPath()) };
        return {UsdValidationError(
            UsdShadeValidationErrorNameTokens->invalidImplSource,
            UsdValidationErrorType::Error, implSourceErrorSite,
            TfStringPrintf("Shader <%s> has invalid implementation source "
                           "'%s'.", usdPrim.GetPath().GetText(), 
                           implSource.GetText()))};
    }

    const std::vector<std::string> sourceTypes = shader.GetSourceTypes();
    if (sourceTypes.empty() && implSource != UsdShadeTokens->id) {
        const UsdValidationErrorSites primErrorSite = {
            UsdValidationErrorSite(usdPrim.GetStage(), 
                                   usdPrim.GetPath()) };
        return {UsdValidationError(
            UsdShadeValidationErrorNameTokens->missingSourceType,
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
            if (SdrShaderNodeConstPtr sdrShaderNode = 
                SdrRegistry::GetInstance().GetShaderNodeByIdentifier(
                    shaderId)) {
                for (const TfToken &propName : sdrShaderNode->GetInputNames()) {
                    if (const SdrShaderPropertyConstPtr sdrProp =
                        sdrShaderNode->GetShaderInput(propName)) {
                        propNameToPropertyMap.emplace(propName, sdrProp);
                    }
                }
            } else {
                const UsdValidationErrorSites shaderIdErrorSite = {
                    UsdValidationErrorSite(usdPrim.GetStage(), 
                                           shader.GetIdAttr().GetPath()) };
                return {UsdValidationError(
                    UsdShadeValidationErrorNameTokens->
                        missingShaderIdInRegistry,
                    UsdValidationErrorType::Error,
                    shaderIdErrorSite,
                    TfStringPrintf("shaderId '%s' specified on shader prim "
                                   "<%s> not found in sdrRegistry.",
                                   shaderId.GetText(), 
                                   usdPrim.GetPath().GetText()))};
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
            if (SdrShaderNodeConstPtr sdrShaderNode = 
                shader.GetShaderNodeForSourceType(TfToken(sourceType))) {
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
            } else {
                UsdValidationErrorSites sourceTypeSites;
                for (const auto& sourceTypeProp : 
                         usdPrim.GetPropertiesInNamespace(
                             SdfPath::JoinIdentifier("info", sourceType))) {
                    sourceTypeSites.emplace_back(usdPrim.GetStage(), 
                                                 sourceTypeProp.GetPath());
                }
                errors.emplace_back(
                    UsdShadeValidationErrorNameTokens->
                        missingSourceTypeInRegistry,
                    UsdValidationErrorType::Error,
                    sourceTypeSites,
                    TfStringPrintf("sourceType '%s' specified on shader prim "
                                   "<%s> not found in sdrRegistry.",
                                   sourceType.c_str(), 
                                   usdPrim.GetPath().GetText()));
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
                UsdShadeValidationErrorNameTokens->
                    incompatShaderPropertyWarning,
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
            map.emplace(prop.first,
                        prop.second->GetTypeAsSdfType().GetSdfType());
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
                    UsdShadeValidationErrorNameTokens->mismatchPropertyType,
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

static
UsdValidationErrorVector
_SubsetMaterialBindFamilyName(const UsdPrim& usdPrim)
{
    if (!(usdPrim && usdPrim.IsInFamily<UsdGeomSubset>(
            UsdSchemaRegistry::VersionPolicy::All))) {
        return {};
    }

    const UsdGeomSubset subset(usdPrim);
    if (!subset) {
        return {};
    }

    size_t numMatBindingRels = 0u;

    const std::vector<UsdProperty> matBindingProperties =
        usdPrim.GetProperties(
            /* predicate = */ [](const TfToken& name) {
                return UsdShadeMaterialBindingAPI::CanContainPropertyName(
                    name);
            }
        );
    for (const UsdProperty& matBindingProperty : matBindingProperties) {
        if (matBindingProperty.Is<UsdRelationship>()) {
            ++numMatBindingRels;
        }
    }

    if (numMatBindingRels < 1u) {
        return {};
    }

    if (subset.GetFamilyNameAttr().HasAuthoredValue()) {
        return {};
    }

    const UsdValidationErrorSites primErrorSites = {
        UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
    };

    return {
        UsdValidationError(
            UsdShadeValidationErrorNameTokens->missingFamilyNameOnGeomSubset,
            UsdValidationErrorType::Error,
            primErrorSites,
            TfStringPrintf(
                "GeomSubset prim <%s> with material bindings applied but "
                "no authored family name should set familyName to '%s'.",
                usdPrim.GetPath().GetText(),
                UsdShadeTokens->materialBind.GetText())
        )
    };
}

static
UsdValidationErrorVector
_SubsetsMaterialBindFamily(const UsdPrim& usdPrim)
{
    if (!(usdPrim && usdPrim.IsInFamily<UsdGeomImageable>(
            UsdSchemaRegistry::VersionPolicy::All))) {
        return {};
    }

    const UsdGeomImageable imageable(usdPrim);
    if (!imageable) {
        return {};
    }

    const std::vector<UsdGeomSubset> materialBindSubsets =
        UsdGeomSubset::GetGeomSubsets(
            imageable,
            /* elementType = */ TfToken(),
            /* familyName = */ UsdShadeTokens->materialBind);

    if (materialBindSubsets.empty()) {
        return {};
    }

    UsdValidationErrorVector errors;

    // Check to make sure that the "materialBind" family is of a restricted
    // type, since it is invalid for an element of geometry to be bound to
    // multiple materials.
    const TfToken materialBindFamilyType = UsdGeomSubset::GetFamilyType(
        imageable,
        UsdShadeTokens->materialBind);
    if (materialBindFamilyType == UsdGeomTokens->unrestricted) {
        const UsdValidationErrorSites primErrorSites = {
            UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
        };

        errors.emplace_back(
            UsdShadeValidationErrorNameTokens->invalidFamilyType,
            UsdValidationErrorType::Error,
            primErrorSites,
            TfStringPrintf(
                "Imageable prim <%s> has '%s' subset family with invalid "
                "family type '%s'. Family type should be '%s' or '%s' "
                "instead.",
                usdPrim.GetPath().GetText(),
                UsdShadeTokens->materialBind.GetText(),
                materialBindFamilyType.GetText(),
                UsdGeomTokens->nonOverlapping.GetText(),
                UsdGeomTokens->partition.GetText())
        );
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdShadeValidatorNameTokens->materialBindingApiAppliedValidator,
        _MaterialBindingApiAppliedValidator);

    registry.RegisterPluginValidator(
        UsdShadeValidatorNameTokens->materialBindingRelationships,
        _MaterialBindingRelationships);

    registry.RegisterPluginValidator(
        UsdShadeValidatorNameTokens->shaderSdrCompliance, 
        _ShaderPropertyTypeConformance);

    registry.RegisterPluginValidator(
        UsdShadeValidatorNameTokens->subsetMaterialBindFamilyName,
        _SubsetMaterialBindFamilyName);

    registry.RegisterPluginValidator(
        UsdShadeValidatorNameTokens->subsetsMaterialBindFamily,
        _SubsetsMaterialBindFamily);

    registry.RegisterPluginValidator(
            UsdShadeValidatorNameTokens->encapsulationValidator,
            _EncapsulationValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE
