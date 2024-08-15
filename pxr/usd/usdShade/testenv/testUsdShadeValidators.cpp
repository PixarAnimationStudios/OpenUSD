//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usdGeom/validatorTokens.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/validatorTokens.h"
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/prim.h>

#include <algorithm>
#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestUsdShadeValidators()
{
    // This should be updated with every new validator added with the
    // UsdShadeValidators keyword.
    const std::set<TfToken> expectedUsdShadeValidatorNames = {
        UsdShadeValidatorNameTokens->materialBindingApiAppliedValidator,
        UsdShadeValidatorNameTokens->materialBindingRelationships,
        UsdShadeValidatorNameTokens->shaderSdrCompliance,
        UsdShadeValidatorNameTokens->subsetMaterialBindFamilyName,
        UsdShadeValidatorNameTokens->subsetsMaterialBindFamily
    };

    // This should be updated with every new validator added with the
    // UsdGeomSubset keyword.
    const std::set<TfToken> expectedUsdGeomSubsetNames = {
        UsdShadeValidatorNameTokens->subsetMaterialBindFamilyName,
        UsdShadeValidatorNameTokens->subsetsMaterialBindFamily
    };

    const UsdValidationRegistry& registry =
        UsdValidationRegistry::GetInstance();

    // Since other validators can be registered with the same keywords,
    // our validators registered in usdShade are/may be a subset of the
    // entire set.
    std::set<TfToken> validatorMetadataNameSet;

    UsdValidatorMetadataVector metadata =
        registry.GetValidatorMetadataForKeyword(
            UsdShadeValidatorKeywordTokens->UsdShadeValidators);
    for (const UsdValidatorMetadata& metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedUsdShadeValidatorNames.begin(),
                           expectedUsdShadeValidatorNames.end()));

    // Repeat the test using a different keyword.
    validatorMetadataNameSet.clear();

    metadata = registry.GetValidatorMetadataForKeyword(
        UsdGeomValidatorKeywordTokens->UsdGeomSubset);
    for (const UsdValidatorMetadata& metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedUsdGeomSubsetNames.begin(),
                           expectedUsdGeomSubsetNames.end()));
}

void
TestUsdShadeMaterialBindingRelationships()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->materialBindingRelationships);
    TF_AXIOM(validator);

    static const std::string layerContents =
        R"usda(#usda 1.0
               def Xform "MatBindAttributes"
               {
                   int material:binding = 42
                   token material:binding:someAttribute = "bogus"
               })usda";
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const SdfPath primPath("/MatBindAttributes");
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(primPath);

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 2u);

        {
            const UsdValidationError& error = errors[0u];

            const SdfPath attrPath =
                primPath.AppendProperty(UsdShadeTokens->materialBinding);

            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsProperty());
            TF_AXIOM(errorSite.GetProperty().GetPath() == attrPath);
            const std::string expectedErrorMsg =
                "Prim </MatBindAttributes> has material binding property "
                "'material:binding' that is not a relationship.";
            TF_AXIOM(error.GetMessage() == expectedErrorMsg);
        }

        {
            const UsdValidationError& error = errors[1u];

            const SdfPath attrPath =
                primPath.AppendProperty(TfToken(
                    SdfPath::JoinIdentifier(
                        UsdShadeTokens->materialBinding, "someAttribute")));

            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsProperty());
            TF_AXIOM(errorSite.GetProperty().GetPath() == attrPath);
            const std::string expectedErrorMsg =
                "Prim </MatBindAttributes> has material binding property "
                "'material:binding:someAttribute' that is not a relationship.";
            TF_AXIOM(error.GetMessage() == expectedErrorMsg);
        }
    }
}

void 
TestUsdShadeShaderPropertyCompliance()
{
    // Need to setup our test shader in sdrRegistry first
    UsdStageRefPtr shaderDefStage = UsdStage::Open("./shaderDefs.usda");
    UsdShadeShader shaderDef =
        UsdShadeShader::Get(shaderDefStage, SdfPath("/TestShaderNode"));
    SdrRegistry::GetInstance().AddDiscoveryResult(
        UsdShadeShaderDefUtils::GetNodeDiscoveryResults(
            shaderDef, shaderDefStage->GetRootLayer()->GetRealPath())[0]);

    // Now lets test our ShaderProperty validator
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->shaderSdrCompliance);
    TF_AXIOM(validator);

    static const std::string layerContents =
        R"usda(#usda 1.0
               def Shader "Test"
               {
                    uniform token info:id = "TestShaderNode"
                    int inputs:inputInt = 2
                    float inputs:inputFloat = 2.0
                    float3 inputs:inputColor = (2.0, 3.0, 4.0)
                    token outputs:surface
               }
               def Shader "Bogus"
               {
                    uniform token info:id = "Bogus"
               })usda";
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/Test"));

        UsdValidationErrorVector errors = validator->Validate(usdPrim);

        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath() == 
                 SdfPath("/Test.inputs:inputColor"));
        const std::string expectedErrorMsg = "Incorrect type for "
            "/Test.inputs:inputColor. Expected 'color3f'; "
            "got 'float3'.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/Bogus"));

        UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsProperty());
        TF_AXIOM(errors[0].GetSites()[0].GetProperty().GetPath() == 
                 SdfPath("/Bogus.info:id"));
        const std::string expectedErrorMsg = "shaderId 'Bogus' specified on "
            "shader prim </Bogus> not found in sdrRegistry.";
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
    }
}

static const std::string subsetsLayerContents =
R"usda(#usda 1.0
(
    defaultPrim = "SubsetsTest"
    metersPerUnit = 0.01
    upAxis = "Z"
)

def Xform "SubsetsTest" (
    kind = "component"
)
{
    def Xform "Geom"
    {
        def Mesh "Cube"
        {
            float3[] extent = [(-0.5, -0.5, -0.5), (0.5, 0.5, 0.5)]
            int[] faceVertexCounts = [4, 4, 4, 4, 4, 4]
            int[] faceVertexIndices = [0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4]
            point3f[] points = [(-0.5, -0.5, 0.5), (0.5, -0.5, 0.5), (-0.5, 0.5, 0.5), (0.5, 0.5, 0.5), (-0.5, 0.5, -0.5), (0.5, 0.5, -0.5), (-0.5, -0.5, -0.5), (0.5, -0.5, -0.5)]

            uniform token subsetFamily:materialBind:familyType = "unrestricted"

            def GeomSubset "materialBindShouldNotBeUnrestricted" (
                prepend apiSchemas = ["MaterialBindingAPI"]
            )
            {
                uniform token elementType = "face"
                uniform token familyName = "materialBind"
                int[] indices = [0, 2, 4]
                rel material:binding = </SubsetsTest/Materials/TestMaterial>
            }

            def GeomSubset "materialBindMissingElementType" (
                prepend apiSchemas = ["MaterialBindingAPI"]
            )
            {
                uniform token familyName = "materialBind"
                int[] indices = [1, 3, 5]
                rel material:binding = </SubsetsTest/Materials/TestMaterial>
            }

            def GeomSubset "materialBindMissingFamilyName" (
                prepend apiSchemas = ["MaterialBindingAPI"]
            )
            {
                uniform token elementType = "face"
                int[] indices = [1, 3, 5]
                rel material:binding = </SubsetsTest/Materials/TestMaterial>
            }
        }
    }

    def Scope "Materials"
    {
        def Material "TestMaterial"
        {
            token outputs:surface.connect = </SubsetsTest/Materials/TestMaterial/PreviewSurface.outputs:surface>

            def Shader "PreviewSurface"
            {
                uniform token info:id = "UsdPreviewSurface"
                color3f inputs:diffuseColor = (1.0, 0.0, 0.0)
                token outputs:surface
            }
        }
    }
}
)usda";

void
TestUsdShadeSubsetMaterialBindFamilyName()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->subsetMaterialBindFamilyName);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(subsetsLayerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/Cube/materialBindMissingFamilyName"));

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);
        const UsdValidationError& error = errors[0u];
        TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(error.GetSites().size() == 1u);
        const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsPrim());
        TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
        const std::string expectedErrorMsg =
            "GeomSubset prim "
            "</SubsetsTest/Geom/Cube/materialBindMissingFamilyName> "
            "with material bindings applied but no authored family name "
            "should set familyName to 'materialBind'.";
        TF_AXIOM(error.GetMessage() == expectedErrorMsg);
    }
}

void
TestUsdShadeSubsetsMaterialBindFamily()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    const UsdValidator* validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->subsetsMaterialBindFamily);
    TF_AXIOM(validator);

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(subsetsLayerContents);
    UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);

    {
        const UsdPrim usdPrim = usdStage->GetPrimAtPath(
            SdfPath("/SubsetsTest/Geom/Cube"));

        const UsdValidationErrorVector errors = validator->Validate(usdPrim);
        TF_AXIOM(errors.size() == 1u);

        {
            const UsdValidationError& error = errors[0u];
            TF_AXIOM(error.GetType() == UsdValidationErrorType::Error);
            TF_AXIOM(error.GetSites().size() == 1u);
            const UsdValidationErrorSite& errorSite = error.GetSites()[0u];
            TF_AXIOM(errorSite.IsValid());
            TF_AXIOM(errorSite.IsPrim());
            TF_AXIOM(errorSite.GetPrim().GetPath() == usdPrim.GetPath());
            const std::string expectedErrorMsg =
                "Imageable prim </SubsetsTest/Geom/Cube> has 'materialBind' "
                "subset family with invalid family type 'unrestricted'. Family "
                "type should be 'nonOverlapping' or 'partition' instead.";
            TF_AXIOM(error.GetMessage() == expectedErrorMsg);
        }
    }
}

void
TestUsdShadeMaterialBindingAPIAppliedValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
        UsdShadeValidatorNameTokens->materialBindingApiAppliedValidator);
    TF_AXIOM(validator);

    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    const UsdPrim usdPrim = usdStage->DefinePrim(SdfPath("/Test"));
    UsdShadeMaterial material = 
        UsdShadeMaterial::Define(usdStage, SdfPath("/Test/Material"));

    // Create the material binding relationship manually
    UsdRelationship materialBinding = usdPrim.CreateRelationship(
        TfToken("material:binding"));
    materialBinding.AddTarget(material.GetPath());

    UsdValidationErrorVector errors = validator->Validate(usdPrim);

    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() == SdfPath("/Test"));
    const std::string expectedErrorMsg = 
        "Found material bindings but no MaterialBindingAPI applied on the prim "
        "</Test>.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Apply the material binding API to the prim and bind the material
    UsdShadeMaterialBindingAPI bindingAPI = 
        UsdShadeMaterialBindingAPI::Apply(usdPrim);
    bindingAPI.Bind(material);

    errors = validator->Validate(usdPrim);

    // Verify the errors are fixed
    TF_AXIOM(errors.size() == 0);
}

int
main()
{
    TestUsdShadeValidators();
    TestUsdShadeMaterialBindingAPIAppliedValidator();
    TestUsdShadeMaterialBindingRelationships();
    TestUsdShadeShaderPropertyCompliance();
    TestUsdShadeSubsetMaterialBindFamilyName();
    TestUsdShadeSubsetsMaterialBindFamily();
    return EXIT_SUCCESS;
};
